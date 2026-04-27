#pragma once
#include "../Core/IEventHandler.h"

namespace GameEvent
{

    // 사냥 이벤트 핸들러
    //
    // 담당 타입: Hunting, HuntingToExchange
    //
    // 특징:
    //   - user_event 테이블의 start_time / close_time으로 직접 시간 체크
    //     (RepeatCycle이나 SiegeEve와 달리 외부 매니저 불필요)
    //   - 이벤트 종료 시 교환 가능 아이템 수량 갱신 패킷 전송
    //   - 일일 리셋 없음 (event_step은 누적 사냥 수)
    class HuntingEventHandler final : public IEventHandler
    {
    public:
        std::vector<EventType> GetSupportedTypes() const override
        {
            return {
                EventType::Hunting,
                EventType::HuntingToExchange,
            };
        }

        bool OnLoginSync(const EventContext& ctx) override;
        HandleResult OnTimeCheck(const EventContext& ctx) override;
        bool FillPacketInfo(const EventContext& ctx, void* out_proto) const override;

    private:
        // 이벤트 진행 기간 내인지 확인
        bool IsWithinEventPeriod(const EventContext& ctx) const;

        // 이벤트 시작 전환 처리
        HandleResult HandleEventStart(const EventContext& ctx) const;

        // 이벤트 종료 전환 처리
        HandleResult HandleEventClose(const EventContext& ctx) const;

        // 삭제/미등록 유저 데이터 복구
        HandleResult HandleDeletedOrMissing(const EventContext& ctx) const;
    };

} // namespace GameEvent
