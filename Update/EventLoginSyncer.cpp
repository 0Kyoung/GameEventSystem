#include "EventLoginSyncer.h"

namespace GameEvent
{

    void EventLoginSyncer::Sync(
        const std::shared_ptr<CZoneGroup>& zone_group,
        const std::shared_ptr<CUser>& user)
    {
        if (zone_group == nullptr || user == nullptr)
            return;

        // auto* res_user_event = GET_RESOURCE(_res_user_event);
        // if (res_user_event == nullptr) return;

        const time_t now = std::time(nullptr);

        // for (auto& [event_id, tbl_base] : res_user_event->GetMap())
        // {
        //     const auto* tbl = static_cast<const TableUserEvent*>(tbl_base);
        //     if (tbl == nullptr) continue;
        //
        //     // 월드 타입 필터
        //     if (!CheckWorldType(user, tbl)) continue;
        //
        //     auto ctx = BuildContext(zone_group, user, tbl);
        //
        //     // ─────────────────────────────────────────────────────────────
        //     // 핵심 변경점:
        //     //   기존 CheckEventLoginData의 305줄 if-else 체인 대신
        //     //   각 핸들러의 OnLoginSync()에 위임
        //     // ─────────────────────────────────────────────────────────────
        //     const auto event_type = static_cast<EventType>(tbl->EventType);
        //     GEventDispatcher().DispatchLoginSync(event_type, ctx);
        // }

        (void)now; // 샘플용
    }

    EventContext EventLoginSyncer::BuildContext(
        const std::shared_ptr<CZoneGroup>& zone_group,
        const std::shared_ptr<CUser>& user,
        const TableUserEvent* table_event)
    {
        EventContext ctx;
        ctx.zone_group  = zone_group;
        ctx.user        = user;
        ctx.table_event = table_event;
        ctx.now         = std::time(nullptr);
        ctx.event_info  = user->GetUserEvent().GetInfo(table_event->EventId);
        return ctx;
    }

} // namespace GameEvent
