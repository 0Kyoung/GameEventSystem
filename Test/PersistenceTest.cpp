// ─────────────────────────────────────────────────────────────────────────────
// Persistence 계층 단위 테스트.
//
// Network/ 디렉터리(IOCP)와 달리 이 테스트는 Windows API에 의존하지 않는다.
// Core/GameTypes.h 가 -DGAME_EVENT_TEST 일 때 Test/MockTypes.h 로 라우팅되고,
// Persistence/DbTypes.h 가 -DDB_TEST 일 때 MockDb 로 라우팅되기 때문에,
// 아래 빌드 명령만으로 리눅스/맥에서도 그대로 빌드·실행된다.
//
//   g++ -std=c++17 -DGAME_EVENT_TEST -DDB_TEST -I. -pthread \
//       Test/PersistenceTest.cpp Persistence/AsyncDbJobQueue.cpp \
//       Persistence/EventDbBridge.cpp Persistence/DbTypes.cpp \
//       -o persistence_test
// ─────────────────────────────────────────────────────────────────────────────
#include "../Core/GameTypes.h"
#include "../Persistence/AsyncDbJobQueue.h"
#include "../Persistence/EventDbBridge.h"
#include "../Persistence/MockDb.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

static int s_pass = 0;
static int s_fail = 0;

#define CHECK(condition, msg)                               \
    do {                                                    \
        if (condition) {                                    \
            std::cout << "  [PASS] " << (msg) << "\n";     \
            ++s_pass;                                       \
        } else {                                            \
            std::cout << "  [FAIL] " << (msg) << "\n";     \
            ++s_fail;                                       \
        }                                                   \
    } while (0)

using namespace GameDb;

// ─── TC1: DB 로드 결과가 메인 스레드 반영 이전에는 보이지 않는다 ────────────
//
// Enqueue 시점에는 아직 워커 스레드가 쿼리를 실행 중일 수 있다.
// ProcessCompletions()를 호출하기 전까지는 UserEventContainer가
// 절대 변경되어서는 안 된다 — 이게 이 큐를 만든 이유 그 자체다.
void TC1_MainThreadBoundary()
{
    std::cout << "\n[TC1] 메인 스레드 반영 경계\n";

    constexpr uint64_t uid = 1001;
    MockDb::GetInstance().Seed(uid, {
        { 101, 5, 0, false, 1000 },
        { 601, 2, 0, false, 2000 },
    });

    auto user = std::make_shared<CUser>(uid);
    std::atomic<bool> on_loaded_called{ false };

    EventDbBridge::LoadUserEventsAsync(uid, user, [&]() {
        on_loaded_called = true;
    });

    // 워커 스레드가 (인위적으로 20ms 지연을 준) 쿼리를 끝낼 시간을 준다.
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    CHECK(user->GetUserEvent().GetInfo(101) == nullptr,
          "ProcessCompletions 호출 전에는 반영되지 않음");
    CHECK(on_loaded_called.load() == false,
          "ProcessCompletions 호출 전에는 on_loaded도 호출되지 않음");

    AsyncDbJobQueue::GetInstance().ProcessCompletions();

    CHECK(user->GetUserEvent().GetInfo(101) != nullptr,
          "ProcessCompletions 이후 이벤트 101 반영됨");
    CHECK(user->GetUserEvent().GetInfo(101)->event_step_ == 5,
          "이벤트 101의 event_step 값이 DB 값과 일치");
    CHECK(user->GetUserEvent().GetInfo(601) != nullptr,
          "ProcessCompletions 이후 이벤트 601 반영됨");
    CHECK(on_loaded_called.load() == true,
          "ProcessCompletions 이후 on_loaded 콜백 실행됨");
}

// ─── TC2: DB에 데이터가 없는 신규 유저도 안전하게 처리 ───────────────────────
void TC2_EmptyResult()
{
    std::cout << "\n[TC2] 신규 유저(빈 결과) 처리\n";

    constexpr uint64_t uid = 1002; // Seed 하지 않음 = 신규 유저
    auto user = std::make_shared<CUser>(uid);
    bool called = false;

    EventDbBridge::LoadUserEventsAsync(uid, user, [&]() { called = true; });
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    AsyncDbJobQueue::GetInstance().ProcessCompletions();

    CHECK(called, "빈 결과여도 on_loaded 콜백은 호출됨");
    CHECK(user->GetUserEvent().GetInfo(101) == nullptr,
          "데이터가 없으므로 어떤 이벤트도 채워지지 않음");
}

// ─── TC3: user == nullptr 방어 ───────────────────────────────────────────────
void TC3_NullUserSafety()
{
    std::cout << "\n[TC3] user == nullptr 방어\n";
    bool called = false;
    EventDbBridge::LoadUserEventsAsync(9999, nullptr, [&]() { called = true; });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    AsyncDbJobQueue::GetInstance().ProcessCompletions();
    CHECK(called == false, "user가 nullptr이면 아무 작업도 큐잉되지 않음");
}

// ─── TC4: 여러 유저의 로그인 요청이 동시에 몰려도 서로 섞이지 않는다 ────────
void TC4_ConcurrentLogins()
{
    std::cout << "\n[TC4] 동시 로그인 다건 처리\n";

    constexpr int kUserCount = 20;
    std::vector<std::shared_ptr<CUser>> users;
    std::atomic<int> loaded_count{ 0 };

    for (int i = 0; i < kUserCount; ++i)
    {
        const uint64_t uid = 2000 + i;
        MockDb::GetInstance().Seed(uid, { { 101, static_cast<int32_t>(i), 0, false, 0 } });

        auto user = std::make_shared<CUser>(uid);
        users.push_back(user);
        EventDbBridge::LoadUserEventsAsync(uid, user, [&]() { ++loaded_count; });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    AsyncDbJobQueue::GetInstance().ProcessCompletions();

    CHECK(loaded_count.load() == kUserCount, "모든 유저의 on_loaded가 호출됨");

    bool all_correct = true;
    for (int i = 0; i < kUserCount; ++i)
    {
        auto* info = users[i]->GetUserEvent().GetInfo(101);
        if (info == nullptr || info->event_step_ != i)
        {
            all_correct = false;
            break;
        }
    }
    CHECK(all_correct, "각 유저가 자신의 event_step 값만 정확히 받음 (교차 오염 없음)");
}

int main()
{
    std::cout << "================================\n";
    std::cout << "  Persistence 계층 테스트\n";
    std::cout << "================================\n";

    AsyncDbJobQueue::GetInstance().Start(/*worker_count=*/4);

    TC1_MainThreadBoundary();
    TC2_EmptyResult();
    TC3_NullUserSafety();
    TC4_ConcurrentLogins();

    AsyncDbJobQueue::GetInstance().Stop();

    std::cout << "\n================================\n";
    std::cout << "  결과: " << s_pass << " 통과 / " << s_fail << " 실패\n";
    std::cout << "================================\n";

    return s_fail == 0 ? 0 : 1;
}
