# 06_SIMULATION_CLOCK_AND_DETERMINISM

## Frozen Simulation Semantics

- **Simulation rate**: fixed 120Hz, ALL quality presets
- **Frame index**: uint64_t, monotonically increasing, one per sim tick
- **Sim time**: `frameCount * (1/120)` seconds
- **No wall clock** in gameplay logic (only performance measurement / non-deterministic UI)

```cpp
// --- Fixed-step accumulator / wait policy ---
class MainLoop {
public:
    // Wall-clock measurement only. Does NOT drive gameplay.
    void Run();

private:
    SimClock clock_;   // Deterministic sim clock
    // wall-time accumulator between frames (render pacing)
};

// Engine loop pseudocode:
//   while (running) {
//       pollInput();
//       while (accumulator >= fixedDt) {   // run all needed sim ticks
//           simRng_  → sim systems tick at 120Hz;
//           clock_.Tick();
//           accumulator -= fixedDt;
//       }
//       render();   // reads current sim state (may interpolate)
//       present();
//       waitForNextFrame();  // sleep_until based on frame budget
//   }
```

## Render Interpolation

Render reads sim snapshots. For smooth visuals at arbitrary display rates:
- Store previous and current sim state snapshots.
- Interpolate `alpha = accumulator / fixedDt` between them for player camera and NPC positions.
- **No gameplay decisions use interpolated values** — only visual projection.

## AI Sub-Rate Scheduling

| System | Rate | Every Nth sim tick |
|--------|------|-------------------|
| Full NPC GOAP re-plan | ≤5Hz | 1/24 |
| Full NPC perception/memory | 30–60Hz | 1/2–1/4 |
| Medium NPC FSM | 30Hz | 1/4 |
| Light NPC FSM | 15Hz | 1/8 |
| Off-screen NPC simulation | 1–2Hz | 1/60–1/120 |

These rates are internal to the AI module; the clock stays at 120Hz.

## Determinism Policy

**Definition**: Same build, same seed, same input sequence → identical world state (all sim-relevant state), frame for frame.

### Scope
- Gameplay-relevant decisions: player controller, collisions, AI, events, damage, narrative.
- Visual-only effects (particle jitter, muzzle flash, screen shake magnitude when purely cosmetic): MAY use visual RNG.
- Float determinism: guaranteed within one build/compiler for identical code paths. Not guaranteed cross-compiler bit-for-bit (documented limitation).

### Requirements
1. Single simulation RNG (XorShift128+) owned by WorldSim. Full 128-bit state saved/loaded.
2. No unordered iteration over containers whose iteration order affects decisions.
   - Use `std::map` / sorted `std::vector` / explicit priority queue with tie-breakers for any state-dependent iteration.
3. No wall-clock in gameplay.
4. Event dispatch is deterministic (see 07 document).
5. Content load order is deterministic (sorted by stable ID).
6. Deterministic replay test: run N frames with seed S → snapshot. Reload from snapshot, run N more frames → compare. Must match exactly.

## RNG Ownership

```cpp
class WorldSim {
    DeterministicRNG simRng_;   // Deterministic gameplay decisions
    // ...
};

class RenderSystem {
    DeterministicRNG visualRng_;  // Cosmetic effects only, never saved
    // ...
};
```

## 报告友好

**STL**: std::chrono for wall-clock measurement in the loop; no usage for sim.
**Design Pattern**: Fixed Timestep Game Loop (classic).
**Core Algorithm**: Fixed-step accumulator.
**Course Note**: 固定时间步长是为什么"所有画质预设下游戏物理一致"的根本原因——Simulation 恒为 120Hz。
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
