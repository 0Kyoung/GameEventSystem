#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Core/GameTypes.h 와 동일한 방식의 빌드 스위치.
//
// 포트폴리오/테스트 빌드:  -DDB_TEST   → MockDb (인메모리, 지연시간 흉내)
// 실제 게임 서버 빌드:     플래그 없음 → MySQL C API 기반 커넥션 풀
//
// EventDbBridge 이상의 상위 계층은 이 스위치를 전혀 신경 쓰지 않는다.
// QueryUserEvents() 하나만 알면 되도록 인터페이스를 좁혀 두었다.
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>
#include <ctime>
#include <vector>

namespace GameDb
{
    // user_event 테이블 한 행
    // (실제 스키마: uid, event_id, event_step, sub_event_step, event_deleted, last_update_time)
    struct DbEventRow
    {
        uint32_t event_id          = 0;
        int32_t  event_step        = 0;
        int32_t  sub_event_step    = 0;
        bool     event_deleted     = false;
        time_t   last_update_time  = 0;
    };

    // 워커 스레드에서 호출되는 동기(블로킹) 쿼리 함수.
    // 호출부(AsyncDbJobQueue)가 이미 워커 스레드 컨텍스트이므로
    // 여기서 블로킹 I/O를 수행해도 게임 스레드는 막히지 않는다.
    std::vector<DbEventRow> QueryUserEvents(uint64_t uid);

} // namespace GameDb
