#include "NetworkServer.h"
#include "../Persistence/AsyncDbJobQueue.h"
#include "../Update/EventUpdateChecker.h"
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

namespace GameNet
{
    NetworkServer::~NetworkServer()
    {
        Stop();
    }

    bool NetworkServer::Start(uint16_t port, int io_worker_count)
    {
        WSADATA wsa_data{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            std::cerr << "[NetworkServer] WSAStartup failed\n";
            return false;
        }

        // 프로젝트 문자 집합이 MultiByte라 매크로 WSASocket()이 WSASocketA로 풀리는데,
        // 최신 Windows SDK(10.0.26100+)에서 WSASocketA가 deprecated(C4996) 처리된다.
        // 문자열 인자가 없는 함수라 A/W 차이가 의미 없으므로 WSASocketW를 직접 호출해 회피한다.
        listen_socket_ = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
                                     nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (listen_socket_ == INVALID_SOCKET)
        {
            std::cerr << "[NetworkServer] WSASocketW failed: " << WSAGetLastError() << "\n";
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
        {
            std::cerr << "[NetworkServer] bind failed: " << WSAGetLastError() << "\n";
            return false;
        }

        if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR)
        {
            std::cerr << "[NetworkServer] listen failed: " << WSAGetLastError() << "\n";
            return false;
        }

        if (!iocp_.Start(io_worker_count))
            return false;

        // 비동기 DB 워커도 함께 기동 (I/O 워커 수만큼 커넥션을 물려 쓰는 구성)
        GameDb::AsyncDbJobQueue::GetInstance().Start(io_worker_count);

        running_ = true;
        accept_thread_ = std::thread(&NetworkServer::AcceptLoop, this);

        std::cout << "[NetworkServer] listening on port " << port << "\n";
        return true;
    }

    void NetworkServer::Stop()
    {
        if (!running_.exchange(false))
            return;

        if (listen_socket_ != INVALID_SOCKET)
        {
            closesocket(listen_socket_); // AcceptLoop의 accept()를 깨워 빠져나오게 함
            listen_socket_ = INVALID_SOCKET;
        }

        if (accept_thread_.joinable())
            accept_thread_.join();

        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (auto& s : sessions_)
                s->Close();
            sessions_.clear();
        }

        iocp_.Stop();
        GameDb::AsyncDbJobQueue::GetInstance().Stop();
        WSACleanup();
    }

    void NetworkServer::AcceptLoop()
    {
        while (running_.load())
        {
            sockaddr_in client_addr{};
            int addr_len = sizeof(client_addr);
            const SOCKET client_socket = accept(listen_socket_,
                                                 reinterpret_cast<sockaddr*>(&client_addr),
                                                 &addr_len);

            if (client_socket == INVALID_SOCKET)
            {
                if (!running_.load())
                    break; // Stop()이 listen 소켓을 닫아서 나온 정상 종료 경로
                continue;
            }

            auto session = std::make_shared<Session>(client_socket);

            if (!iocp_.AssociateSocket(client_socket, session))
            {
                session->Close();
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                sessions_.push_back(session);
            }

            session->PostRecv();
        }
    }

    void NetworkServer::Tick()
    {
        // ── 순서가 중요하다 ──────────────────────────────────────────────────
        // DB 로드 결과를 먼저 게임 상태에 반영한 뒤에 EventUpdateChecker를
        // 돌려야, 방금 로그인해서 로드가 끝난 유저도 이번 틱에 정상적으로
        // 이벤트 시간 체크 대상에 포함된다.
        GameDb::AsyncDbJobQueue::GetInstance().ProcessCompletions();

        std::lock_guard<std::mutex> lock(sessions_mutex_);
        const auto now_tp = std::chrono::system_clock::now();

        for (auto it = sessions_.begin(); it != sessions_.end(); )
        {
            auto& session = *it;
            if (session->IsClosed())
            {
                iocp_.RemoveSession(session->GetSocket());
                it = sessions_.erase(it);
                continue;
            }

            auto user = session->GetUser();
            auto zone = session->GetZoneGroup();
            if (user != nullptr && zone != nullptr)
                GameEvent::EventUpdateChecker::Check(now_tp, zone, user);

            ++it;
        }
    }

} // namespace GameNet
