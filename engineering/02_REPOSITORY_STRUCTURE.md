# 02_REPOSITORY_STRUCTURE

## Directory Layout

```
WRITEOVER-07/
├── CMakeLists.txt              # Root CMake (include/cmake all targets)
├── CMakePresets.json           # Build presets (debug/release/ci)
├── .editorconfig               # Editor consistency
├── .clang-format               # Formatting rules
├── .gitignore                  # Ignore build/, out/, saves/, logs/
├── .github/workflows/ci.yml    # CI: configure+build+test on push/PR
├── cmake/
│   └── writeover_common.cmake  # Shared CMake helpers (warnings, /utf-8)
├── include/writeover/          # PUBLIC headers (frozen after 9/3)
│   ├── common/
│   │   ├── types.h             # Basic shared types (Coord, Color, etc.)
│   │   ├── ids.h               # Strong typed IDs
│   │   ├── math.h              # Vec2, Vec3, geometry helpers
│   │   ├── rng.h               # DeterministicRNG
│   │   ├── world_event.h       # WorldEvent + EventBus
│   │   ├── result.h            # Result<T> error type
│   │   ├── serialize.h         # Serializer/Deserializer
│   │   ├── clock.h             # SimClock (fixed 120Hz)
│   │   ├── debug.h             # DebugMetrics interface
│   │   └── logging.h           # Logger interface
│   ├── core/
│   │   ├── engine.h            # Main loop / app lifecycle
│   │   ├── save.h              # Save/Load
│   │   ├── settings.h          # Settings registry
│   │   ├── profile.h           # Profile metadata
│   │   └── console.h           # Terminal guard (RAII)
│   ├── render/
│   │   ├── raycaster.h         # Height-Span Raycaster
│   │   ├── terminal_backend.h  # CharCell + ITerminalBackend
│   │   ├── hud.h               # HUD rendering
│   │   └── benchmark.h         # Performance benchmark
│   ├── player/
│   │   ├── controller.h        # Player controller + postures
│   │   ├── input.h             # Input backends + mapper
│   │   ├── weapon.h            # Weapon data + firing
│   │   └── combat.h            # Combat/damage system
│   ├── world/
│   │   ├── grid.h              # GridCell + grid data
│   │   ├── room.h              # Room template/loading
│   │   ├── puzzle.h            # Puzzle system
│   │   ├── infrastructure.h    # Power/doors/cameras
│   │   └── map_validator.h     # Map validation
│   ├── ai/
│   │   ├── npc.h               # NPC data + state machine
│   │   ├── goap.h              # GOAP-lite planner
│   │   ├── perception.h        # NPC perception
│   │   ├── fact_belief.h       # WorldFact/Belief/Claim
│   │   └── memory.h            # NPC memory
│   └── narrative/
│       ├── storylet.h          # Storylet engine
│       ├── narrator.h          # Narrator system
│       ├── causality.h         # Causality ledger
│       ├── judge.h             # Judge mode
│       └── dialog.h            # Dialog wheel
├── src/
│   ├── core/                   # Implementation
│   ├── render/
│   ├── player/
│   ├── world/
│   ├── ai/
│   ├── narrative/
│   └── platform/windows/       # Win32 impl (RawInput, Console, SEH, Audio)
├── tests/
│   ├── test_harness.h          # Minimal concrete test harness
│   ├── test_core.cpp
│   ├── test_render.cpp
│   ├── test_player.cpp
│   ├── test_world.cpp
│   ├── test_ai.cpp
│   ├── test_narrative.cpp
│   └── test_main.cpp
├── tools/
│   ├── mapc/                   # Map validator tool
│   ├── bench/                  # Benchmark runner
│   └── contract_check/         # Public contract drift check
├── data/
│   ├── rooms/                  # room_XX.json
│   ├── npcs/                   # npc_XX.json
│   ├── facts/                  # facts.json
│   └── storylets/              # storylets.json
├── scripts/
│   ├── bootstrap.ps1           # One-time setup (check toolchain)
│   ├── build.ps1               # Configure + build
│   ├── test.ps1                # Build + run tests
│   ├── bench.ps1               # Build + run benchmark
│   ├── smoke.ps1               # Build + run smoke test
│   └── package.ps1             # Create release zip
└── docs/
    ├── adr/                    # Architecture Decision Records
    └── (engineering docs mirror)
```

## Ownership Rules

- Each module owns ONLY its own directory under `src/<module>/` and `include/writeover/<module>/`
- M1 owns: engine, save, settings, profile, console, common/clock
- M2 owns: render/ (raycaster, terminal_backend, hud, benchmark)
- M3 owns: player/ (controller, input, weapon, combat)
- M4 owns: world/ (grid, room, puzzle, infrastructure, map_validator)
- M5 owns: ai/ (npc, goap, perception, fact_belief, memory)
- M6 owns: narrative/ (storylet, narrator, causality, judge, dialog)
- Common owns: types, ids, math, rng, world_event, result, serialize, debug, logging

## Public Include Boundary

- Only `include/writeover/` is public
- No module includes another module's private `src/` files
- All cross-module includes use `<writeover/...>` public headers
- Platform-specific code lives in `src/platform/windows/` and is not public

## Dependency Direction

```
common ← everything
core ← world/player/ai/narrative/render (they use common, but not core's private)
world ← player, ai (they query world)
player ← world (collision queries)
ai ← world, player (perception)
narrative ← world, ai, player, (reads state, posts events)
render ← world, player (read-only snapshots)
platform/windows ← outermost
```

No cyclic dependencies allowed. Verified by `tools/contract_check/check_deps.py`.
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
