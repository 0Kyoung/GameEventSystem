#pragma once
#include <cstdint>

namespace GameEvent
{
    // 이벤트 카테고리 - 시간 체크 방식 분류
    enum class EventCategory
    {
        None = 0,
        FixedTime,      // 고정 시작/종료 시간 (출석, 누적출석 등)
        RepeatCycle,    // 반복 주기 이벤트 (초상화, 호위대행사, 빙고)
        UserDefined,    // 유저 정의 시간 (사냥, 교환)
        MissionPass,    // 미션 패스
        SiegeEve,       // 공성 전야
    };

    // 이벤트 타입
    enum class EventType : uint32_t
    {
        None = 0,

        // 출석 계열
        RepeatAttendance                    = 101,
        SpecialAttendance                   = 102,
        CumulativeAttendance                = 103,
        Attendance                          = 110,
        ContinuityAttendance                = 111,
        Attendance14Days                    = 112,
        AttendanceNewAndReturn7Days         = 120,
        AttendanceNewAndReturn14Days        = 121,

        // 목표 계열
        RelationQuestGoal                   = 201,
        LevelGoal                           = 202,
        GoldChargeGoal                      = 211,
        GoldUseGoal                         = 212,

        // 배틀패스 계열
        BattlePass                          = 301,
        BattlePassWooyeogi                  = 302,
        BattlePassGather                    = 304,
        BattlePassHoryong                   = 310,
        BattlePassShinryong                 = 311,
        BattlePassBongbong                  = 312,

        // 가챠/복덕 계열
        Gacha                               = 401,
        Benediction                         = 402,
        GachaGoal                           = 403,

        // 공성 전야
        SiegeEve                            = 501,

        // 반복 이벤트 계열
        Portrait                            = 601,
        EventPassSangbaek                   = 701,
        EscortAgency                        = 801,

        // 사냥 계열
        Hunting                             = 901,
        HuntingToExchange                   = 902,

        // 기타
        FullBanner                          = 1001,
        Bingo                               = 1101,

        // 미션 패스
        MissionPassGrowth                   = 2001,
        MissionPassChallenge                = 2002,
    };

    // 이벤트 시작/종료 조건 타입
    enum class StartCondition : uint32_t
    {
        Always  = 0,
        Time    = 1,
        TimeGM  = 4,
    };

    enum class CloseCondition : uint32_t
    {
        None    = 0,
        Time    = 1,
        TimeGM  = 4,
    };

    // 이벤트 상태 변경 타입
    enum class StateChangeType
    {
        None = 0,
        Start,
        Close,
    };

} // namespace GameEvent
