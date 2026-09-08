#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// DB_TEST 미정의(실제 서버 빌드)일 때만 컴파일되는 MySQL C API 기반 커넥션 풀.
//
// - MYSQL* 핸들 하나는 스레드 세이프하지 않으므로, 워커 스레드 수만큼
//   미리 연결을 만들어 두고 Acquire/Release로 빌려 쓰는 단순 풀로 구성했다.
// - AsyncDbJobQueue의 워커 스레드에서만 Acquire()가 호출되므로
//   풀 크기 = 워커 스레드 수로 맞추면 대기 없이 동작한다.
//
// 빌드 요구사항: MySQL Connector/C (libmysqlclient) 헤더/라이브러리
//   Windows : mysqlclient.lib 링크, mysql.h 포함 경로 추가
//   Linux   : libmysqlclient-dev 설치 후 -lmysqlclient
// ─────────────────────────────────────────────────────────────────────────────
#include "DbTypes.h"
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <string>

struct MYSQL; // <mysql/mysql.h> 전방 선언 (헤더 의존 최소화)

namespace GameDb
{
    struct DbConfig
    {
        std::string host     = "127.0.0.1";
        std::string user     = "game";
        std::string password = "";
        std::string database = "game_db";
        unsigned    port     = 3306;
    };

    // 커넥션 하나에 대한 래퍼. 실제 쿼리 로직은 여기 모아 둔다.
    class MySqlConnection
    {
    public:
        explicit MySqlConnection(const DbConfig& config);
        ~MySqlConnection();

        MySqlConnection(const MySqlConnection&) = delete;
        MySqlConnection& operator=(const MySqlConnection&) = delete;

        bool IsConnected() const { return handle_ != nullptr; }

        // SELECT event_id, event_step, sub_event_step, event_deleted, last_update_time
        //   FROM user_event WHERE uid = ?
        std::vector<DbEventRow> QueryUserEvents(uint64_t uid);

    private:
        bool Reconnect();

        DbConfig config_;
        MYSQL*   handle_ = nullptr;
    };

    // 고정 크기 커넥션 풀
    class MySqlConnectionPool
    {
    public:
        static MySqlConnectionPool& GetInstance()
        {
            static MySqlConnectionPool instance;
            return instance;
        }

        // 서버 시작 시 한 번 호출 (worker_count = AsyncDbJobQueue 워커 스레드 수)
        void Init(const DbConfig& config, int worker_count);

        std::unique_ptr<MySqlConnection> Acquire();
        void Release(std::unique_ptr<MySqlConnection> conn);

    private:
        MySqlConnectionPool() = default;

        DbConfig config_;
        std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<std::unique_ptr<MySqlConnection>> pool_;
    };

} // namespace GameDb
