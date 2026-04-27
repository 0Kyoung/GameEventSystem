#pragma once
#include "EventType.h"
#include "EventContext.h"
#include "IEventHandler.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <cassert>

namespace GameEvent
{

    // 이벤트 타입과 핸들러를 연결하는 디스패처
    //
    // 사용 방법:
    //   1. RegisterHandler()로 핸들러를 등록
    //   2. Dispatch*() 로 해당 이벤트 타입의 핸들러를 호출
    //
    // 기존 코드 대비 개선점:
    //   - 이벤트 추가 시 RegisterHandler() 한 줄만 추가
    //   - CheckUserEvent 내 if-else 체인을 수정할 필요 없음
    class EventDispatcher
    {
    public:
        // 싱글톤
        static EventDispatcher& GetInstance()
        {
            static EventDispatcher instance;
            return instance;
        }

        // 핸들러 등록
        // 하나의 핸들러가 여러 이벤트 타입을 담당할 수 있음
        void RegisterHandler(std::shared_ptr<IEventHandler> handler)
        {
            assert(handler != nullptr);
            for (auto type : handler->GetSupportedTypes())
            {
                assert(handler_map_.find(type) == handler_map_.end()
                    && "이벤트 타입에 핸들러가 이미 등록되어 있습니다");
                handler_map_[type] = handler;
            }
        }

        // 로그인 동기화 디스패치
        bool DispatchLoginSync(EventType type, const EventContext& ctx) const
        {
            auto* handler = FindHandler(type);
            if (handler == nullptr)
                return false;
            return handler->OnLoginSync(ctx);
        }

        // 시간 체크 디스패치
        HandleResult DispatchTimeCheck(EventType type, const EventContext& ctx) const
        {
            auto* handler = FindHandler(type);
            if (handler == nullptr)
                return HandleResult::None;
            return handler->OnTimeCheck(ctx);
        }

        // 패킷 정보 채우기 디스패치
        bool DispatchFillPacket(EventType type, const EventContext& ctx, void* out_proto) const
        {
            auto* handler = FindHandler(type);
            if (handler == nullptr)
                return false;
            return handler->FillPacketInfo(ctx, out_proto);
        }

        // 특정 타입의 핸들러가 등록되어 있는지 확인
        bool HasHandler(EventType type) const
        {
            return handler_map_.find(type) != handler_map_.end();
        }

    private:
        EventDispatcher() = default;

        IEventHandler* FindHandler(EventType type) const
        {
            auto it = handler_map_.find(type);
            return (it != handler_map_.end()) ? it->second.get() : nullptr;
        }

        std::unordered_map<EventType, std::shared_ptr<IEventHandler>> handler_map_;
    };

    // 전역 접근 편의 함수
    inline EventDispatcher& GEventDispatcher()
    {
        return EventDispatcher::GetInstance();
    }

} // namespace GameEvent
