#include "EventUpdateChecker.h"

namespace GameEvent
{

    void EventUpdateChecker::Check(
        const std::chrono::system_clock::time_point& now_tp,
        const std::shared_ptr<CZoneGroup>& zone_group,
        const std::shared_ptr<CUser>& user)
    {
        if (!ShouldCheck(now_tp, user))
            return;

        const auto now        = std::chrono::system_clock::to_time_t(now_tp);
        const auto last_check = std::chrono::system_clock::to_time_t(
                                    user->GetUserEvent().GetLastCheckTime());

        auto base_ctx = BuildBaseContext(now, last_check, zone_group, user);
        [[maybe_unused]] bool notify = ProcessEventLoop(base_ctx, zone_group, user);

        // if (IsReset_Day(now, last_check, reset_hour) || notify)
        //     Process::SendUserEventInfo(zone_group, user);

        user->GetUserEvent().SetLastCheckTime(now_tp);
    }

    bool EventUpdateChecker::ShouldCheck(
        const std::chrono::system_clock::time_point& now_tp,
        const std::shared_ptr<CUser>& user)
    {
        constexpr auto kCheckInterval = std::chrono::seconds(5);
        return (now_tp - user->GetUserEvent().GetLastCheckTime()) >= kCheckInterval;
    }

    bool EventUpdateChecker::ProcessEventLoop(
        [[maybe_unused]] const EventContext& base_ctx,
        [[maybe_unused]] const std::shared_ptr<CZoneGroup>& zone_group,
        [[maybe_unused]] const std::shared_ptr<CUser>& user)
    {
        bool notify_change_day = false;

        // for (auto& [event_id, tbl_base] : res_user_event->GetMap())
        // {
        //     const auto* tbl = static_cast<const TableUserEvent*>(tbl_base);
        //     if (tbl == nullptr) continue;
        //
        //     EventContext ctx     = base_ctx;
        //     ctx.table_event      = tbl;
        //     ctx.event_info       = user->GetUserEvent().GetInfo(tbl->EventId);
        //
        //     const auto event_type = static_cast<EventType>(tbl->EventType);
        //     auto result = GEventDispatcher().DispatchTimeCheck(event_type, ctx);
        //
        //     if (result == HandleResult::Reset)
        //         notify_change_day = true;
        // }

        return notify_change_day;
    }

    EventContext EventUpdateChecker::BuildBaseContext(
        time_t now, time_t last_check_time,
        const std::shared_ptr<CZoneGroup>& zone_group,
        const std::shared_ptr<CUser>& user)
    {
        EventContext ctx;
        ctx.zone_group      = zone_group;
        ctx.user            = user;
        ctx.now             = now;
        ctx.last_check_time = last_check_time;
        return ctx;
    }

} // namespace GameEvent
