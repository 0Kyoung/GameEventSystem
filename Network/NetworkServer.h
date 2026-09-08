#pragma once
// Windows 전용 (IOCP).
//
// 서버 전체를 묶는 최상위 객체.
//   Listen 소켓으로 accept → IOCP에 등록 → Session::PostRecv()로 수신 대기.
//
// AcceptEx 기반 완전 비동기 accept 대신, 별도 스레드에서 블로킹 accept()를
// 도는 단순한 구성을 택했다. 신규 접속 처리량이 병목이 되는 서비스가 아니라면
// (게임 서버의 접속 빈도는 패킷 처리량에 비해 훨씬 낮다) 이 정도로 충분하고,
// AcceptEx 큐잉/재사용 로직 없이도 코드를 훨씬 단순하게 유지할 수 있다.
// 접속 폭주 대응이 필요해지면 AcceptEx + 소켓 프리 생성으로 교체할 지점을
// 여기 하나로 좁혀둔 것이 이 구조의 의도다.
#include "IoCompletionPort.h"
#include "Session.h"
#include <winsock2.h>
#include <thread>
#include <atomic>

namespace GameNet
{
    class NetworkServer
    {
    public:
        NetworkServer() = default;
        ~NetworkServer();

        bool Start(uint16_t port, int io_worker_count);
        void Stop();

        // 메인(게임) 스레드가 일정 주기(예: 100ms)로 호출.
        //  - GameDb::AsyncDbJobQueue::ProcessCompletions() 로 DB 결과를 반영하고
        //  - 접속 중인 유저들에 대해 GameEvent::EventUpdateChecker::Check()를 돌린다.
        void Tick();

    private:
        void AcceptLoop();

        SOCKET listen_socket_ = INVALID_SOCKET;
        IoCompletionPort iocp_;

        std::thread accept_thread_;
        std::atomic<bool> running_{ false };

        std::mutex sessions_mutex_;
        std::vector<std::shared_ptr<Session>> sessions_;
    };

} // namespace GameNet
