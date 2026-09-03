cmake_minimum_required(VERSION 3.20)

project(writeover
    VERSION 0.1.0
    DESCRIPTION "WRITEOVER-07 terminal character-3D FPS foundation"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global policy
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(WO_ENABLE_DEV "Build with developer panel / debug commands" OFF)
option(WO_BUILD_TESTS "Build unit test target" ON)
option(WO_BUILD_TOOLS "Build tools (bench, mapc, contentc)" ON)

include(cmake/compiler_flags.cmake)

set(WO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------------
# Libraries (dependency direction: common <- world/player <- ai <- narrative
#                       core <- all via composition root, render <- snapshots)
# ---------------------------------------------------------------------------
add_library(writeover_common STATIC
    src/common/result.cpp
    src/common/serialize.cpp
    src/common/rng.cpp
    src/common/logging.cpp
    src/common/debug.cpp
    src/common/io.cpp
    src/common/clock.cpp
    src/common/world_event.cpp
)
target_include_directories(writeover_common PUBLIC ${WO_INCLUDE_DIR})
writeover_configure_target(writeover_common)

# Systemic Gameplay Foundation: cross-module POD contracts and the minimal
# world kernel. It depends only on common so it can be embedded in any module
# without forcing AI/narrative cycles.
add_library(writeover_systemic STATIC
    src/systemic/systemic.cpp
)
target_include_directories(writeover_systemic PUBLIC ${WO_INCLUDE_DIR})
target_link_libraries(writeover_systemic PUBLIC writeover_common)
writeover_configure_target(writeover_systemic)

add_library(writeover_world STATIC
    src/world/grid.cpp
    src/world/room.cpp
    src/world/puzzle.cpp
    src/world/infrastructure.cpp
    src/world/map_validator.cpp
    src/world/fact_belief.cpp
)
target_include_directories(writeover_world PUBLIC ${WO_INCLUDE_DIR})
target_link_libraries(writeover_world PUBLIC writeover_common)
writeover_configure_target(writeover_world)

add_library(writeover_player STATIC
    src/player/controller.cpp
    src/player/weapon.cpp
    src/player/combat.cpp
    src/player/input_mapper.cpp
    src/player/input_runtime.cpp
)
target_include_directories(writeover_player PUBLIC ${WO_INCLUDE_DIR})
target_link_libraries(writeover_player PUBLIC writeover_common writeover_world)
writeover_configure_target(writeover_player)

add_library(writeover_ai STATIC
    src/ai/npc.cpp
    src/ai/perception.cpp
    src/ai/goap.cpp
    src/ai/memory.cpp
)
target_include_directories(writeover_ai PUBLIC ${WO_INCLUDE_DIR})
target_link_libraries(writeover_ai PUBLIC writeover_common writeover_world writeover_player)
writeover_configure_target(writeover_ai)

add_library(writeover_narrative STATIC
    src/narrative/storylet.cpp
    src/narrative/narrator.cpp
    src/narrative/causality.cpp
    src/narrative/judge.cpp
    src/narrative/dialog.cpp
)
target_include_directories(writeover_narrative PUBLIC ${WO_INCLUDE_DIR})
target_link_libraries(writeover_narrative PUBLIC writeover_common writeover_world writeover_player writeover_ai)
writeover_configure_target(writeover_narrative)

add_library(writeover_render STATIC
    src/render/raycaster.cpp
    src/render/reference_renderer.cpp
    src/render/production_renderer.cpp
    src/render/frame_encoder.cpp
    src/render/utf.cpp
    src/render/terminal_backend.cpp
    src/render/hud.cpp
    src/render/benchmark.cpp
)
target_include_directories(writeover_render PUBLIC ${WO_INCLUDE_DIR})
target_link_libraries(writeover_render PUBLIC writeover_common writeover_world writeover_player)
writeover_configure_target(writeover_render)

# Core owns: engine loop, save, settings, profile, console guard.
# Core depends ONLY on common. Cross-module wiring happens in writeover_app
# (composition root), not inside core. (Fixes M-002.)
add_library(writeover_core STATIC
    src/core/engine.cpp
    src/core/save.cpp
    src/core/settings.cpp
    src/core/profile.cpp
    src/core/console.cpp
)
target_include_directories(writeover_core PUBLIC ${WO_INCLUDE_DIR})
target_link_libraries(writeover_core PUBLIC writeover_common)
writeover_configure_target(writeover_core)

# Platform implements platform-facing interfaces (input/terminal/atomic-io).
# It may depend on interface-only headers; semantic modules never include it.
add_library(writeover_platform STATIC
    src/platform/windows/win_console.cpp
    src/platform/windows/win_input.cpp
    src/platform/windows/win_raw_input.cpp
    src/platform/windows/win_terminal.cpp
    src/platform/windows/win_audio.cpp
    src/platform/windows/win_file_io.cpp
)
target_include_directories(writeover_platform PUBLIC ${WO_INCLUDE_DIR})
target_include_directories(writeover_platform PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(writeover_platform PUBLIC writeover_common writeover_render writeover_player writeover_core)
writeover_configure_target(writeover_platform)

# ---------------------------------------------------------------------------
# Application / Composition Root (M1 owns this executable)
# ---------------------------------------------------------------------------
add_executable(writeover_app
    src/app/game_main.cpp
    src/app/composition_root.cpp
)
target_include_directories(writeover_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(writeover_app PRIVATE
    writeover_core
    writeover_world
    writeover_player
    writeover_ai
    writeover_narrative
    writeover_render
    writeover_platform
    writeover_systemic
)
writeover_configure_target(writeover_app)
target_include_directories(writeover_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/app)

# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------
if(WO_BUILD_TOOLS)
    add_executable(writeover_bench tools/bench/main.cpp)
    target_link_libraries(writeover_bench PRIVATE writeover_common writeover_systemic writeover_world writeover_player writeover_render)
    writeover_configure_target(writeover_bench)

    add_executable(mapc tools/mapc/main.cpp)
    target_link_libraries(mapc PRIVATE writeover_common writeover_world)
    writeover_configure_target(mapc)

    add_executable(writeover_input_probe tools/input_probe/main.cpp)
    target_link_libraries(writeover_input_probe PRIVATE
        writeover_core writeover_player writeover_platform)
    writeover_configure_target(writeover_input_probe)
    target_include_directories(writeover_input_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# ---------------------------------------------------------------------------
# Tests (CTest)
# ---------------------------------------------------------------------------
if(WO_BUILD_TESTS)
    enable_testing()
    add_executable(writeover_tests
        tests/test_harness.cpp
        tests/test_main.cpp
        tests/test_common.cpp
        tests/test_core.cpp
        tests/test_render.cpp
        tests/test_player.cpp
        tests/test_input.cpp
        tests/test_world.cpp
        tests/test_ai.cpp
        tests/test_narrative.cpp
        tests/test_save_replay.cpp
        tests/test_systemic.cpp
    )
    target_include_directories(writeover_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${WO_INCLUDE_DIR}
    )
    target_link_libraries(writeover_tests PRIVATE
        writeover_common
    writeover_systemic
        writeover_core
        writeover_world
        writeover_player
        writeover_ai
        writeover_narrative
        writeover_render
    )
    writeover_configure_target(writeover_tests)
    add_test(NAME unit COMMAND writeover_tests)
endif()

# ---------------------------------------------------------------------------
# Headers must compile standalone (public contract hygiene)
# ---------------------------------------------------------------------------
if(WO_BUILD_TESTS)
    file(GLOB_RECURSE WO_PUBLIC_HEADERS ${WO_INCLUDE_DIR}/writeover/*.h)
    foreach(WO_HDR ${WO_PUBLIC_HEADERS})
        get_filename_component(WO_NAME ${WO_HDR} NAME)
        add_library(header_${WO_NAME} OBJECT ${WO_HDR})
        target_include_directories(header_${WO_NAME} PRIVATE ${WO_INCLUDE_DIR})
        writeover_configure_target(header_${WO_NAME})
        target_link_libraries(header_${WO_NAME} PRIVATE
            writeover_common writeover_systemic writeover_core writeover_world writeover_player
            writeover_ai writeover_narrative writeover_render)
    endforeach()
endif()

message(STATUS "WRITEOVER-07 foundation configured")
