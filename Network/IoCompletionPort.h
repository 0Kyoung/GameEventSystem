#pragma once
// Windows 전용 (IOCP).
//
// GetQueuedCompletionStatus() 루프를 도는 워커 스레드 풀.
// 스레드 수는 관례대로 CPU 코어 수만큼 둔다 (그 이상은 컨텍스트 스위칭
// 비용만 늘고, IOCP가 어차피 코어 수만큼만 스레드를 동시에 깨워준다).
#include "Session.h"
#include <winsock2.h>
#include <thread>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <mutex>

namespace GameNet
{
    class IoCompletionPort
    {
    public:
        IoCompletionPort() = default;
        ~IoCompletionPort();

        bool Start(int worker_count);
        void Stop();

        // accept 직후 호출: 소켓을 IOCP에 등록
        bool AssociateSocket(SOCKET socket, std::shared_ptr<Session> session);

        // Session 소멸 시 등록 해제(맵에서만 제거 — 핸들 자체는 CloseHandle 불필요,
        // 소켓을 닫으면 IOCP 등록도 함께 정리된다)
        void RemoveSession(SOCKET socket);

    private:
        void WorkerLoop();

        HANDLE iocp_handle_ = nullptr;
        std::vector<std::thread> workers_;
        std::atomic<bool> running_{ false };

        // OVERLAPPED만으로는 어느 Session인지 알 수 없으므로 completion key로
        // socket을 등록해 두고, GQCS가 돌려주는 key로 Session을 찾는다.
        std::mutex session_mutex_;
        std::unordered_map<SOCKET, std::shared_ptr<Session>> sessions_;
    };

} // namespace GameNet
