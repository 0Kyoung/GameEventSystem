#include "Session.h"
#include "PacketRouter.h"
#include <iostream>
#include <cstring>

namespace GameNet
{
    Session::Session(SOCKET socket)
        : socket_(socket)
    {
        recv_ctx_.type = IoType::Recv;
        send_ctx_.type = IoType::Send;
        recv_accum_.reserve(kMaxPacketSize * 2);
    }

    Session::~Session()
    {
        Close();
    }

    void Session::BindUser(std::shared_ptr<CUser> user, std::shared_ptr<CZoneGroup> zone)
    {
        user_ = std::move(user);
        zone_group_ = std::move(zone);
    }

    bool Session::PostRecv()
    {
        if (closed_.load())
            return false;

        ZeroMemory(static_cast<OVERLAPPED*>(&recv_ctx_), sizeof(OVERLAPPED));
        recv_ctx_.wsa_buf.buf = recv_ctx_.buffer;
        recv_ctx_.wsa_buf.len = static_cast<ULONG>(sizeof(recv_ctx_.buffer));

        DWORD flags = 0;
        DWORD bytes = 0;
        const int ret = WSARecv(socket_, &recv_ctx_.wsa_buf, 1, &bytes, &flags,
                                 static_cast<LPOVERLAPPED>(&recv_ctx_), nullptr);

        if (ret == SOCKET_ERROR)
        {
            const int err = WSAGetLastError();
            if (err != WSA_IO_PENDING)
            {
                // 진짜 에러 (연결 끊김 등) — IO_PENDING만 정상 진행 상태
                Close();
                return false;
            }
        }
        return true;
    }

    void Session::OnRecvComplete(DWORD bytes_transferred)
    {
        if (bytes_transferred == 0)
        {
            // 상대가 정상 종료(FIN)한 경우
            Close();
            return;
        }

        // 이번에 받은 만큼 누적 버퍼 뒤에 붙인다.
        recv_accum_.insert(recv_accum_.end(),
                            recv_ctx_.buffer,
                            recv_ctx_.buffer + bytes_transferred);

        // 누적 버퍼 안에 완결된 패킷이 여러 개 들어 있을 수 있으므로 while로 전부 소진.
        size_t offset = 0;
        while (recv_accum_.size() - offset >= sizeof(PacketHeader))
        {
            PacketHeader header{};
            std::memcpy(&header, recv_accum_.data() + offset, sizeof(PacketHeader));

            if (header.size < sizeof(PacketHeader) || header.size > kMaxPacketSize)
            {
                // 손상되었거나 악의적인 패킷 — 세션을 끊는다.
                std::cerr << "[Session] invalid packet size=" << header.size << "\n";
                Close();
                return;
            }

            if (recv_accum_.size() - offset < header.size)
                break; // 본문이 아직 덜 옴 — 다음 recv를 기다린다.

            const char* payload = recv_accum_.data() + offset + sizeof(PacketHeader);
            const uint16_t payload_size = header.size - sizeof(PacketHeader);

            PacketRouter::Route(shared_from_this(),
                                 static_cast<Opcode>(header.opcode),
                                 payload, payload_size);

            offset += header.size;
        }

        // 처리하고 남은(다음 패킷의 일부) 바이트만 앞으로 당겨서 보관.
        if (offset > 0)
            recv_accum_.erase(recv_accum_.begin(), recv_accum_.begin() + offset);

        PostRecv();
    }

    void Session::PostSend(Opcode opcode, const void* payload, uint16_t payload_size)
    {
        if (closed_.load())
            return;

        std::vector<char> packet(sizeof(PacketHeader) + payload_size);
        PacketHeader header{};
        header.size   = static_cast<uint16_t>(packet.size());
        header.opcode = static_cast<uint16_t>(opcode);

        std::memcpy(packet.data(), &header, sizeof(PacketHeader));
        if (payload_size > 0)
            std::memcpy(packet.data() + sizeof(PacketHeader), payload, payload_size);

        std::lock_guard<std::mutex> lock(send_mutex_);
        send_queue_.push_back(std::move(packet));
        TryFlushSendQueue_NoLock();
    }

    void Session::TryFlushSendQueue_NoLock()
    {
        // 이미 WSASend가 진행 중이면 그 완료 콜백(OnSendComplete)이
        // 알아서 다음 것을 꺼내 보낸다 — 동시에 두 번 WSASend를 걸지 않는다.
        if (send_in_flight_ || send_queue_.empty() || closed_.load())
            return;

        auto& next = send_queue_.front();

        ZeroMemory(static_cast<OVERLAPPED*>(&send_ctx_), sizeof(OVERLAPPED));
        send_ctx_.wsa_buf.buf = next.data();
        send_ctx_.wsa_buf.len = static_cast<ULONG>(next.size());

        DWORD bytes = 0;
        const int ret = WSASend(socket_, &send_ctx_.wsa_buf, 1, &bytes, 0,
                                 static_cast<LPOVERLAPPED>(&send_ctx_), nullptr);

        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            Close();
            return;
        }

        send_in_flight_ = true;
    }

    void Session::OnSendComplete(DWORD /*bytes_transferred*/)
    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        if (!send_queue_.empty())
            send_queue_.pop_front();

        send_in_flight_ = false;
        TryFlushSendQueue_NoLock();
    }

    void Session::Close()
    {
        if (closed_.exchange(true))
            return;

        if (socket_ != INVALID_SOCKET)
        {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
    }

} // namespace GameNet
