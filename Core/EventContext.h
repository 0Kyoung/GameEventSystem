#pragma once
#include <memory>
#include <ctime>
#include <chrono>
#include "GameTypes.h"

namespace GameEvent
{

    // 이벤트 핸들러에 전달되는 공통 컨텍스트
    // - 핸들러가 필요한 모든 정보를 하나의 구조체로 묶음
    // - 핸들러 인터페이스 시그니처를 단순하게 유지하기 위한 설계
    struct EventContext
    {
        std::shared_ptr<CZoneGroup>     zone_group;
        std::shared_ptr<CUser>          user;
        const TableUserEvent*           table_event     = nullptr;
        UserEventInfo*                  event_info      = nullptr;  // nullptr 가능 (미등록 상태)

        time_t                          now             = 0;
        time_t                          last_check_time = 0;
        time_t                          reset_time      = 0;        // 오늘 리셋 기준 시각
    };

    // 이벤트 상태 변경 결과 (핸들러 반환값)
    enum class HandleResult
    {
        None = 0,
        Continue,   // 다음 처리 없이 continue
        Notified,   // 클라이언트에 상태 변경 알림 발송됨
        Reset,      // 일일 리셋 처리됨 → notify_change_day 플래그 설정
    };

} // namespace GameEvent
