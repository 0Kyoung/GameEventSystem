#pragma once
#include "../Core/EventContext.h"
#include "../Core/EventDispatcher.h"
#include <memory>
#include <vector>

class CUser;
class CZoneGroup;

namespace GameEvent
{

    // 유저 이벤트 정보 패킷 구성
    //
    // 기존 구조의 문제:
    //   SendUserEventInfo() 613줄짜리 함수에
    //   이벤트 타입별 패킷 필드 채우기 로직이 전부 혼재.
    //   새 이벤트 추가 시 이 함수에 직접 분기를 추가해야 했음.
    //
    // 개선된 구조:
    //   - 각 핸들러의 FillPacketInfo()가 타입별 패킷 구성 책임을 가짐
    //   - 이 클래스는 순회, 필터링, 전송만 담당
    //   - 새 이벤트 = 핸들러 추가만으로 자동으로 패킷에 포함됨
    class EventInfoPacketBuilder
    {
    public:

        // 유저에게 이벤트 정보 패킷 전송 (기존 SendUserEventInfo 대체)
        static bool Send(
            const std::shared_ptr<CZoneGroup>& zone_group,
            const std::shared_ptr<CUser>& user);

    private:
        // 이벤트를 패킷에 포함할지 필터링
        // (시간 범위, 월드 타입, 신규/복귀 유저 조건 등)
        static bool ShouldIncludeInPacket(
            const EventContext& ctx,
            const std::shared_ptr<CZoneGroup>& zone_group);

        // 일일 초기화 정보 설정
        static void SetDailyAccessField(
            const std::shared_ptr<CUser>& user,
            time_t now,
            void* out_smsg);
    };

} // namespace GameEvent
