#include <iostream>
#include <cassert>
#include <chrono>
#include <ctime>
#include <string>

#include "../Core/EventType.h"
#include "../Core/EventContext.h"
#include "../Core/IEventHandler.h"
#include "../Core/EventDispatcher.h"
#include "../Handlers/AttendanceEventHandler.h"
#include "../Handlers/BingoEventHandler.h"
#include "../Handlers/MissionPassEventHandler.h"
#include "../Handlers/SiegeEveEventHandler.h"
#include "../Handlers/HuntingEventHandler.h"
#include "../Handlers/BenedictionEventHandler.h"
#include "../Update/EventUpdateChecker.h"
#include "../Update/EventLoginSyncer.h"
#include "../Update/EventInfoPacketBuilder.h"
// MockTypes.h는 GameTypes.h를 통해 자동 포함됨 (-DGAME_EVENT_TEST 빌드 시)

using namespace GameEvent;

// ─── 테스트 헬퍼 ─────────────────────────────────────────────────────────────

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

static void RegisterAll(EventDispatcher& d)
{
    d.RegisterHandler(std::make_shared<AttendanceEventHandler>());
    d.RegisterHandler(std::make_shared<BingoEventHandler>());
    d.RegisterHandler(std::make_shared<MissionPassEventHandler>());
    d.RegisterHandler(std::make_shared<SiegeEveEventHandler>());
    d.RegisterHandler(std::make_shared<HuntingEventHandler>());
    d.RegisterHandler(std::make_shared<BenedictionEventHandler>());
}

static TableUserEvent MakeTable(
    uint32_t id, EventType type,
    time_t start = 0, time_t close = 0,
    uint32_t start_cond = 0, uint32_t close_cond = 0,
    int32_t max_step = 28)
{
    TableUserEvent tbl{};
    tbl.EventId        = id;
    tbl.EventType      = static_cast<uint32_t>(type);
    tbl.start_time     = start;
    tbl.close_time     = close;
    tbl.StartCondition = start_cond;
    tbl.CloseCondition = close_cond;
    tbl.MaxStep        = max_step;
    return tbl;
}

static EventContext MakeCtx(
    const TableUserEvent& tbl, CUser& user, CZoneGroup& zone,
    time_t now, time_t last_check)
{
    EventContext ctx;
    ctx.table_event     = &tbl;
    ctx.user            = std::shared_ptr<CUser>(&user, [](CUser*){});
    ctx.zone_group      = std::shared_ptr<CZoneGroup>(&zone, [](CZoneGroup*){});
    ctx.now             = now;
    ctx.last_check_time = last_check;
    ctx.event_info      = user.GetUserEvent().GetInfo(tbl.EventId);
    return ctx;
}

// ─── TC 1: 핸들러 등록 확인 ──────────────────────────────────────────────────

void TC1_AllHandlersRegistered(EventDispatcher& d)
{
    std::cout << "\n[TC1] 핸들러 등록 확인\n";
    const std::vector<std::pair<EventType, std::string>> types = {
        { EventType::RepeatAttendance,             "RepeatAttendance" },
        { EventType::SpecialAttendance,            "SpecialAttendance" },
        { EventType::CumulativeAttendance,         "CumulativeAttendance" },
        { EventType::Attendance,                   "Attendance" },
        { EventType::ContinuityAttendance,         "ContinuityAttendance" },
        { EventType::Attendance14Days,             "Attendance14Days" },
        { EventType::AttendanceNewAndReturn7Days,  "AttendanceNew7Days" },
        { EventType::AttendanceNewAndReturn14Days, "AttendanceNew14Days" },
        { EventType::Bingo,                        "Bingo" },
        { EventType::MissionPassGrowth,            "MissionPassGrowth" },
        { EventType::MissionPassChallenge,         "MissionPassChallenge" },
        { EventType::SiegeEve,                     "SiegeEve" },
        { EventType::Hunting,                      "Hunting" },
        { EventType::HuntingToExchange,            "HuntingToExchange" },
        { EventType::Benediction,                  "Benediction" },
    };
    for (auto& [type, name] : types)
        CHECK(d.HasHandler(type), name + " 핸들러 등록됨");
}

// ─── TC 2: 미등록 타입 안전 처리 ─────────────────────────────────────────────

void TC2_UnregisteredType(EventDispatcher& d)
{
    std::cout << "\n[TC2] 미등록 타입 안전 처리\n";
    CUser user(1);
    CZoneGroup zone;
    auto tbl = MakeTable(9999, EventType::None);
    auto ctx = MakeCtx(tbl, user, zone, std::time(nullptr), 0);

    CHECK(d.DispatchTimeCheck(EventType::None, ctx) == HandleResult::None,
          "미등록 타입 TimeCheck → None");
    CHECK(d.DispatchLoginSync(EventType::None, ctx) == false,
          "미등록 타입 LoginSync → false");
    CHECK(d.DispatchFillPacket(EventType::None, ctx, nullptr) == false,
          "미등록 타입 FillPacket → false");
}

// ─── TC 3: 출석 이벤트 종료 처리 ─────────────────────────────────────────────

void TC3_Attendance_EventClose(EventDispatcher& d)
{
    std::cout << "\n[TC3] 출석 이벤트 종료 처리\n";
    const time_t now = std::time(nullptr);
    CUser user(100);
    CZoneGroup zone;

    auto tbl = MakeTable(101, EventType::RepeatAttendance,
        now - 7200, now - 3600,
        static_cast<uint32_t>(StartCondition::Time),
        static_cast<uint32_t>(CloseCondition::Time));

    auto info = std::make_shared<UserEventInfo>();
    info->event_id_ = tbl.EventId; info->event_step_ = 5;
    user.GetUserEvent().Insert(info);

    auto ctx = MakeCtx(tbl, user, zone, now, now - 7200);
    CHECK(d.DispatchTimeCheck(EventType::RepeatAttendance, ctx) == HandleResult::Notified,
          "종료 조건 충족 → Notified");
}

// ─── TC 4: 출석 이벤트 이미 삭제된 경우 무시 ─────────────────────────────────

void TC4_Attendance_AlreadyDeleted(EventDispatcher& d)
{
    std::cout << "\n[TC4] 출석 이벤트 이미 삭제된 경우\n";
    const time_t now = std::time(nullptr);
    CUser user(101);
    CZoneGroup zone;

    auto tbl = MakeTable(102, EventType::Attendance,
        now - 7200, now - 3600,
        static_cast<uint32_t>(StartCondition::Time),
        static_cast<uint32_t>(CloseCondition::Time));

    auto info = std::make_shared<UserEventInfo>();
    info->event_id_ = tbl.EventId;
    info->event_deleted_ = true; // 이미 삭제됨
    user.GetUserEvent().Insert(info);

    auto ctx = MakeCtx(tbl, user, zone, now, now - 7200);
    CHECK(d.DispatchTimeCheck(EventType::Attendance, ctx) == HandleResult::None,
          "이미 삭제된 이벤트 종료 → None");
}

// ─── TC 5: 사냥 이벤트 기간 외 처리 ─────────────────────────────────────────

void TC5_Hunting_OutOfPeriod(EventDispatcher& d)
{
    std::cout << "\n[TC5] 사냥 이벤트 기간 외\n";
    const time_t now = std::time(nullptr);
    CUser user(200);
    CZoneGroup zone;

    auto tbl_future = MakeTable(901, EventType::Hunting, now + 3600, now + 7200);
    auto ctx_future = MakeCtx(tbl_future, user, zone, now, now - 60);
    CHECK(d.DispatchTimeCheck(EventType::Hunting, ctx_future) == HandleResult::None,
          "미래 이벤트 → None");

    auto tbl_past = MakeTable(902, EventType::Hunting, now - 7200, now - 3600);
    auto ctx_past = MakeCtx(tbl_past, user, zone, now, now - 60);
    CHECK(d.DispatchTimeCheck(EventType::Hunting, ctx_past) == HandleResult::None,
          "과거 이벤트 → None");
}

// ─── TC 6: 사냥 이벤트 시작 전환 ────────────────────────────────────────────

void TC6_Hunting_EventStart(EventDispatcher& d)
{
    std::cout << "\n[TC6] 사냥 이벤트 시작 전환\n";
    const time_t now = std::time(nullptr);
    CUser user(201);
    CZoneGroup zone;

    // 30초 전 시작, 마지막 체크는 60초 전
    auto tbl = MakeTable(903, EventType::Hunting, now - 30, now + 7200);
    auto ctx = MakeCtx(tbl, user, zone, now, now - 60);
    CHECK(d.DispatchTimeCheck(EventType::Hunting, ctx) == HandleResult::Notified,
          "사냥 이벤트 시작 전환 → Notified");
}

// ─── TC 7: 미션 패스 신규 시작 ───────────────────────────────────────────────

void TC7_MissionPass_NewStart(EventDispatcher& d)
{
    std::cout << "\n[TC7] 미션 패스 신규 시작\n";
    const time_t now = std::time(nullptr);
    CUser user(300);
    CZoneGroup zone;

    auto tbl = MakeTable(2001, EventType::MissionPassGrowth,
        now - 30, now + 86400,
        static_cast<uint32_t>(StartCondition::Time),
        static_cast<uint32_t>(CloseCondition::Time));

    auto ctx = MakeCtx(tbl, user, zone, now, now - 60); // 유저 데이터 없음
    CHECK(d.DispatchTimeCheck(EventType::MissionPassGrowth, ctx) == HandleResult::Notified,
          "미션 패스 신규 시작 → Notified");
}

// ─── TC 8: 미션 패스 종료 ────────────────────────────────────────────────────

void TC8_MissionPass_EventClose(EventDispatcher& d)
{
    std::cout << "\n[TC8] 미션 패스 종료\n";
    const time_t now = std::time(nullptr);
    CUser user(301);
    CZoneGroup zone;

    auto tbl = MakeTable(2002, EventType::MissionPassChallenge,
        now - 7200, now - 3600,
        static_cast<uint32_t>(StartCondition::Time),
        static_cast<uint32_t>(CloseCondition::Time));

    auto info = std::make_shared<UserEventInfo>();
    info->event_id_ = tbl.EventId; info->event_step_ = 10;
    user.GetUserEvent().Insert(info);

    auto ctx = MakeCtx(tbl, user, zone, now, now - 7200);
    CHECK(d.DispatchTimeCheck(EventType::MissionPassChallenge, ctx) == HandleResult::Notified,
          "미션 패스 종료 → Notified");
}

// ─── TC 9: FillPacketInfo 호출 ───────────────────────────────────────────────

void TC9_FillPacketInfo(EventDispatcher& d)
{
    std::cout << "\n[TC9] FillPacketInfo 호출\n";
    const time_t now = std::time(nullptr);
    CUser user(400);
    CZoneGroup zone;

    auto tbl = MakeTable(110, EventType::Attendance,
        now - 3600, now + 3600,
        static_cast<uint32_t>(StartCondition::Time),
        static_cast<uint32_t>(CloseCondition::Time));

    auto info = std::make_shared<UserEventInfo>();
    info->event_id_ = tbl.EventId; info->event_step_ = 3;
    user.GetUserEvent().Insert(info);

    auto ctx = MakeCtx(tbl, user, zone, now, now - 60);
    int dummy = 0;
    CHECK(d.DispatchFillPacket(EventType::Attendance, ctx, &dummy) == true,
          "FillPacketInfo 호출 성공");
}

// ─── TC 10: EventLoginSyncer / EventInfoPacketBuilder ────────────────────────

void TC10_SyncerAndBuilder()
{
    std::cout << "\n[TC10] EventLoginSyncer / EventInfoPacketBuilder\n";
    auto user = std::make_shared<CUser>(500);
    auto zone = std::make_shared<CZoneGroup>();

    // nullptr 안전 처리
    EventLoginSyncer::Sync(nullptr, user);
    EventLoginSyncer::Sync(zone, nullptr);
    EventLoginSyncer::Sync(zone, user);

    CHECK(EventInfoPacketBuilder::Send(nullptr, user) == false,
          "Send(nullptr zone) → false");
    CHECK(EventInfoPacketBuilder::Send(zone, nullptr) == false,
          "Send(nullptr user) → false");
    CHECK(EventInfoPacketBuilder::Send(zone, user) == true,
          "Send(valid args) → true");
}

// ─── TC 11: 핸들러 미등록 타입 경계 ─────────────────────────────────────────

void TC11_HandlerTypeBoundary(EventDispatcher& d)
{
    std::cout << "\n[TC11] 핸들러 미등록 타입 경계\n";
    CHECK(!d.HasHandler(EventType::BattlePass),  "BattlePass - 미등록 확인");
    CHECK(!d.HasHandler(EventType::Gacha),       "Gacha - 미등록 확인");
    CHECK(!d.HasHandler(EventType::FullBanner),  "FullBanner - 미등록 확인");
}

// ─── TC 12: RepeatAttendance MaxStep 리셋 감지 ───────────────────────────────

void TC12_RepeatAttendance_MaxStepReset(EventDispatcher& d)
{
    std::cout << "\n[TC12] RepeatAttendance MaxStep 리셋 감지\n";
    const time_t now = std::time(nullptr);
    CUser user(600);
    CZoneGroup zone;

    auto tbl = MakeTable(201, EventType::RepeatAttendance,
        now - 86400, now + 86400,
        static_cast<uint32_t>(StartCondition::Time),
        static_cast<uint32_t>(CloseCondition::Time),
        28);

    auto info = std::make_shared<UserEventInfo>();
    info->event_id_   = tbl.EventId;
    info->event_step_ = 28; // MaxStep 도달
    user.GetUserEvent().Insert(info);

    auto ctx = MakeCtx(tbl, user, zone, now, now - 60);
    ctx.reset_time = now - 30; // 리셋 시각이 now보다 과거 → 리셋 대상

    auto result = d.DispatchTimeCheck(EventType::RepeatAttendance, ctx);
    // MaxStep 도달 + reset_time <= now → 리셋 처리
    CHECK(result != HandleResult::Notified,
          "MaxStep 리셋 처리 (종료 Notified 아님)");
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "================================\n";
    std::cout << "  GameEventSystem 테스트\n";
    std::cout << "================================\n";

    EventDispatcher& dispatcher = EventDispatcher::GetInstance();
    RegisterAll(dispatcher);

    TC1_AllHandlersRegistered(dispatcher);
    TC2_UnregisteredType(dispatcher);
    TC3_Attendance_EventClose(dispatcher);
    TC4_Attendance_AlreadyDeleted(dispatcher);
    TC5_Hunting_OutOfPeriod(dispatcher);
    TC6_Hunting_EventStart(dispatcher);
    TC7_MissionPass_NewStart(dispatcher);
    TC8_MissionPass_EventClose(dispatcher);
    TC9_FillPacketInfo(dispatcher);
    TC10_SyncerAndBuilder();
    TC11_HandlerTypeBoundary(dispatcher);
    TC12_RepeatAttendance_MaxStepReset(dispatcher);

    std::cout << "\n================================\n";
    std::cout << "  결과: " << s_pass << " 통과 / "
              << s_fail << " 실패\n";
    std::cout << "================================\n";

    return s_fail == 0 ? 0 : 1;
}
