#include "BingoEventHandler.h"

namespace GameEvent
{

    bool BingoEventHandler::OnLoginSync(const EventContext& ctx)
    {
        time_t event_start{}, event_end{}, period_start{}, period_end{};
        if (!GetRepeatEventTimeRange(ctx, event_start, event_end, period_start, period_end))
            return false;

        // 현재 시각이 이벤트 기간 내인지 확인
        if (ctx.now < event_start || ctx.now > event_end)
            return false;

        // TODO: 빙고 보드 데이터 로드
        return true;
    }

    HandleResult BingoEventHandler::OnTimeCheck(const EventContext& ctx)
    {
        time_t event_start{}, event_end{}, period_start{}, period_end{};
        if (!GetRepeatEventTimeRange(ctx, event_start, event_end, period_start, period_end))
            return HandleResult::Continue;

        // Case 1: 이전 주기 이벤트가 완전히 종료된 후 새 주기 시작 체크
        if (ctx.last_check_time > event_end && ctx.now > event_end)
            return CheckNextCycleStart(ctx, event_end, period_end);

        // Case 2: 현재 주기 내 시작/종료/진행 체크
        if (ctx.now >= period_start && ctx.now < period_end)
            return CheckWithinCycle(ctx, event_start, event_end);

        return HandleResult::Continue;
    }

    bool BingoEventHandler::FillPacketInfo(const EventContext& ctx, void* out_proto) const
    {
        if (out_proto == nullptr || ctx.event_info == nullptr)
            return false;

        // auto* proto = static_cast<MirMobile::_user_event_info*>(out_proto);
        // proto->set_event_id(ctx.table_event->EventId);
        // 빙고 전용 필드 채우기 (번호 목록, 빙고 달성 여부 등)
        return true;
    }

    bool BingoEventHandler::ProcessCreateBoard(const EventContext& ctx)
    {
        // 빙고 보드 생성 로직
        // 1. 이벤트 시간 확인
        // 2. 기존 보드 존재 여부 확인
        // 3. DB 저장 (_call_back_Event_Bingo_Card_Create_Update_Game)
        return true;
    }

    bool BingoEventHandler::ProcessPickNumber(const EventContext& ctx)
    {
        // 번호 추첨 로직
        // 1. 이벤트 시간 확인
        // 2. 빙고 달성 체크
        // 3. 보상 지급 DB 저장 (_call_back_Event_Bingo_Pick_Number_Reward_Update_Game)
        return true;
    }

    bool BingoEventHandler::ProcessGoldenKeyReward(const EventContext& ctx, uint32_t select_index)
    {
        // 황금열쇠 보상 로직
        // 1. select_index 유효성 검사
        // 2. 재화 차감 처리
        // 3. DB 저장 (_call_back_Event_Bingo_GoldenKey_Reward_Update_Game)
        return true;
    }

    bool BingoEventHandler::ProcessCumulativeReward(const EventContext& ctx)
    {
        // 줄 빙고 누적 보상 로직
        // 1. 달성한 빙고 줄 수 계산
        // 2. 이미 수령한 보상 단계 체크
        // 3. DB 저장 (_call_back_Event_Line_Bingo_Cumulative_Reward_Update_Game)
        return true;
    }

    // ─── private ────────────────────────────────────────────────────────────

    bool BingoEventHandler::GetRepeatEventTimeRange(
        const EventContext& ctx,
        time_t& out_start, time_t& out_end,
        time_t& out_period_start, time_t& out_period_end) const
    {
        if (ctx.zone_group == nullptr || ctx.table_event == nullptr)
            return false;

        // auto repeat_info = ctx.zone_group->GetEventRepeatManager()
        //                        .GetInfo(ctx.table_event->EventContentsId);
        // if (repeat_info == nullptr) return false;
        //
        // out_start        = UTILITY::Time::UtcToLocalTime(repeat_info->GetEventStartTime());
        // out_end          = repeat_info->GetEventEndTime();
        // out_period_start = repeat_info->GetPeriodStartTime();
        // out_period_end   = repeat_info->GetPeriodEndTime();

        return true;
    }

    void BingoEventHandler::DeleteBingoBoard(const EventContext& ctx) const
    {
        if (ctx.user == nullptr || ctx.table_event == nullptr)
            return;

        // Process::DeleteBingoEventInfo(worker_index, user, event_id) 호출
        // LOG_INFO: DeleteBingoBoard
    }

    HandleResult BingoEventHandler::CheckNextCycleStart(
        const EventContext& ctx, time_t event_end, time_t period_end) const
    {
        // period_end 기준으로 초 단위 정규화
        struct tm t;
        // UTILITY::Time::LocalTime(&t, &period_end);
        t.tm_sec = 0;
        time_t normalized_period_end = mktime(&t);

        if (ctx.last_check_time < normalized_period_end && ctx.now >= normalized_period_end)
        {
            // UpdateUserEventStartReset 호출
            // SendUserEventChangeState(Start) 호출
            DeleteBingoBoard(ctx);
            return HandleResult::Notified;
        }

        return HandleResult::Continue;
    }

    HandleResult BingoEventHandler::CheckWithinCycle(
        const EventContext& ctx, time_t event_start, time_t event_end) const
    {
        // 이벤트 시작 전환
        if (ctx.last_check_time < event_start && ctx.now >= event_start)
        {
            auto* event_info = ctx.event_info;
            if (event_info != nullptr &&
                event_info->last_update_time_ >= event_start)
                return HandleResult::Continue;

            // UpdateUserEventStartReset 호출
            // SendUserEventChangeState(Start) 호출
            DeleteBingoBoard(ctx);
            return HandleResult::Notified;
        }

        // 이벤트 종료 전환
        if (ctx.last_check_time < event_end && ctx.now >= event_end)
        {
            auto* event_info = ctx.event_info;
            if (event_info != nullptr && !event_info->event_deleted_)
            {
                // UpdateUserEvent(delete=true) 호출
                // SendUserEventChangeState(Close) 호출
                DeleteBingoBoard(ctx);
                return HandleResult::Notified;
            }
        }

        // 이벤트 진행 중 - 삭제된 상태면 재시작
        auto* event_info = ctx.event_info;
        if (event_info == nullptr || event_info->event_deleted_)
        {
            // UpdateUserEventStartReset 호출
            DeleteBingoBoard(ctx);
            return HandleResult::Notified;
        }

        return HandleResult::None;
    }

} // namespace GameEvent
