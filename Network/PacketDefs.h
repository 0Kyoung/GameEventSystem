#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 패킷 헤더 및 오퍼레이션 코드 정의.
//
// TCP는 스트림이라 한 번의 recv가 패킷 하나와 정확히 일치한다는 보장이 없다.
// (여러 패킷이 붙어 오거나, 패킷 하나가 잘려서 온다) 그래서 모든 패킷은
// 고정 크기 헤더(size, opcode)로 시작하게 해서, Session이 수신 버퍼에서
// "헤더만큼 왔는가 → 그 size만큼 본문이 왔는가"를 스스로 판단할 수 있게 한다.
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>

namespace GameNet
{
    constexpr uint16_t kMaxPacketSize = 4096;

#pragma pack(push, 1)
    struct PacketHeader
    {
        uint16_t size;    // 헤더를 포함한 전체 패킷 크기
        uint16_t opcode;
    };
#pragma pack(pop)

    static_assert(sizeof(PacketHeader) == 4, "PacketHeader는 4바이트로 고정되어야 한다");

    enum class Opcode : uint16_t
    {
        None                = 0,

        CS_LOGIN            = 1001, // uid를 담아 로그인 요청
        SC_LOGIN_ACK        = 1002,

        CS_EVENT_INFO_REQ   = 2001, // 이벤트 정보 패킷 요청 (기존 CLIENT_GAME_USER_EVENT_INFO)
        SC_EVENT_INFO_ACK   = 2002,

        SC_ERROR            = 9001,
    };

#pragma pack(push, 1)
    struct CsLogin
    {
        uint64_t uid;
    };

    struct ScLoginAck
    {
        uint8_t success; // 0/1
    };
#pragma pack(pop)

} // namespace GameNet
