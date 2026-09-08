#include "DbTypes.h"

#ifdef DB_TEST
    #include "MockDb.h"
#else
    #include "MySqlConnectionPool.h"
#endif

namespace GameDb
{
    std::vector<DbEventRow> QueryUserEvents(uint64_t uid)
    {
#ifdef DB_TEST
        // 포트폴리오/유닛테스트 빌드: 인메모리 Mock DB, 왕복 지연 20ms 흉내
        return MockDb::GetInstance().Query(uid, std::chrono::milliseconds(20));
#else
        // 실제 서버 빌드: MySQL 커넥션 풀에서 커넥션을 빌려 동기 쿼리 실행
        auto conn = MySqlConnectionPool::GetInstance().Acquire();
        std::vector<DbEventRow> rows = conn->QueryUserEvents(uid);
        MySqlConnectionPool::GetInstance().Release(std::move(conn));
        return rows;
#endif
    }

} // namespace GameDb
