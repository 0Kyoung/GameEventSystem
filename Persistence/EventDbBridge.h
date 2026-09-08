#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// GameEventSystem(Core/Update/Handlers)과 DB 계층(AsyncDbJobQueue) 사이의 접착 코드.
//
// GameEventSystem 쪽 EventLoginSyncer::Sync()는 "유저 이벤트 데이터가
// 이미 UserEventContainer에 로드되어 있다"는 것을 전제로 동작한다
// (EventContext::event_info = user->GetUserEvent().GetInfo(...)).
//
// 실제 로그인 시퀀스에서는 그 전제를 만족시키기 위해 DB에서 유저의
// user_event 테이블 행을 먼저 읽어와야 하는데, 이 조회를 게임 스레드에서
// 동기로 하면 로그인 처리 중 서버 틱이 그대로 멎는다.
//
// EventDbBridge::LoadUserEventsAsync() 가 그 사이를 메꾼다:
//   1) AsyncDbJobQueue 워커 스레드에서 QueryUserEvents(uid) 실행
//   2) 메인 스레드로 넘어온 뒤 UserEventContainer에 결과를 채움
//   3) 그 다음에야 on_loaded 콜백(보통 EventLoginSyncer::Sync 호출)을 실행
// ─────────────────────────────────────────────────────────────────────────────
#include "../Core/GameTypes.h"
#include <cstdint>
#include <functional>
#include <memory>

namespace GameDb
{
    class EventDbBridge
    {
    public:
        // uid          : 로드할 유저의 고유 ID
        // user         : 조회 결과를 채워 넣을 대상 (메인 스레드에서만 접근됨)
        // on_loaded    : 로드 + 반영이 끝난 뒤 메인 스레드에서 실행할 콜백
        //                (일반적으로 GameEvent::EventLoginSyncer::Sync(...) 호출)
        static void LoadUserEventsAsync(
            uint64_t uid,
            std::shared_ptr<CUser> user,
            std::function<void()> on_loaded);
    };

} // namespace GameDb
