#pragma once
#include "../Core/IEventHandler.h"

namespace GameEvent
{

    // 빙고 이벤트 핸들러
    //
    // 담당 타입: Bingo
    //
    // 특징:
    //   - RepeatCycle 카테고리 (EventRepeatManager 기반 시간 체크)
    //   - 이벤트 시작/종료 시 빙고 보드 데이터 삭제 처리 필요
    //   - 내부 처리: 카드 생성, 번호 추첨, 황금열쇠, 누적 보상
    //
    // 기존 코드 대비:
    //   - 빙고 관련 분기가 Portrait/EscortAgency 블록 안에 혼재했던 것을
    //     독립 클래스로 분리
    class BingoEventHandler final : public IEventHandler
    {
    public:
        std::vector<EventType> GetSupportedTypes() const override
        {
            return { EventType::Bingo };
        }

        bool OnLoginSync(const EventContext& ctx) override;
        HandleResult OnTimeCheck(const EventContext& ctx) override;
        bool FillPacketInfo(const EventContext& ctx, void* out_proto) const override;

        // 빙고 전용 처리 - 패킷 핸들러에서 직접 호출
        bool ProcessCreateBoard(const EventContext& ctx);
        bool ProcessPickNumber(const EventContext& ctx);
        bool ProcessGoldenKeyReward(const EventContext& ctx, uint32_t select_index);
        bool ProcessCumulativeReward(const EventContext& ctx);

    private:
        // 반복 이벤트 매니저에서 현재 주기 정보 조회
        bool GetRepeatEventTimeRange(const EventContext& ctx,
                                     time_t& out_start, time_t& out_end,
                                     time_t& out_period_start, time_t& out_period_end) const;

        // 빙고 보드 삭제 (이벤트 리셋 시 호출)
        void DeleteBingoBoard(const EventContext& ctx) const;

        // 이벤트 종료 후 다음 주기 시작 체크
        HandleResult CheckNextCycleStart(const EventContext& ctx,
                                         time_t event_end, time_t period_end) const;

        // 주기 내 시작/종료/진행 상태 체크
        HandleResult CheckWithinCycle(const EventContext& ctx,
                                      time_t event_start, time_t event_end) const;
    };

} // namespace GameEvent
