#include "MySqlConnectionPool.h"

#ifndef DB_TEST
#include <mysql/mysql.h>
#include <iostream>

namespace GameDb
{
    MySqlConnection::MySqlConnection(const DbConfig& config)
        : config_(config)
    {
        Reconnect();
    }

    MySqlConnection::~MySqlConnection()
    {
        if (handle_ != nullptr)
            mysql_close(handle_);
    }

    bool MySqlConnection::Reconnect()
    {
        if (handle_ != nullptr)
        {
            mysql_close(handle_);
            handle_ = nullptr;
        }

        MYSQL* h = mysql_init(nullptr);
        if (h == nullptr)
            return false;

        // 쿼리 도중 커넥션이 끊기는 경우를 대비해 자동 재연결 옵션을 켠다.
        bool reconnect = true;
        mysql_options(h, MYSQL_OPT_RECONNECT, &reconnect);

        MYSQL* connected = mysql_real_connect(
            h,
            config_.host.c_str(),
            config_.user.c_str(),
            config_.password.c_str(),
            config_.database.c_str(),
            config_.port,
            nullptr, 0);

        if (connected == nullptr)
        {
            std::cerr << "[MySqlConnection] connect failed: " << mysql_error(h) << "\n";
            mysql_close(h);
            return false;
        }

        handle_ = connected;
        return true;
    }

    std::vector<DbEventRow> MySqlConnection::QueryUserEvents(uint64_t uid)
    {
        std::vector<DbEventRow> result;

        if (handle_ == nullptr && !Reconnect())
            return result;

        const std::string sql =
            "SELECT event_id, event_step, sub_event_step, event_deleted, "
            "UNIX_TIMESTAMP(last_update_time) "
            "FROM user_event WHERE uid = " + std::to_string(uid);

        // mysql_real_query: uid는 uint64_t를 std::to_string으로 직접 만든 값이라
        // 문자열 이스케이프가 필요한 외부 입력이 아니다.
        // (문자열 파라미터가 섞이는 쿼리는 prepared statement로 별도 구성)
        if (mysql_real_query(handle_, sql.c_str(), static_cast<unsigned long>(sql.size())) != 0)
        {
            std::cerr << "[MySqlConnection] query failed: " << mysql_error(handle_) << "\n";
            return result;
        }

        MYSQL_RES* res = mysql_store_result(handle_);
        if (res == nullptr)
            return result;

        result.reserve(static_cast<size_t>(mysql_num_rows(res)));

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            DbEventRow ev;
            ev.event_id         = row[0] ? static_cast<uint32_t>(std::stoul(row[0])) : 0;
            ev.event_step       = row[1] ? std::stoi(row[1]) : 0;
            ev.sub_event_step   = row[2] ? std::stoi(row[2]) : 0;
            ev.event_deleted    = row[3] ? (std::stoi(row[3]) != 0) : false;
            ev.last_update_time = row[4] ? static_cast<time_t>(std::stoll(row[4])) : 0;
            result.push_back(ev);
        }

        mysql_free_result(res);
        return result;
    }

    void MySqlConnectionPool::Init(const DbConfig& config, int worker_count)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
        for (int i = 0; i < worker_count; ++i)
            pool_.push(std::make_unique<MySqlConnection>(config_));
    }

    std::unique_ptr<MySqlConnection> MySqlConnectionPool::Acquire()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !pool_.empty(); });
        auto conn = std::move(pool_.front());
        pool_.pop();
        return conn;
    }

    void MySqlConnectionPool::Release(std::unique_ptr<MySqlConnection> conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(std::move(conn));
        cv_.notify_one();
    }

} // namespace GameDb

#endif // !DB_TEST
