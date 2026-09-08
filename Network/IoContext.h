#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Windows 전용 (IOCP). Visual Studio + Winsock2로 빌드한다.
//
// OVERLAPPED를 그대로 확장해서, GetQueuedCompletionStatus()가 돌려주는
// LPOVERLAPPED 포인터를 IoContext*로 그대로 캐스팅해 쓸 수 있게 한다.
// (OVERLAPPED가 구조체의 첫 멤버여야 이 캐스팅이 안전하다 — 상속으로 보장)
//
// 하나의 Session이 recv용 1개 + send용 1개, 최소 2개의 IoContext를 갖는다.
// 두 방향을 완전히 분리해 둬야 "recv 완료 처리 중에 send가 완료됐다"는
// 상황에서도 서로의 버퍼를 건드리지 않는다.
// ─────────────────────────────────────────────────────────────────────────────
#include <winsock2.h>
#include <mswsock.h>
#include "PacketDefs.h"

namespace GameNet
{
    enum class IoType
    {
        Recv,
        Send,
    };

    struct IoContext : OVERLAPPED
    {
        IoContext()
        {
            ZeroMemory(static_cast<OVERLAPPED*>(this), sizeof(OVERLAPPED));
        }

        IoType   type = IoType::Recv;
        WSABUF   wsa_buf{};
        char     buffer[kMaxPacketSize] = {};
    };

} // namespace GameNet
