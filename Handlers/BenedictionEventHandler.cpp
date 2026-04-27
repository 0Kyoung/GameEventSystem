#include "BenedictionEventHandler.h"

namespace GameEvent
{

    bool BenedictionEventHandler::OnLoginSync(const EventContext& ctx)
    {
        if (!IsEventActive(ctx))
            return false;

        // TODO: DB에서 복덕 데이터 로드
        // call_back_Benediction_Event_Get_Game
        return true;
    }

    HandleResult BenedictionEventHandler::OnTimeCheck(const EventContext& ctx)
    {
        // 복덕 이벤트는 시간 체크에서 별도 처리 없음
        // (클라이언트 요청 기반으로만 동작)
        return HandleResult::None;
    }

    bool BenedictionEventHandler::FillPacketInfo(const EventContext& ctx, void* out_proto) const
    {
        if (out_proto == nullptr || !IsEventActive(ctx))
            return false;

        // auto* proto = static_cast<MirMobile::_user_event_info*>(out_proto);
        // proto->set_event_id(ctx.table_event->EventId);
        //
        // 복덕 전용 필드:
        // - 향 타입별 남은 횟수
        // - 획득한 복덕 아이템 목록 및 등급
        return true;
    }

    bool BenedictionEventHandler::RequestBenedictionData(const EventContext& ctx) const
    {
        if (ctx.user == nullptr || !IsEventActive(ctx))
            return false;

        // 복덕 이벤트 현황 조회 후 패킷 전송
        // SendBenedictionEventData(user, event_info)
        return true;
    }

    bool BenedictionEventHandler::SelectBenediction(
        const EventContext& ctx, int32_t benediction_tid)
    {
        if (ctx.user == nullptr || ctx.zone_group == nullptr)
            return false;

        if (!IsEventActive(ctx))
            return false;

        // 1. 복덕 테이블 조회
        // auto* tbl_benediction = GET_TABLE_DATA(_table_user_event_benediction, benediction_tid);
        // if (tbl_benediction == nullptr) return false;

        // 2. 향 아이템 소지 여부 확인
        // if (!user->GetInven().HasItem(tbl_benediction->IncenseItemId)) return false;

        // 3. 아이템 소모 및 복덕 처리
        return ProcessItemConsume(ctx, benediction_tid);
    }

    void BenedictionEventHandler::AddGrade(
        const std::shared_ptr<CUser>& user,
        uint32_t main_type, uint32_t sub_type, uint32_t grade)
    {
        if (user == nullptr)
            return;

        // auto* benediction_info = user->GetUserEvent()
        //                              .GetBenedictionInfo(main_type, sub_type);
        // if (benediction_info == nullptr) return;
        //
        // benediction_info->AddGrade(grade);
        // ArrangeGrades(user); // 등급 정렬 갱신
    }

    void BenedictionEventHandler::ArrangeGrades(const std::shared_ptr<CUser>& user)
    {
        if (user == nullptr)
            return;

        // 복덕 아이템 등급을 내림차순 정렬
        // (높은 등급이 앞에 오도록)
        //
        // auto& benediction_list = user->GetUserEvent().GetBenedictionList();
        // std::sort(benediction_list.begin(), benediction_list.end(),
        //     [](const auto& a, const auto& b) { return a.grade > b.grade; });
    }

    // ─── private ────────────────────────────────────────────────────────────

    bool BenedictionEventHandler::IsEventActive(const EventContext& ctx) const
    {
        if (ctx.table_event == nullptr)
            return false;

        const auto* tbl = ctx.table_event;
        if (tbl->StartCondition == static_cast<uint32_t>(StartCondition::Time) &&
            ctx.now < tbl->start_time)
            return false;

        if (tbl->CloseCondition == static_cast<uint32_t>(CloseCondition::Time) &&
            ctx.now > tbl->close_time)
            return false;

        return true;
    }

    bool BenedictionEventHandler::ProcessItemConsume(
        const EventContext& ctx, int32_t benediction_tid)
    {
        // 아이템 소모 DB 업데이트 비동기 처리
        // 콜백: call_back_Benediction_Event_Update_Game
        //       → 성공 시 보상 지급 및 클라이언트 패킷 전송
        return true;
    }

} // namespace GameEvent
