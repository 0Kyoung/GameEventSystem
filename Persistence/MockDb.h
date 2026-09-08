#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// DB_TEST 빌드에서만 컴파일되는 인메모리 Mock DB.
//
// 실제 MySQL 왕복 지연을 흉내 내기 위해 Query()에 약간의 sleep을 넣어 두었다.
// → AsyncDbJobQueue를 거치지 않고 게임 스레드에서 직접 호출하면
//    프레임이 그대로 지연된다는 것을 테스트로 보여줄 수 있다.
// ─────────────────────────────────────────────────────────────────────────────
#include "DbTypes.h"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <thread>

namespace GameDb
{
    class MockDb
    {
    public:
        static MockDb& GetInstance()
        {
            static MockDb instance;
            return instance;
        }

        // 테스트 데이터 주입 (실제로는 INSERT 된 상태의 테이블)
        void Seed(uint64_t uid, std::vector<DbEventRow> rows)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            table_[uid] = std::move(rows);
        }

        // 동기 조회 (블로킹). 실제 네트워크 왕복을 흉내 내기 위해 지연을 준다.
        std::vector<DbEventRow> Query(uint64_t uid, std::chrono::milliseconds simulated_latency) const
        {
            std::this_thread::sleep_for(simulated_latency);

            std::lock_guard<std::mutex> lock(mutex_);
            auto it = table_.find(uid);
            if (it == table_.end())
                return {};
            return it->second;
        }

    private:
        MockDb() = default;
        mutable std::mutex mutex_;
        std::unordered_map<uint64_t, std::vector<DbEventRow>> table_;
    };

} // namespace GameDb
