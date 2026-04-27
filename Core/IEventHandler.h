#pragma once
#include "EventType.h"
#include "EventContext.h"
#include <vector>

namespace GameEvent
{

    // 모든 이벤트 핸들러가 구현해야 하는 인터페이스
    //
    // 설계 의도:
    //   기존 코드는 CheckUserEvent / CheckEventLoginData 내부에
    //   이벤트 타입별 분기가 거대한 if-else 체인으로 구성되어 있었음.
    //   새 이벤트 추가 시 기존 함수를 직접 수정해야 하는 구조.
    //
    //   개선 후:
    //   각 이벤트 타입은 독립적인 핸들러 클래스를 가지며,
    //   EventDispatcher에 등록만 하면 기존 코드 수정 없이 동작.
    //   (Open/Closed Principle)
    class IEventHandler
    {
    public:
        virtual ~IEventHandler() = default;

        // 이 핸들러가 담당하는 이벤트 타입 목록 반환
        // 여러 타입을 하나의 핸들러로 처리할 수 있음 (e.g. MissionPassGrowth + MissionPassChallenge)
        virtual std::vector<EventType> GetSupportedTypes() const = 0;

        // 로그인 시 이벤트 상태 동기화
        // - 서버 재접속 / 로그인 시 한 번 호출
        // - DB 또는 캐시에서 유저 이벤트 데이터를 불러와 메모리에 반영
        virtual bool OnLoginSync(const EventContext& ctx) = 0;

        // 주기적 시간 체크 (Update tick)
        // - 이벤트 시작/종료/리셋 조건을 검사하고 처리
        // - 반환값으로 후처리(notify 등)를 결정
        virtual HandleResult OnTimeCheck(const EventContext& ctx) = 0;

        // 이벤트 정보 패킷 구성
        // - CLIENT_GAME_USER_EVENT_INFO 응답 시 호출
        // - out_proto에 이벤트 정보를 채움
        virtual bool FillPacketInfo(const EventContext& ctx, void* out_proto) const = 0;
    };

} // namespace GameEvent
