#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 빌드 환경에 따라 실제 게임 서버 타입 또는 Mock 타입을 선택
//
// 포트폴리오/테스트 빌드:  -DGAME_EVENT_TEST 플래그 사용
// 실제 게임 서버 빌드:     해당 플래그 없이 프로젝트 헤더 포함
// ─────────────────────────────────────────────────────────────────────────────

#ifdef GAME_EVENT_TEST
    #include "../Test/MockTypes.h"
#else
    // 실제 게임 서버 빌드 시 실제 타입 헤더 포함
    // #include "GameServer/Game/Entity/User.h"
    // #include "GameServer/Game/ZoneGroup/ZoneGroup.h"
    // #include "GameServer/Game/Character/UserEvent.h"
    // #include "Common/Resource/_res_user_event.h"
#endif
