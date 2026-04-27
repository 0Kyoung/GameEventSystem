#pragma once
#include "../Core/EventContext.h"
#include "../Core/EventDispatcher.h"
#include <memory>
#include <chrono>

class CUser;
class CZoneGroup;

namespace GameEvent
{

    // 유저 이벤트 주기적 시간 체크
    //
    // 기존 구조의 문제:
    //   CheckUserEvent() 함수가 332줄짜리 거대 함수로,
    //   이벤트 타입별 if-else 체인이 함수 내부에 직접 구현되어 있었음.
    //   새 이벤트 타입 추가 시 이 함수를 직접 수정해야 했음.
    //
    // 개선된 구조:
    //   - 타입별 처리는 각 핸들러가 담당
    //   - 이 클래스는 순회와 디스패치만 담당 (단일 책임)
    //   - 새 이벤트 추가 = 핸들러 구현 + RegisterHandler() 한 줄
    class EventUpdateChecker
    {
    public:
        // 주기적 업데이트 진입점 (UserUpdate tick에서 호출)
        static void Check(
            const std::chrono::system_clock::time_point& now_tp,
            const std::shared_ptr<CZoneGroup>& zone_group,
            const std::shared_ptr<CUser>& user);

    private:
        // 업데이트 주기 체크 (5초 기본)
        static bool ShouldCheck(
            const std::chrono::system_clock::time_point& now_tp,
            const std::shared_ptr<CUser>& user);

        // 이벤트 테이블 순회 및 핸들러 디스패치
        static bool ProcessEventLoop(
            const EventContext& base_ctx,
            const std::shared_ptr<CZoneGroup>& zone_group,
            const std::shared_ptr<CUser>& user);

        // 공통 컨텍스트 구성
        static EventContext BuildBaseContext(
            time_t now,
            time_t last_check_time,
            const std::shared_ptr<CZoneGroup>& zone_group,
            const std::shared_ptr<CUser>& user);
    };

} // namespace GameEvent
