#pragma once
// Windows 전용 (IOCP 세션을 다루므로).
//
// 이 파일이 이 확장의 핵심 연결부다.
//   네트워크에서 받은 패킷(opcode) → GameEventSystem의 진입점 함수 호출
//
// Session::OnRecvComplete()가 패킷 하나를 완전히 파싱한 뒤 여기로 넘긴다.
#include "PacketDefs.h"
#include <memory>
#include <cstdint>

namespace GameNet
{
    class Session;

    class PacketRouter
    {
    public:
        static void Route(const std::shared_ptr<Session>& session,
                           Opcode opcode,
                           const char* payload,
                           uint16_t payload_size);

    private:
        static void HandleLogin(const std::shared_ptr<Session>& session,
                                 const char* payload, uint16_t payload_size);

        static void HandleEventInfoRequest(const std::shared_ptr<Session>& session);
    };

} // namespace GameNet
