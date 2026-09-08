#include "PacketRouter.h"
#include "Session.h"
#include "../Update/EventLoginSyncer.h"
#include "../Update/EventInfoPacketBuilder.h"
#include "../Persistence/EventDbBridge.h"
#include <cstring>
#include <iostream>

namespace GameNet
{
    void PacketRouter::Route(const std::shared_ptr<Session>& session,
                              Opcode opcode,
                              const char* payload,
                              uint16_t payload_size)
    {
        switch (opcode)
        {
        case Opcode::CS_LOGIN:
            HandleLogin(session, payload, payload_size);
            break;

        case Opcode::CS_EVENT_INFO_REQ:
            HandleEventInfoRequest(session);
            break;

        default:
            std::cerr << "[PacketRouter] unknown opcode=" << static_cast<uint16_t>(opcode) << "\n";
            break;
        }
    }

    void PacketRouter::HandleLogin(const std::shared_ptr<Session>& session,
                                    const char* payload, uint16_t payload_size)
    {
        if (payload_size < sizeof(CsLogin))
        {
            ScLoginAck ack{ 0 };
            session->PostSend(Opcode::SC_LOGIN_ACK, &ack, sizeof(ack));
            return;
        }

        CsLogin req{};
        std::memcpy(&req, payload, sizeof(req));

        // 실제 서버라면 여기서 인증(토큰 검증) 후 UserManager에서
        // CUser/CZoneGroup을 찾아오지만, 이 샘플에서는 uid만으로 새로 만든다.
        auto user = std::make_shared<CUser>(req.uid);
        auto zone = std::make_shared<CZoneGroup>();
        session->SetUid(req.uid);
        session->BindUser(user, zone);

        // ── 네트워크 → 비동기 DB 로드 → 게임 이벤트 시스템 반영, 순서로 연결 ──
        //
        // LoadUserEventsAsync는 워커 스레드에서 DB를 조회하고,
        // 그 결과를 메인(게임) 스레드 반영 콜백까지 큐에 넣어 둔다.
        // 아래 람다(EventLoginSyncer::Sync 호출 이후)는 그 반영이 끝난 뒤,
        // 서버의 NetworkServer::Tick() → AsyncDbJobQueue::ProcessCompletions()
        // 호출 시점에 메인 스레드에서 실행된다. Session::PostSend 역시
        // 메인 스레드에서만 호출되므로 별도 동기화가 필요 없다.
        GameDb::EventDbBridge::LoadUserEventsAsync(
            req.uid, user,
            [session, user, zone]()
            {
                GameEvent::EventLoginSyncer::Sync(zone, user);

                ScLoginAck ack{ 1 };
                session->PostSend(Opcode::SC_LOGIN_ACK, &ack, sizeof(ack));
            });
    }

    void PacketRouter::HandleEventInfoRequest(const std::shared_ptr<Session>& session)
    {
        auto user = session->GetUser();
        auto zone = session->GetZoneGroup();

        if (user == nullptr || zone == nullptr)
        {
            ScLoginAck ack{ 0 }; // 로그인 전 요청 — 재사용 가능한 실패 응답으로 대체
            session->PostSend(Opcode::SC_ERROR, &ack, sizeof(ack));
            return;
        }

        // EventInfoPacketBuilder::Send()는 이 저장소의 Core/Update 계층이
        // 담당하는 부분으로, 실제 프로토콜 직렬화(out_smsg 채우기)는
        // 서버별 패킷 포맷에 종속적이라 이 포트폴리오 범위 밖에 둔다.
        // 여기서는 그 결과(성공/실패)만 받아 ACK 오퍼코드로 알린다.
        const bool ok = GameEvent::EventInfoPacketBuilder::Send(zone, user);

        uint8_t result = ok ? 1 : 0;
        session->PostSend(ok ? Opcode::SC_EVENT_INFO_ACK : Opcode::SC_ERROR,
                           &result, sizeof(result));
    }

} // namespace GameNet
