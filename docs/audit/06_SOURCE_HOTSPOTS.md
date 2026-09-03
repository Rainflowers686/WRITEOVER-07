# Source Hotspots — Fable 优先阅读

```text
repo_seed/tests/test_harness.h
    WO_CHECK macros: failure does not fail test.

repo_seed/tests/test_player.cpp
    InputMapperRebind currently contains hidden failed assertion.

repo_seed/src/player/input_mapper.cpp
    first-match global mapping; no contexts.

repo_seed/src/platform/windows/win_input.cpp
    key-up ignored; FlushConsoleInputBuffer; cursor delta unavailable through interface.

repo_seed/src/core/engine.cpp
    no EventBus::Dispatch in loop.

repo_seed/src/common/world_event.cpp
    Post always next_pending; dispatch timing needs decision.

repo_seed/src/app/composition_root.cpp
    fake frame causality; Storylet failure silently empties engine;
    renderer player view set once; smoke save only RNG/events.

repo_seed/src/narrative/storylet.cpp
    content/runtime state format conflated; WorldCommandAction marker-only;
    Load expects fired_count.

repo_seed/tools/contentc/contentc.py
    --check unused; RoomId hardcoded 1; NPC/storylet refs empty;
    worldcommand marker-only.

repo_seed/src/core/save.cpp
    section_count >= Count rejects all 7 valid sections.

repo_seed/src/player/controller.cpp
    no ground support probe; no step-up; lean state only.

repo_seed/src/render/raycaster.cpp
    only far floor rise / far ceiling drop boundary segments;
    corner tie only X.

repo_seed/src/platform/windows/win_terminal.cpp
    per-cell full SGR; no run/delta encoder; fallback palette incomplete;
    last fallback uninitialized.

repo_seed/tools/bench/main.cpp
    raycast-only benchmark, no terminal present.

repo_seed/src/world/map_validator.cpp
    `uint8_t light > 255` impossible check.
```
