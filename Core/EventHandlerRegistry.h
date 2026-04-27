#pragma once

namespace GameEvent
{

    // 모든 이벤트 핸들러를 EventDispatcher에 등록하는 진입점
    //
    // 서버 초기화 시 한 번 호출.
    // 새 이벤트 타입 추가 시 이 함수에 RegisterHandler() 한 줄만 추가하면 됨.
    void RegisterAllEventHandlers();

} // namespace GameEvent
