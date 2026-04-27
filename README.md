# GameEventSystem

게임 서버의 유저 이벤트 처리 시스템을 핸들러 패턴으로 리팩토링한 샘플 프로젝트입니다.

---

## 기존 구조의 문제

```
CheckUserEvent()       - 332줄  ┐
CheckEventLoginData()  - 305줄  ├ 이벤트 타입별 if-else 체인이 각 함수에 중복
SendUserEventInfo()    - 613줄  ┘
```

- 새 이벤트 타입 추가 시 위 3개 함수를 **동시에 수정**해야 함
- 타입별 로직이 거대한 함수 안에 혼재 → 코드 파악 난이도 증가
- 빙고처럼 추가 동작(보드 삭제)이 필요한 타입이 다른 타입 블록 안에 혼재
- 50개 이상의 `#define` 플래그로 데드코드가 누적된 구조

---

## 개선된 구조

```
GameEventSystem/
├── Core/
│   ├── EventType.h               이벤트 타입 enum 정의
│   ├── EventContext.h            핸들러 공통 컨텍스트
│   ├── IEventHandler.h           핸들러 인터페이스 (3개 메서드)
│   ├── EventDispatcher.h         타입-핸들러 매핑 및 디스패치
│   ├── EventHandlerRegistry.h/cpp 핸들러 등록 진입점
│   └── GameTypes.h               빌드 환경별 타입 선택
│
├── Handlers/
│   ├── AttendanceEventHandler    출석 계열 (8가지 타입 통합)
│   ├── BingoEventHandler         빙고 이벤트 (보드 삭제 포함)
│   ├── MissionPassEventHandler   미션 패스
│   ├── SiegeEveEventHandler      공성 전야
│   ├── HuntingEventHandler       사냥 이벤트
│   └── BenedictionEventHandler   복덕 이벤트
│
├── Update/
│   ├── EventUpdateChecker        CheckUserEvent 대체 (주기적 시간 체크)
│   ├── EventLoginSyncer          CheckEventLoginData 대체 (로그인 동기화)
│   └── EventInfoPacketBuilder    SendUserEventInfo 대체 (패킷 구성)
│
└── Test/
    ├── MockTypes.h               빌드용 Mock 타입 정의
    └── main.cpp                  테스트 33개
```

---

## 핵심 개선점

### 1. Open/Closed Principle 적용

새 이벤트 타입 추가 시 **기존 코드를 수정하지 않음**

```cpp
// Before: CheckUserEvent 332줄 함수 직접 수정
// After:  EventHandlerRegistry.cpp 에 한 줄만 추가
dispatcher.RegisterHandler(std::make_shared<NewEventHandler>());
```

### 2. 단일 책임 원칙 적용

| 기존 | 개선 | 역할 |
|------|------|------|
| `CheckUserEvent` (332줄) | `EventUpdateChecker` | 순회/디스패치만 담당 |
| `CheckEventLoginData` (305줄) | `EventLoginSyncer` | 로그인 동기화만 담당 |
| `SendUserEventInfo` (613줄) | `EventInfoPacketBuilder` | 패킷 구성만 담당 |
| 타입별 로직 인라인 혼재 | 각 `XxxEventHandler` | 타입별 독립 책임 |

### 3. 인터페이스 통일

모든 이벤트 핸들러가 동일한 3개의 메서드를 구현

```cpp
class IEventHandler {
    // 로그인 시 DB 데이터 동기화
    virtual bool         OnLoginSync   (const EventContext& ctx) = 0;
    // 주기적 시간 체크 (시작/종료/리셋)
    virtual HandleResult OnTimeCheck   (const EventContext& ctx) = 0;
    // 클라이언트 패킷 정보 구성
    virtual bool         FillPacketInfo(const EventContext& ctx, void* out) const = 0;
};
```

---

## 사용 예시

```cpp
// 서버 초기화 시 (한 번만 호출)
GameEvent::RegisterAllEventHandlers();

// 로그인 시 (CheckEventLoginData 대체)
GameEvent::EventLoginSyncer::Sync(zone_group, user);

// Update tick (CheckUserEvent 대체)
GameEvent::EventUpdateChecker::Check(now_tp, zone_group, user);

// 이벤트 정보 패킷 전송 (SendUserEventInfo 대체)
GameEvent::EventInfoPacketBuilder::Send(zone_group, user);
```

---

## 빌드 방법

### VS2017 이상
- C++ 언어 표준: `/std:c++17`
- 추가 포함 디렉터리: `$(ProjectDir)`
- 전처리기 정의: `GAME_EVENT_TEST`

---

## 테스트 항목 (TC 12개 / 검증 항목 33개)

TC 함수는 12개이며, TC1이 이벤트 타입 15개를 루프로 순회하기 때문에
실제 `CHECK()` 실행 횟수는 33번입니다.

| TC | CHECK 수 | 항목 |
|----|----------|------|
| TC1  | 15 | 핸들러 등록 확인 (이벤트 타입 15개 루프 검증) |
| TC2  | 3  | 미등록 타입 안전 처리 (TimeCheck / LoginSync / FillPacket) |
| TC3  | 1  | 출석 이벤트 종료 처리 |
| TC4  | 1  | 이미 삭제된 이벤트 종료 무시 |
| TC5  | 2  | 사냥 이벤트 기간 외 처리 (미래 / 과거) |
| TC6  | 1  | 사냥 이벤트 시작 전환 |
| TC7  | 1  | 미션 패스 신규 시작 |
| TC8  | 1  | 미션 패스 종료 처리 |
| TC9  | 1  | FillPacketInfo 호출 |
| TC10 | 3  | EventLoginSyncer / EventInfoPacketBuilder nullptr 안전 처리 |
| TC11 | 3  | 핸들러 미등록 타입 경계 확인 (BattlePass / Gacha / FullBanner) |
| TC12 | 1  | RepeatAttendance MaxStep 리셋 감지 |
| **합계** | **33** | |
