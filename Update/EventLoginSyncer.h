#pragma once
#include "../Core/EventContext.h"
#include "../Core/EventDispatcher.h"
#include <memory>

class CUser;
class CZoneGroup;

namespace GameEvent
{

    // 로그인 시 유저 이벤트 상태 동기화
    //
    // 기존 구조의 문제:
    //   CheckEventLoginData() 305줄짜리 함수에
    //   이벤트 타입별 DB 로드 / 시간 체크 / 초기화 로직이 혼재.
    //
    // 개선된 구조:
    //   - 각 핸들러의 OnLoginSync()가 타입별 책임을 가짐
    //   - 이 클래스는 순회와 디스패치만 담당
    class EventLoginSyncer
    {
    public:

        // 로그인 완료 후 호출 (기존 CheckEventLoginData 대체)
        // - 유저의 이벤트 목록을 순회하며 각 핸들러에 동기화를 위임
        // - 신규 이벤트 등록, 리셋, DB 로드 등을 각 핸들러가 처리
        static void Sync(
            const std::shared_ptr<CZoneGroup>& zone_group,
            const std::shared_ptr<CUser>& user);

    private:
        static EventContext BuildContext(
            const std::shared_ptr<CZoneGroup>& zone_group,
            const std::shared_ptr<CUser>& user,
            const TableUserEvent* table_event);
    };

} // namespace GameEvent
