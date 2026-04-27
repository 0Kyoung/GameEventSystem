#include "MissionPassEventHandler.h"

namespace GameEvent
{

    bool MissionPassEventHandler::OnLoginSync(const EventContext& ctx)
    {
        if (ctx.table_event == nullptr || ctx.user == nullptr)
            return false;

        // 이벤트 시간 범위 내인지 확인
        const auto* tbl = ctx.table_event;
        if (tbl->StartCondition == static_cast<uint32_t>(StartCondition::Time) &&
            ctx.now < tbl->start_time)
            return false;

        if (tbl->CloseCondition == static_cast<uint32_t>(CloseCondition::Time) &&
            ctx.now > tbl->close_time)
            return false;

        // TODO: DB에서 미션 데이터 및 패스 수령 정보 로드
        // Process::LoadMissionPassFromDB(zone_group, user)
        return true;
    }

    HandleResult MissionPassEventHandler::OnTimeCheck(const EventContext& ctx)
    {
        if (ctx.table_event == nullptr)
            return HandleResult::None;

        // 이벤트 시작 전환
        auto result = HandleEventStart(ctx);
        if (result != HandleResult::None)
            return result;

        // 이벤트 종료 전환
        result = HandleEventClose(ctx);
        if (result != HandleResult::None)
            return result;

        return HandleResult::None;
    }

    bool MissionPassEventHandler::FillPacketInfo(const EventContext& ctx, void* out_proto) const
    {
        if (out_proto == nullptr || ctx.table_event == nullptr)
            return false;

        // auto* proto = static_cast<MirMobile::_user_event_info*>(out_proto);
        // proto->set_event_id(ctx.table_event->EventId);
        //
        // if (ctx.event_info != nullptr)
        //     proto->set_event_step(ctx.event_info->event_step_);
        //
        // 미션 패스 전용 필드:
        // - 미션 달성 현황 (조건별 카운트)
        // - 패스 보상 수령 단계
        return true;
    }

    bool MissionPassEventHandler::IsActivatedMissionData(
        const EventContext& ctx,
        int32_t& out_event_id,
        int32_t& out_condition_id,
        int32_t progress_type,
        int32_t category_type,
        int32_t subtype) const
    {
        if (ctx.user == nullptr)
            return false;

        // 유저가 진행 중인 미션 패스 이벤트 중
        // 조건(progress_type, category_type, subtype)에 해당하는 미션이 있는지 탐색
        //
        // auto& mission_pass = ctx.user->GetUserEvent().GetMissionPassList();
        // for (auto& [event_id, pass_info] : mission_pass)
        // {
        //     auto* tbl_condition = GET_TABLE_DATA(_table_mission_pass_condition, ...);
        //     if (tbl_condition == nullptr) continue;
        //
        //     if (tbl_condition->ProgressType   == progress_type &&
        //         tbl_condition->CategoryType   == category_type &&
        //         tbl_condition->SubType        == subtype)
        //     {
        //         out_event_id     = event_id;
        //         out_condition_id = tbl_condition->ConditionId;
        //         return true;
        //     }
        // }
        return false;
    }

    int64_t MissionPassEventHandler::IncreaseMissionDataCount(
        const EventContext& ctx,
        int32_t event_id,
        int32_t condition_id,
        int32_t value)
    {
        if (ctx.user == nullptr || ctx.zone_group == nullptr)
            return 0;

        // auto* mission_data = ctx.user->GetUserEvent().GetMissionData(event_id, condition_id);
        // if (mission_data == nullptr) return 0;
        //
        // mission_data->count += value;
        //
        // DB 업데이트 비동기 호출:
        // _call_back_MissionData_Update_Game
        //
        // return mission_data->count;
        return 0;
    }

    void MissionPassEventHandler::ResetMissionPass(const EventContext& ctx) const
    {
        if (ctx.user == nullptr || ctx.table_event == nullptr)
            return;

        // 미션 데이터 및 패스 수령 정보 초기화
        // ctx.user->GetUserEvent().ClearMissionPass(ctx.table_event->EventId);
        //
        // DB 삭제 비동기 호출:
        // _call_back_MissionPass_Refresh_Delete_Game
    }

    bool MissionPassEventHandler::CheckPassReward(
        const EventContext& ctx,
        int32_t event_id,
        int32_t condition_group_id,
        int32_t pass_reward_id) const
    {
        if (ctx.user == nullptr)
            return false;

        // 1. 이벤트 진행 중인지 확인
        // 2. 해당 조건 그룹의 목표 달성 여부 확인
        // 3. 이미 수령한 보상인지 확인 (중복 수령 방지)
        //
        // auto* pass_info = ctx.user->GetUserEvent()
        //                       .GetMissionPassInfo(event_id);
        // if (pass_info == nullptr) return false;
        //
        // return pass_info->IsRewardAvailable(condition_group_id, pass_reward_id);
        return true;
    }

    bool MissionPassEventHandler::ProcessPassReward(
        const EventContext& ctx,
        int32_t event_id,
        int32_t pass_reward_id)
    {
        if (ctx.user == nullptr || ctx.zone_group == nullptr)
            return false;

        // 1. 보상 테이블 조회
        // auto* tbl_reward = GET_TABLE_DATA(_table_mission_pass_reward, pass_reward_id);
        // if (tbl_reward == nullptr) return false;
        //
        // 2. 인벤 공간 확인
        // if (!CONTENTS::ITEM::Process::IsRemainInvenCount(user, reward_list))
        //     return false;
        //
        // 3. DB 저장 후 보상 지급
        // _call_back_MissionPass_Update_Game
        return true;
    }

    // ─── private ────────────────────────────────────────────────────────────

    HandleResult MissionPassEventHandler::HandleEventStart(const EventContext& ctx) const
    {
        const auto* tbl = ctx.table_event;
        if (tbl->StartCondition != static_cast<uint32_t>(StartCondition::Time))
            return HandleResult::None;

        if (!(ctx.last_check_time < tbl->start_time && ctx.now >= tbl->start_time))
            return HandleResult::None;

        auto* event_info = ctx.event_info;
        if (event_info == nullptr)
        {
            // 최초 시작: 이벤트 레코드 생성 + 미션 데이터 초기화
            // Process::UpdateUserEvent(false, false)
            // ResetMissionPass(ctx)
            // SendUserEventChangeState(Start)
            return HandleResult::Notified;
        }

        if (event_info->event_deleted_)
        {
            // 재시작 (삭제 상태에서 복구)
            // Process::UpdateUserEvent(false, true)
            // ResetMissionPass(ctx)
            // SendUserEventChangeState(Start)
            return HandleResult::Notified;
        }

        return HandleResult::None;
    }

    HandleResult MissionPassEventHandler::HandleEventClose(const EventContext& ctx) const
    {
        const auto* tbl = ctx.table_event;
        if (tbl->CloseCondition != static_cast<uint32_t>(CloseCondition::Time))
            return HandleResult::None;

        if (!(ctx.last_check_time < tbl->close_time && ctx.now >= tbl->close_time))
            return HandleResult::None;

        auto* event_info = ctx.event_info;
        if (event_info != nullptr && !event_info->event_deleted_)
        {
            // 이벤트 종료 처리
            // Process::UpdateUserEvent(true, false)
            // ResetMissionPass(ctx)
            // SendUserEventChangeState(Close)
            return HandleResult::Notified;
        }

        return HandleResult::None;
    }

} // namespace GameEvent
