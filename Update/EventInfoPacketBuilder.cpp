#include "EventInfoPacketBuilder.h"

namespace GameEvent
{

    bool EventInfoPacketBuilder::Send(
        const std::shared_ptr<CZoneGroup>& zone_group,
        const std::shared_ptr<CUser>& user)
    {
        if (zone_group == nullptr || user == nullptr)
            return false;

        // auto* res_user_event = GET_RESOURCE(_res_user_event);
        // if (res_user_event == nullptr) return false;

        const time_t now = std::time(nullptr);

        // MirMobile::GAME_CLIENT_USER_EVENT_INFO smsg;
        // SetDailyAccessField(user, now, &smsg);
        //
        // auto* packet_event_info = smsg.mutable_event_info_list();

        // for (auto& [event_id, tbl_base] : res_user_event->GetMap())
        // {
        //     const auto* tbl = static_cast<const TableUserEvent*>(tbl_base);
        //     if (tbl == nullptr) continue;
        //
        //     EventContext ctx;
        //     ctx.zone_group  = zone_group;
        //     ctx.user        = user;
        //     ctx.table_event = tbl;
        //     ctx.now         = now;
        //     ctx.event_info  = user->GetUserEvent().GetInfo(tbl->EventId);
        //
        //     // 패킷 포함 여부 필터링
        //     if (!ShouldIncludeInPacket(ctx, zone_group)) continue;
        //
        //     // 이벤트 정보 proto 슬롯 할당
        //     auto* event_info_proto = packet_event_info->Add();
        //
        //     // ─────────────────────────────────────────────────────────────
        //     // 핵심 변경점:
        //     //   기존 SendUserEventInfo의 613줄 switch-case 대신
        //     //   각 핸들러의 FillPacketInfo()에 위임
        //     // ─────────────────────────────────────────────────────────────
        //     const auto event_type = static_cast<EventType>(tbl->EventType);
        //     GEventDispatcher().DispatchFillPacket(event_type, ctx, event_info_proto);
        // }

        // user->SendPacketToClient(Network::Protocol::GAME_CLIENT_USER_EVENT_INFO, &smsg);

        (void)now; // 샘플용
        return true;
    }

    bool EventInfoPacketBuilder::ShouldIncludeInPacket(
        const EventContext& ctx,
        [[maybe_unused]] const std::shared_ptr<CZoneGroup>& zone_group)
    {
        if (ctx.table_event == nullptr || ctx.user == nullptr)
            return false;

        const auto* tbl = ctx.table_event;
        const auto  event_type = static_cast<EventType>(tbl->EventType);

        // 반복 이벤트 타입은 RepeatManager 기반 시간 체크
        const bool is_repeat_type =
            event_type == EventType::Portrait      ||
            event_type == EventType::EscortAgency  ||
            event_type == EventType::Bingo;

        if (is_repeat_type)
        {
            // auto repeat_info = zone_group->GetEventRepeatManager()
            //                        .GetInfo(tbl->EventContentsId);
            // if (repeat_info == nullptr) return false;
            // if (now > repeat_info->GetEventEndTime()) return false;
            // if (now < repeat_info->GetEventStartTime()) return false;
            return true;
        }

        // 공성 전야는 SiegeManager 기반 시간 체크
        if (event_type == EventType::SiegeEve)
        {
            // auto siege = zone_group->GetZoneGroupSiegeManager()
            //                  .GetSiege(Siege::kBicheonSiegeTableTid);
            // if (siege == nullptr) return false;
            // ... 시간 범위 체크 ...
            return true;
        }

        // 일반 이벤트: start_time / close_time 기반 체크
        if (tbl->StartCondition == static_cast<uint32_t>(StartCondition::Time) &&
            ctx.now < tbl->start_time)
            return false;

        if (tbl->CloseCondition == static_cast<uint32_t>(CloseCondition::Time) &&
            ctx.now > tbl->close_time)
            return false;

        // GM 시간 조건 체크
        if ((tbl->StartCondition == static_cast<uint32_t>(StartCondition::TimeGM) &&
             tbl->start_time == 0) ||
            (tbl->StartCondition == static_cast<uint32_t>(StartCondition::TimeGM) &&
             tbl->start_time > ctx.now))
            return false;

        if ((tbl->CloseCondition == static_cast<uint32_t>(CloseCondition::TimeGM) &&
             tbl->close_time == 0) ||
            (tbl->CloseCondition == static_cast<uint32_t>(CloseCondition::TimeGM) &&
             tbl->close_time < ctx.now))
            return false;

        // 신규/복귀 유저 이벤트 필터
        if (event_type == EventType::AttendanceNewAndReturn7Days ||
            event_type == EventType::AttendanceNewAndReturn14Days)
        {
            // if (!user->IsNewCharacter(tbl->new_period_start_time) &&
            //     !user->IsReturningUser(tbl->return_period_start_time))
            //     return false;
        }

        return true;
    }

    void EventInfoPacketBuilder::SetDailyAccessField(
        const std::shared_ptr<CUser>& user,
        time_t now,
        [[maybe_unused]] void* out_smsg)
    {
        if (user == nullptr)
            return;

        // reset_hour 기준 오늘 리셋 시각 계산
        // auto reset_hour = CONTENTS::ETC::Process::GetSchdulerCycleHour(
        //                       Game::SCHEDULE_RESET_TIME_TYPE::USER_EVENT);
        // bool is_first_access = UTILITY::Time::IsReset_Day(
        //                            now, user->GetLastLoginTime(), reset_hour);
        //
        // auto* smsg = static_cast<MirMobile::GAME_CLIENT_USER_EVENT_INFO*>(out_smsg);
        // smsg->set_daily_first_access(is_first_access ? 1 : 0);

        (void)now;
    }

} // namespace GameEvent
