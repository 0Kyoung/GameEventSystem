#pragma once
#include "../Core/IEventHandler.h"

namespace GameEvent
{

    // 미션 패스 이벤트 핸들러
    //
    // 담당 타입: MissionPassGrowth, MissionPassChallenge
    //
    // 특징:
    //   - 고정 시작/종료 시간 기반
    //   - 이벤트 시작/종료 시 미션 데이터 리셋 처리 필요
    //   - 미션 달성 조건(progress_type, category_type) 기반 카운트 관리
    //   - 패스 보상 단계별 수령
    class MissionPassEventHandler final : public IEventHandler
    {
    public:
        std::vector<EventType> GetSupportedTypes() const override
        {
            return {
                EventType::MissionPassGrowth,
                EventType::MissionPassChallenge,
            };
        }

        bool OnLoginSync(const EventContext& ctx) override;
        HandleResult OnTimeCheck(const EventContext& ctx) override;
        bool FillPacketInfo(const EventContext& ctx, void* out_proto) const override;

        // 미션 패스 전용 처리
        bool IsActivatedMissionData(const EventContext& ctx,
                                    int32_t& out_event_id,
                                    int32_t& out_condition_id,
                                    int32_t progress_type,
                                    int32_t category_type,
                                    int32_t subtype) const;

        int64_t IncreaseMissionDataCount(const EventContext& ctx,
                                         int32_t event_id,
                                         int32_t condition_id,
                                         int32_t value = 1);

        void ResetMissionPass(const EventContext& ctx) const;

        bool CheckPassReward(const EventContext& ctx,
                             int32_t event_id,
                             int32_t condition_group_id,
                             int32_t pass_reward_id) const;

        bool ProcessPassReward(const EventContext& ctx,
                               int32_t event_id,
                               int32_t pass_reward_id);

    private:
        HandleResult HandleEventStart(const EventContext& ctx) const;
        HandleResult HandleEventClose(const EventContext& ctx) const;
    };

} // namespace GameEvent
