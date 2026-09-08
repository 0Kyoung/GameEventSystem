#include "IoCompletionPort.h"
#include "IoContext.h"
#include <iostream>

namespace GameNet
{
    IoCompletionPort::~IoCompletionPort()
    {
        Stop();
    }

    bool IoCompletionPort::Start(int worker_count)
    {
        iocp_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, worker_count);
        if (iocp_handle_ == nullptr)
        {
            std::cerr << "[IoCompletionPort] CreateIoCompletionPort failed: "
                      << GetLastError() << "\n";
            return false;
        }

        running_ = true;
        for (int i = 0; i < worker_count; ++i)
            workers_.emplace_back(&IoCompletionPort::WorkerLoop, this);

        return true;
    }

    void IoCompletionPort::Stop()
    {
        if (!running_.exchange(false))
            return;

        // 워커들을 깨우기 위해 completion key 0(더미)로 PostQueuedCompletionStatus를
        // worker 수만큼 날린다. WorkerLoop은 running_==false && lpOverlapped==nullptr
        // 조합을 종료 신호로 해석한다.
        for (size_t i = 0; i < workers_.size(); ++i)
            PostQueuedCompletionStatus(iocp_handle_, 0, 0, nullptr);

        for (auto& t : workers_)
        {
            if (t.joinable())
                t.join();
        }
        workers_.clear();

        if (iocp_handle_ != nullptr)
        {
            CloseHandle(iocp_handle_);
            iocp_handle_ = nullptr;
        }
    }

    bool IoCompletionPort::AssociateSocket(SOCKET socket, std::shared_ptr<Session> session)
    {
        // completion key로 socket 값을 그대로 사용 — WorkerLoop에서
        // 다시 socket을 key로 sessions_ 맵을 조회해 Session을 찾는다.
        HANDLE result = CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(socket), iocp_handle_,
            static_cast<ULONG_PTR>(socket), 0);

        if (result == nullptr)
        {
            std::cerr << "[IoCompletionPort] associate failed: " << GetLastError() << "\n";
            return false;
        }

        std::lock_guard<std::mutex> lock(session_mutex_);
        sessions_[socket] = std::move(session);
        return true;
    }

    void IoCompletionPort::RemoveSession(SOCKET socket)
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        sessions_.erase(socket);
    }

    void IoCompletionPort::WorkerLoop()
    {
        for (;;)
        {
            DWORD bytes_transferred = 0;
            ULONG_PTR completion_key = 0;
            LPOVERLAPPED overlapped = nullptr;

            const BOOL ok = GetQueuedCompletionStatus(
                iocp_handle_, &bytes_transferred, &completion_key, &overlapped, INFINITE);

            if (!running_.load() && overlapped == nullptr)
                return; // Stop()이 보낸 종료 신호

            if (overlapped == nullptr)
                continue; // 스퓨리어스 웨이크업 등 — 다음 루프

            const SOCKET socket = static_cast<SOCKET>(completion_key);

            std::shared_ptr<Session> session;
            {
                std::lock_guard<std::mutex> lock(session_mutex_);
                auto it = sessions_.find(socket);
                if (it != sessions_.end())
                    session = it->second;
            }

            if (session == nullptr)
                continue; // 이미 정리된 세션의 지연 완료 통지 — 무시

            if (!ok || (bytes_transferred == 0 &&
                        static_cast<IoContext*>(overlapped)->type == IoType::Recv))
            {
                // GQCS 자체가 실패했거나 recv가 0바이트 = 상대 종료/에러
                session->Close();
                RemoveSession(socket);
                continue;
            }

            auto* io_ctx = static_cast<IoContext*>(overlapped);
            if (io_ctx->type == IoType::Recv)
                session->OnRecvComplete(bytes_transferred);
            else
                session->OnSendComplete(bytes_transferred);
        }
    }

} // namespace GameNet
