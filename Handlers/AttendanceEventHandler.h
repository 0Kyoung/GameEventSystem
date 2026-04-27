#pragma once
#include "../Core/IEventHandler.h"

namespace GameEvent
{

    // 출석 계열 이벤트 핸들러
    //
    // 담당 타입:
    //   RepeatAttendance, SpecialAttendance, CumulativeAttendance,
    //   Attendance, ContinuityAttendance, Attendance14Days,
    //   AttendanceNewAndReturn7Days, AttendanceNewAndReturn14Days
    //
    // 특징:
    //   - 고정 시작/종료 시간 기반 (FixedTime 카테고리)
    //   - 매일 리셋 체크 필요 (RepeatAttendance는 MaxStep 도달 시 리셋)
    //   - 신규/복귀 유저 필터링 (7일, 14일 타입)
    class AttendanceEventHandler final : public IEventHandler
    {
    public:
        std::vector<EventType> GetSupportedTypes() const override
        {
            return {
                EventType::RepeatAttendance,
                EventType::SpecialAttendance,
                EventType::CumulativeAttendance,
                EventType::Attendance,
                EventType::ContinuityAttendance,
                EventType::Attendance14Days,
                EventType::AttendanceNewAndReturn7Days,
                EventType::AttendanceNewAndReturn14Days,
            };
        }

        bool OnLoginSync(const EventContext& ctx) override;
        HandleResult OnTimeCheck(const EventContext& ctx) override;
        bool FillPacketInfo(const EventContext& ctx, void* out_proto) const override;

    private:
        // 이벤트 시작 조건 충족 여부
        bool IsEventStarted(const EventContext& ctx) const;
        // 이벤트 종료 조건 충족 여부
        bool IsEventClosed(const EventContext& ctx) const;
        // 신규/복귀 유저 타입의 유저 자격 검사
        bool CheckNewOrReturnUser(const EventContext& ctx) const;
        // 일일 리셋 처리
        HandleResult ProcessDailyReset(const EventContext& ctx) const;
    };

} // namespace GameEvent
