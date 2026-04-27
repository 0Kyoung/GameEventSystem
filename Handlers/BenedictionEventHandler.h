#pragma once
#include "../Core/IEventHandler.h"

namespace GameEvent
{

    // 복덕 이벤트 핸들러
    //
    // 담당 타입: Benediction
    //
    // 특징:
    //   - 향 아이템을 소모하여 등급별 복덕 아이템 획득
    //   - 향 타입(IncenseType) 6종 관리
    //   - 아이템 사용 콜백(ItemUse_Update_Game) 기반 비동기 처리
    //   - 등급 누적 및 정렬 로직 포함 (AddBenedictionEventGrade, Arrange)
    class BenedictionEventHandler final : public IEventHandler
    {
    public:
        std::vector<EventType> GetSupportedTypes() const override
        {
            return { EventType::Benediction };
        }

        bool OnLoginSync(const EventContext& ctx) override;
        HandleResult OnTimeCheck(const EventContext& ctx) override;
        bool FillPacketInfo(const EventContext& ctx, void* out_proto) const override;

        // 복덕 전용 처리
        bool RequestBenedictionData(const EventContext& ctx) const;
        bool SelectBenediction(const EventContext& ctx, int32_t benediction_tid);

        // 복덕 등급 추가 및 정렬
        void AddGrade(const std::shared_ptr<CUser>& user,
                      uint32_t main_type, uint32_t sub_type, uint32_t grade);
        void ArrangeGrades(const std::shared_ptr<CUser>& user);

    private:
        // 복덕 이벤트 시간 유효성 확인 (FixedTime 방식)
        bool IsEventActive(const EventContext& ctx) const;

        // 아이템 소모 처리 후 DB 업데이트 콜백 등록
        bool ProcessItemConsume(const EventContext& ctx, int32_t benediction_tid);
    };

} // namespace GameEvent
