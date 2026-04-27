#pragma once
#include "../Core/IEventHandler.h"

namespace GameEvent
{

    // 공성 전야 이벤트 핸들러
    //
    // 담당 타입: SiegeEve
    //
    // 특징:
    //   - ZoneGroupSiegeManager에서 시간 정보를 가져오는 독립적인 시간 체크 방식
    //   - 응원 팀 선택 / 응원 횟수 누적 / 보상 지급
    //   - 이벤트 종료 후 리셋(응원 횟수 초기화)
    class SiegeEveEventHandler final : public IEventHandler
    {
    public:
        std::vector<EventType> GetSupportedTypes() const override
        {
            return { EventType::SiegeEve };
        }

        bool OnLoginSync(const EventContext& ctx) override;
        HandleResult OnTimeCheck(const EventContext& ctx) override;
        bool FillPacketInfo(const EventContext& ctx, void* out_proto) const override;

        // 공성 전야 전용 처리
        bool ProcessCheeringTeamSelect(const EventContext& ctx,
                                       int32_t cheering_team,
                                       int32_t cheering_count,
                                       int32_t total_defense_count,
                                       int32_t total_siege_count);

    private:
        // 공성 시간 정보 조회 (SiegeEventType::UserEvent501, 502)
        bool GetSiegeEventTimeRange(const EventContext& ctx,
                                    time_t& out_start, time_t& out_end) const;

        // 이벤트 종료 후 유저 데이터 리셋
        void ResetUserSiegeData(const EventContext& ctx) const;
    };

} // namespace GameEvent
