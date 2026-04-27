#include "SiegeEveEventHandler.h"

namespace GameEvent
{

    bool SiegeEveEventHandler::OnLoginSync(const EventContext& ctx)
    {
        if (ctx.zone_group == nullptr || ctx.user == nullptr)
            return false;

        time_t event_start{}, event_end{};
        if (!GetSiegeEventTimeRange(ctx, event_start, event_end))
            return false;

        // 이벤트 기간 내인지 확인
        if (ctx.now < event_start || ctx.now > event_end)
            return false;

        // TODO: 공성 전야 응원 데이터 로드
        // Process::call_back_SiegeEve_EventInfo_Load_Game
        return true;
    }

    HandleResult SiegeEveEventHandler::OnTimeCheck(const EventContext& ctx)
    {
        if (ctx.zone_group == nullptr)
            return HandleResult::Continue;

        // 인터서버(Inter-game server)는 공성 전야 이벤트 제외
        // if (SERVER::CServerConfig::GetSingleton()
        //         .GetGameServerInfo().inter_game_server)
        //     return HandleResult::Continue;

        time_t event_start{}, event_end{};
        if (!GetSiegeEventTimeRange(ctx, event_start, event_end))
            return HandleResult::Continue;

        // 이벤트 시작 전환: 클라이언트에 시작 알림
        if (ctx.last_check_time < event_start && ctx.now >= event_start)
        {
            // SendUserEventChangeState(Start)
        }
        // 이벤트 종료 전환: 클라이언트에 종료 알림
        else if (ctx.last_check_time < event_end && ctx.now >= event_end)
        {
            // SendUserEventChangeState(Close)
        }

        auto* siege_event_info = ctx.event_info;

        // 유저 데이터 없음 (미참여 상태) → 건너뜀
        if (siege_event_info == nullptr)
            return HandleResult::Continue;

        // 신규 입장 데이터 (last_update_time_ == 0) → 건너뜀
        if (siege_event_info->last_update_time_ == 0)
            return HandleResult::Continue;

        // 응원 횟수가 0이고 이벤트가 유효 상태 → 건너뜀
        if (siege_event_info->event_step_ == 0 &&
            !siege_event_info->event_deleted_)
            return HandleResult::Continue;

        // 이벤트 종료 후 데이터 리셋
        if (ctx.now > event_end)
        {
            ResetUserSiegeData(ctx);
            return HandleResult::None;
        }

        return HandleResult::None;
    }

    bool SiegeEveEventHandler::FillPacketInfo(const EventContext& ctx, void* out_proto) const
    {
        if (out_proto == nullptr || ctx.zone_group == nullptr)
            return false;

        time_t event_start{}, event_end{};
        if (!GetSiegeEventTimeRange(ctx, event_start, event_end))
            return false;

        // 이벤트 시간 범위 밖 → 패킷에 포함하지 않음
        if (ctx.now < event_start || ctx.now > event_end)
            return false;

        // auto* proto = static_cast<MirMobile::_user_event_info*>(out_proto);
        // proto->set_event_id(ctx.table_event->EventId);
        //
        // 공성 전야 전용 필드:
        // - 응원 팀 (defense / siege)
        // - 현재 응원 횟수
        // - 총 응원 횟수 (방어/공격 합산)
        return true;
    }

    bool SiegeEveEventHandler::ProcessCheeringTeamSelect(
        const EventContext& ctx,
        int32_t cheering_team,
        int32_t cheering_count,
        int32_t total_defense_count,
        int32_t total_siege_count)
    {
        if (ctx.user == nullptr || ctx.zone_group == nullptr)
            return false;

        // 1. 이벤트 진행 중 확인
        time_t event_start{}, event_end{};
        if (!GetSiegeEventTimeRange(ctx, event_start, event_end))
            return false;

        if (ctx.now < event_start || ctx.now > event_end)
            return false;

        // 2. 공성 전야 보상 테이블 조회
        // auto* tbl_siegeeve = GET_TABLE_DATA(_table_user_event_siegeeve,
        //                          ctx.table_event->EventId, cheering_team);
        // if (tbl_siegeeve == nullptr) return false;

        // 3. 인벤 공간 확인
        // reward_item = tbl_siegeeve->Reward_Item * cheering_count
        // if (!CONTENTS::ITEM::Process::IsRemainInvenCount(user, reward_list))
        //     return false;

        // 4. 응원 횟수 DB 저장 및 보상 지급
        // _call_back_Event_Reward_Update_Game_Siege_Eve 호출
        //
        // 5. 월드 서버에 응원 정보 전달
        // user->SendPacketToWorld(GAME_WORLD_SIEGE_EVE_EVENT_SELECT_CHEERING_TEAM)

        return true;
    }

    // ─── private ────────────────────────────────────────────────────────────

    bool SiegeEveEventHandler::GetSiegeEventTimeRange(
        const EventContext& ctx,
        time_t& out_start,
        time_t& out_end) const
    {
        if (ctx.zone_group == nullptr)
            return false;

        // auto siege = ctx.zone_group->GetZoneGroupSiegeManager()
        //                  .GetSiege(Siege::kBicheonSiegeTableTid);
        // if (siege == nullptr) return false;
        //
        // 공성 시작 시각 (UserEvent501)
        // auto& start_info = siege->GetSiegeEventTimeInfo(Siege::SiegeEventType::UserEvent501);
        // out_start = std::chrono::system_clock::to_time_t(start_info.start_time);
        //
        // 이벤트 결과 종료 시각 (UserEvent502)
        // auto& end_info = siege->GetSiegeEventTimeInfo(Siege::SiegeEventType::UserEvent502);
        // out_end = std::chrono::system_clock::to_time_t(end_info.end_time);

        return true;
    }

    void SiegeEveEventHandler::ResetUserSiegeData(const EventContext& ctx) const
    {
        if (ctx.user == nullptr || ctx.event_info == nullptr)
            return;

        // UpdateUserEvent(reset=true) 호출
        // ctx.event_info->Reset()
        // ctx.user->SetCheeringCount(0)
    }

} // namespace GameEvent
