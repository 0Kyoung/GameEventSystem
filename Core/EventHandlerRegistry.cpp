#include "EventHandlerRegistry.h"
#include "EventDispatcher.h"
#include "../Handlers/AttendanceEventHandler.h"
#include "../Handlers/BingoEventHandler.h"
#include "../Handlers/MissionPassEventHandler.h"
#include "../Handlers/SiegeEveEventHandler.h"
#include "../Handlers/HuntingEventHandler.h"
#include "../Handlers/BenedictionEventHandler.h"

namespace GameEvent
{

    // 모든 핸들러를 EventDispatcher에 등록
    // 새 이벤트 타입 추가 시 이 함수에 한 줄만 추가하면 됨
    void RegisterAllEventHandlers()
    {
        auto& dispatcher = GEventDispatcher();

        dispatcher.RegisterHandler(std::make_shared<AttendanceEventHandler>());
        dispatcher.RegisterHandler(std::make_shared<BingoEventHandler>());
        dispatcher.RegisterHandler(std::make_shared<MissionPassEventHandler>());
        dispatcher.RegisterHandler(std::make_shared<SiegeEveEventHandler>());
        dispatcher.RegisterHandler(std::make_shared<HuntingEventHandler>());
        dispatcher.RegisterHandler(std::make_shared<BenedictionEventHandler>());

        // ── 새 이벤트 추가 시 여기에 한 줄 ──────────────────────────────────
        // dispatcher.RegisterHandler(std::make_shared<NewEventHandler>());
    }

} // namespace GameEvent
