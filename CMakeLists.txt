cmake_minimum_required(VERSION 3.14)
project(GameEventSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── 소스 파일 ──────────────────────────────────────────────────────────────────
set(HANDLER_SOURCES
    Handlers/AttendanceEventHandler.cpp
    Handlers/BingoEventHandler.cpp
    Handlers/MissionPassEventHandler.cpp
    Handlers/SiegeEveEventHandler.cpp
    Handlers/HuntingEventHandler.cpp
    Handlers/BenedictionEventHandler.cpp
)

set(CORE_SOURCES
    Core/EventHandlerRegistry.cpp
)

# ── 테스트 실행파일 ────────────────────────────────────────────────────────────
add_executable(EventSystemTest
    Test/main.cpp
    ${HANDLER_SOURCES}
    ${CORE_SOURCES}
)

target_include_directories(EventSystemTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 경고 옵션
if(MSVC)
    target_compile_options(EventSystemTest PRIVATE /W4)
else()
    target_compile_options(EventSystemTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
