#pragma once
// Windows 전용 (IOCP).
#include "IoContext.h"
#include "../Core/GameTypes.h"
#include <winsock2.h>
#include <memory>
#include <mutex>
#include <deque>
#include <atomic>

namespace GameNet
{
    // 접속 하나에 대응하는 세션.
    //
    // - recv_ctx_ / send_ctx_ : 각 방향의 Overlapped I/O 컨텍스트
    // - recv_accum_          : TCP 스트림 재조립용 누적 버퍼
    //     (헤더가 다 안 왔거나, 헤더는 왔는데 본문이 덜 온 경우를 대비)
    // - send_queue_          : 이전 WSASend가 완료되기 전에 또 보낼 데이터가
    //     생기면 큐에 쌓아두고, 완료 콜백에서 다음 것을 이어 보낸다.
    //     (동시에 여러 WSASend를 같은 소켓에 걸면 순서가 꼬일 수 있어 금지)
    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        explicit Session(SOCKET socket);
        ~Session();

        SOCKET GetSocket() const { return socket_; }
        uint64_t GetUid() const { return uid_; }
        void SetUid(uint64_t uid) { uid_ = uid; }

        std::shared_ptr<CUser>      GetUser() const { return user_; }
        std::shared_ptr<CZoneGroup> GetZoneGroup() const { return zone_group_; }
        void BindUser(std::shared_ptr<CUser> user, std::shared_ptr<CZoneGroup> zone);

        // 최초 수신 대기 등록 (Accept 직후 1회)
        bool PostRecv();

        // 완료된 recv 처리: 누적 버퍼에 채우고, 완결된 패킷을 모두 꺼내
        // PacketRouter로 넘긴 뒤 다시 PostRecv()를 건다.
        void OnRecvComplete(DWORD bytes_transferred);

        // 패킷 하나를 큐에 넣고, 유휴 상태면 즉시 WSASend를 건다.
        void PostSend(Opcode opcode, const void* payload, uint16_t payload_size);

        // 완료된 send 처리: 큐에 남은 다음 패킷을 이어 보낸다.
        void OnSendComplete(DWORD bytes_transferred);

        void Close();
        bool IsClosed() const { return closed_.load(); }

        IoContext& RecvIoContext() { return recv_ctx_; }
        IoContext& SendIoContext() { return send_ctx_; }

    private:
        void TryFlushSendQueue_NoLock();

        SOCKET socket_;
        uint64_t uid_ = 0;

        std::shared_ptr<CUser>      user_;
        std::shared_ptr<CZoneGroup> zone_group_;

        IoContext recv_ctx_;
        IoContext send_ctx_;

        // 스트림 재조립 버퍼. kMaxPacketSize*2로 잡아 헤더+본문이 한 번에
        // 안 들어와도 다음 recv까지 버틸 수 있게 한다.
        std::vector<char> recv_accum_;

        std::mutex send_mutex_;
        std::deque<std::vector<char>> send_queue_;
        bool send_in_flight_ = false;

        std::atomic<bool> closed_{ false };
    };

} // namespace GameNet
