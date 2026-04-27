#include "AttendanceEventHandler.h"
// #include "../Core/EventDispatcher.h"
// #include <실제 프로젝트 헤더들>

namespace GameEvent
{

    bool AttendanceEventHandler::OnLoginSync(const EventContext& ctx)
    {
        if (ctx.table_event == nullptr)
            return false;

        // 신규/복귀 유저 타입은 자격 검사 선행
        const auto type = static_cast<EventType>(ctx.table_event->EventType);
        if (type == EventType::AttendanceNewAndReturn7Days ||
            type == EventType::AttendanceNewAndReturn14Days)
        {
            if (!CheckNewOrReturnUser(ctx))
                return false;
        }

        // 이벤트 시간 범위 내인지 확인 후 유저 이벤트 데이터 동기화
        if (!IsEventStarted(ctx) || IsEventClosed(ctx))
            return false;

        // TODO: DB에서 유저 출석 데이터 로드 및 메모리 반영
        return true;
    }

    HandleResult AttendanceEventHandler::OnTimeCheck(const EventContext& ctx)
    {
        if (ctx.table_event == nullptr)
            return HandleResult::None;

        // 이벤트 시작 전환
        if (IsEventStarted(ctx))
        {
            auto* event_info = ctx.event_info;
            if (event_info != nullptr &&
                (event_info->event_deleted_ || event_info->event_step_ > 0))
            {
                // 이미 진행된 이력이 있으면 start_time 이후만 리셋
                if (event_info->last_update_time_ > ctx.table_event->start_time &&
                    event_info->event_step_ > 0)
                    return HandleResult::Continue;

                // UpdateUserEvent(reset=true) 호출
                // SendUserEventChangeState(Start) 호출
                return HandleResult::Notified;
            }
        }

        // 이벤트 종료 전환
        if (IsEventClosed(ctx))
        {
            auto* event_info = ctx.event_info;
            if (event_info != nullptr && !event_info->event_deleted_)
            {
                // UpdateUserEvent(delete=true) 호출
                // SendUserEventChangeState(Close) 호출
                return HandleResult::Notified;
            }
        }

        // RepeatAttendance: MaxStep 도달 시 리셋
        const auto type = static_cast<EventType>(ctx.table_event->EventType);
        if (type == EventType::RepeatAttendance)
        {
            auto* event_info = ctx.event_info;
            if (event_info != nullptr &&
                event_info->event_step_ == ctx.table_event->MaxStep &&
                ctx.reset_time <= ctx.now)
            {
                // UpdateUserEvent(reset=true) 호출
            }
        }

        return ProcessDailyReset(ctx);
    }

    bool AttendanceEventHandler::FillPacketInfo(const EventContext& ctx, void* out_proto) const
    {
        if (ctx.table_event == nullptr || out_proto == nullptr)
            return false;

        // auto* event_info_proto = static_cast<MirMobile::_user_event_info*>(out_proto);
        // event_info_proto->set_event_id(ctx.table_event->EventId);
        // event_info_proto->set_event_step(ctx.event_info ? ctx.event_info->event_step_ : 0);
        // ... 등 패킷 필드 채우기

        return true;
    }

    // ─── private ────────────────────────────────────────────────────────────

    bool AttendanceEventHandler::IsEventStarted(const EventContext& ctx) const
    {
        const auto* tbl = ctx.table_event;
        return tbl->StartCondition == static_cast<uint32_t>(StartCondition::Time)
            && ctx.last_check_time < tbl->start_time
            && ctx.now >= tbl->start_time;
    }

    bool AttendanceEventHandler::IsEventClosed(const EventContext& ctx) const
    {
        const auto* tbl = ctx.table_event;
        return tbl->CloseCondition == static_cast<uint32_t>(CloseCondition::Time)
            && ctx.last_check_time < tbl->close_time
            && ctx.now >= tbl->close_time;
    }

    bool AttendanceEventHandler::CheckNewOrReturnUser(const EventContext& ctx) const
    {
        if (ctx.user == nullptr || ctx.table_event == nullptr)
            return false;

        // return ctx.user->IsNewCharacter(ctx.table_event->new_period_start_time)
        //     || ctx.user->IsReturningUser(ctx.table_event->return_period_start_time);
        return true; // 샘플용
    }

    HandleResult AttendanceEventHandler::ProcessDailyReset(const EventContext& ctx) const
    {
        auto* event_info = ctx.event_info;
        if (event_info == nullptr)
            return HandleResult::None;

        const auto type = static_cast<EventType>(ctx.table_event->EventType);
        const bool is_attendance_type =
            type == EventType::RepeatAttendance        ||
            type == EventType::SpecialAttendance       ||
            type == EventType::CumulativeAttendance    ||
            type == EventType::Attendance              ||
            type == EventType::ContinuityAttendance    ||
            type == EventType::Attendance14Days        ||
            type == EventType::AttendanceNewAndReturn7Days  ||
            type == EventType::AttendanceNewAndReturn14Days;

        if (is_attendance_type &&
            !event_info->event_deleted_ &&           // 삭제된 이벤트는 리셋 대상 아님
            event_info->event_step_ < ctx.table_event->MaxStep &&
            ctx.reset_time > 0 &&                    // reset_time 미설정 방어
            ctx.reset_time <= ctx.now)
        {
            return HandleResult::Reset;
        }

        return HandleResult::None;
    }

} // namespace GameEvent
