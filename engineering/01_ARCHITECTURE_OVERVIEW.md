# 01_ARCHITECTURE_OVERVIEW

## System Overview

WRITEOVER-07 is a terminal-based character 3D FPS running on Windows. The architecture is organized into 6 modules plus a common module and platform layer.

## Module Map

```
┌─────────────────────────────────────────────────────────────┐
│                     Engine (M1 - Core)                       │
│  Main Loop │ Save/Load │ Settings │ Profile │ Dev Panel     │
└──────────┬──────────────────────────────────────┬───────────┘
           │ Events (EventBus)                    │
           ▼                                      ▼
┌──────────────────┐  ┌──────────────────────────────────────┐
│  Player (M3)     │  │  World (M4)                           │
│  Controller      │  │  Grid │ Room │ Puzzle │ Infra       │
│  Input │ Combat  │  │  Map Validator                        │
│  Weapon          │  └──────────────────┬───────────────────┘
└──────────────────┘                     │
           │                             │
           ▼                             ▼
┌──────────────────┐  ┌──────────────────────────────────────┐
│  AI (M5)         │  │  Narrative (M6)                       │
│  NPC │ GOAP      │  │  Storylet │ Narrator │ Causality    │
│  Perception │    │  │  Judge │ Dialog                       │
│  Fact/Belief     │  └──────────────────────────────────────┘
└──────────────────┘
           │                             │
           └──────────────┬──────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Render (M2)                                                │
│  Raycaster │ Terminal Backend │ HUD │ Benchmark             │
│  ANSI / Win32 Backend                                       │
└─────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Platform/Windows (outermost layer)                          │
│  RawInput │ Console API │ SEH │ Audio                       │
└─────────────────────────────────────────────────────────────┘
```

## Module Responsibilities

| Module | Responsibility | Key Contracts |
|--------|---------------|---------------|
| M1 Core | Main loop, save/load, settings, profile, dev panel | engine.h, save.h, settings.h |
| M2 Render | Raycaster, terminal backends, HUD, performance benchmark | raycaster.h, terminal_backend.h |
| M3 Player | Player controller, collision, input, weapons, combat | controller.h, input.h, weapon.h |
| M4 World | Grid data, rooms, puzzles, infrastructure, data validation | grid.h, room.h, infrastructure.h |
| M5 AI | NPCs, GOAP planner, perception, fact/belief, memory | npc.h, goap.h, fact_belief.h |
| M6 Narrative | Storylet engine, narrator, causality ledger, judge, dialog | storylet.h, narrator.h, judge.h |

## Communication Patterns

1. **Command/Action**: Player requests world change (e.g., "move forward", "fire weapon")
2. **WorldEvent**: Something that happened in the world (e.g., "door opened", "NPC died")
3. **Query/View**: Read-only interface for getting state (e.g., collision query, render snapshot)
4. **Presentation**: What the UI shows (e.g., HUD, subtitles, narrator text)

## Data Flow (Per Tick at 120Hz)

```
1. Input Poll: Read raw input, map to GameActions
2. Sim Tick: Player controller → collision check → world state → AI → narrative → events
3. Event Dispatch: Process all WorldEvents, fan-out to all modules
4. Render Snapshot: Collect read-only state for rendering
5. Render Frame: Raycaster → span → terminal buffer → submit
6. Present: Display buffer to terminal (sync or async)
```

## Directory Layout

```
include/writeover/   → Public headers (frozen)
src/                 → Implementation
src/platform/windows → Win32-specific code
tests/               → Unit tests
tools/               → mapc, bench, contract_check
data/                → Room, NPC, fact, storylet data files
scripts/             → Build, test, bench, smoke, package scripts
docs/adr/            → Architecture Decision Records
```

## 报告友好

**OOA Objects**: Engine, Player, World, NPC, Narrator, Room, Weapon, Grid
**UML**: See module-specific class diagrams
**STL**: vector, variant, optional, array, unique_ptr, string, unordered_map, map
**Design Patterns**: Observer (EventBus), Strategy (InputBackend, TerminalBackend), State (NPC FSM), Command (GameAction → Event), Composite (Room → Grid cells)
**Core Algorithms**: DDA ray marching, XorShift128+ RNG, GOAP-lite planning, AABB collision, CRC32 checksum
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
