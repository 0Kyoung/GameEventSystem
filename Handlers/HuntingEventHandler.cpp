#include "HuntingEventHandler.h"

namespace GameEvent
{

    bool HuntingEventHandler::OnLoginSync(const EventContext& ctx)
    {
        if (ctx.table_event == nullptr)
            return false;

        if (!IsWithinEventPeriod(ctx))
            return false;

        // 삭제 상태이면 재초기화
        if (ctx.event_info != nullptr && ctx.event_info->event_deleted_)
        {
            // Process::UpdateUserEventStartReset(start_time)
        }

        return true;
    }

    HandleResult HuntingEventHandler::OnTimeCheck(const EventContext& ctx)
    {
        if (ctx.table_event == nullptr)
            return HandleResult::None;

        if (!IsWithinEventPeriod(ctx))
            return HandleResult::None;

        // 시작 전환
        auto result = HandleEventStart(ctx);
        if (result != HandleResult::None)
            return result;

        // 종료 전환
        result = HandleEventClose(ctx);
        if (result != HandleResult::None)
            return result;

        // 삭제/미등록 복구
        return HandleDeletedOrMissing(ctx);
    }

    bool HuntingEventHandler::FillPacketInfo(const EventContext& ctx, void* out_proto) const
    {
        if (out_proto == nullptr || ctx.table_event == nullptr)
            return false;

        if (!IsWithinEventPeriod(ctx))
            return false;

        // auto* proto = static_cast<MirMobile::_user_event_info*>(out_proto);
        // proto->set_event_id(ctx.table_event->EventId);
        //
        // 사냥 이벤트 전용 필드:
        // - 누적 사냥 수 (event_step_)
        // - 교환 가능 아이템 목록 (HuntingToExchange 타입만)
        return true;
    }

    // ─── private ────────────────────────────────────────────────────────────

    bool HuntingEventHandler::IsWithinEventPeriod(const EventContext& ctx) const
    {
        const auto* tbl = ctx.table_event;
        return ctx.now >= tbl->start_time && ctx.now <= tbl->close_time;
    }

    HandleResult HuntingEventHandler::HandleEventStart(const EventContext& ctx) const
    {
        const auto* tbl = ctx.table_event;
        if (!(ctx.last_check_time < tbl->start_time && ctx.now >= tbl->start_time))
            return HandleResult::None;

        // Process::UpdateUserEventStartReset(start_time)
        // SendUserEventChangeState(Start)
        return HandleResult::Notified;
    }

    HandleResult HuntingEventHandler::HandleEventClose(const EventContext& ctx) const
    {
        const auto* tbl = ctx.table_event;
        if (!(ctx.last_check_time < tbl->close_time && ctx.now >= tbl->close_time))
            return HandleResult::None;

        // SendUserEventChangeState(Close)
        // Process::UpdateUserEvent(delete=true)
        return HandleResult::Notified;
    }

    HandleResult HuntingEventHandler::HandleDeletedOrMissing(const EventContext& ctx) const
    {
        const auto* tbl = ctx.table_event;
        auto* event_info = ctx.event_info;

        if (event_info == nullptr)
        {
            // 미등록 유저: 이벤트 데이터 생성
            // Process::UpdateUserEventStartReset(start_time)
            return HandleResult::Continue;
        }

        if (event_info->event_deleted_)
        {
            // 삭제 상태에서 복구
            // Process::UpdateUserEventStartReset(start_time)
            return HandleResult::Continue;
        }

        return HandleResult::None;
    }

} // namespace GameEvent
