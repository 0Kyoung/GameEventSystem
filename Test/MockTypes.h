#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 실제 게임 서버 타입을 대신하는 Mock 구조체
// 포트폴리오 빌드 및 단위 테스트용
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>

// ── UserEventInfo ─────────────────────────────────────────────────────────────
struct UserEventInfo
{
    uint32_t event_id_          = 0;
    int32_t  event_step_        = 0;
    int32_t  sub_event_step_    = 0;
    bool     event_deleted_     = false;
    time_t   last_update_time_  = 0;

    void Reset()
    {
        event_step_       = 0;
        sub_event_step_   = 0;
        event_deleted_    = false;
        last_update_time_ = 0;
    }

    bool IsSpecialAttendance(int32_t /*step*/) const { return false; }
};

// ── TableUserEvent ────────────────────────────────────────────────────────────
struct TableUserEvent
{
    uint32_t EventId            = 0;
    uint32_t EventType          = 0;
    uint32_t StartCondition     = 0;
    uint32_t CloseCondition     = 0;
    time_t   start_time         = 0;
    time_t   close_time         = 0;
    int32_t  MaxStep            = 0;
    uint32_t EventContentsId    = 0;
    uint32_t WorldType          = 0;
    int32_t  EventMinLevel      = 0;
    time_t   new_period_start_time    = 0;
    time_t   return_period_start_time = 0;
};

// ── UserEventContainer (CUser 내부 이벤트 관리) ───────────────────────────────
class UserEventContainer
{
public:
    UserEventInfo* GetInfo(uint32_t event_id)
    {
        auto it = events_.find(event_id);
        return (it != events_.end()) ? it->second.get() : nullptr;
    }

    void Insert(std::shared_ptr<UserEventInfo> info)
    {
        events_[info->event_id_] = std::move(info);
    }

    bool IsProcess() const { return is_processing_; }
    void SetProcess(bool v) { is_processing_ = v; }

    std::chrono::system_clock::time_point GetLastCheckTime() const { return last_check_; }
    void SetLastCheckTime(std::chrono::system_clock::time_point t) { last_check_ = t; }

private:
    std::unordered_map<uint32_t, std::shared_ptr<UserEventInfo>> events_;
    bool is_processing_ = false;
    std::chrono::system_clock::time_point last_check_{};
};

// ── CUser ─────────────────────────────────────────────────────────────────────
class CUser
{
public:
    explicit CUser(uint64_t uid) : uid_(uid) {}
    uint64_t GetUID() const { return uid_; }
    int32_t  GetLevel() const { return level_; }
    void     SetLevel(int32_t lv) { level_ = lv; }

    UserEventContainer& GetUserEvent() { return event_container_; }

    bool IsNewCharacter(time_t /*period*/) const { return is_new_; }
    bool IsReturningUser(time_t /*period*/) const { return is_returning_; }

    void SetIsNew(bool v)       { is_new_ = v; }
    void SetIsReturning(bool v) { is_returning_ = v; }

private:
    uint64_t           uid_     = 0;
    int32_t            level_   = 1;
    bool               is_new_        = false;
    bool               is_returning_  = false;
    UserEventContainer event_container_;
};

// ── CZoneGroup (최소 구현) ────────────────────────────────────────────────────
class CZoneGroup
{
public:
    int32_t GetWorkerIndex() const { return 0; }
};
