# ADR-0008: PVS-01 Gold Portable Runtime Closure

- 状态: 已实现
- 日期: 2026-09-04
- 决策者: Rainflowers686
- 受影响模块: M1, M2, M3, M4, M5, M6

## 背景

PVS-01 Gold requires evidence from the production composition root rather
than isolated foundation claims. The application needs one bounded autonomous
NPC adapter, real replayable input, complete runtime save/load validation, a
portable platform boundary, and authored NPC records that can be checked
against the systemic seed. These seams cross public headers, so they must be
recorded before the public-header snapshot is refreshed.

## 决策

- Keep `SystemicWorld` as the shared semantic kernel; the autonomous NPC loop
  remains a small composition adapter with the fixed Observe → Remember →
  Evaluate → Choose → Act phases.
- Separate NPC cognition from faction and role in the public runtime and
  authoring contracts, with `Full` and `SemiHuman` as the only cognition
  tiers.
- Expose portable audio, terminal, input, and atomic-file interfaces. Windows
  uses the existing Win32/WinMM path; POSIX uses ANSI/input fallbacks and an
  explicit subtitles-only audio backend.
- Persist and validate the Player, World, RNG, Events, AI, Narrative, and
  Systemic sections together. Deserializers remain bounded, deterministic, and
  fail-closed.
- Add deterministic replay traces and a small set of authored NPC profiles so
  normal, aggressive, stealth, and systemic routes can be independently
  reproduced without a second gameplay world.

## 备选方案

Keep the new behavior behind test-only fakes or add a second simulation world
for the Gold demonstration. This was rejected because it would not prove the
actual application wiring and would duplicate ownership of systemic state.

## 影响

The public header snapshot changes and is refreshed after this ADR. The
changes do not alter the 120 Hz fixed clock, add a thread, add a third-party
dependency, or introduce a new save section id. POSIX voice playback is
reported honestly as unavailable while subtitles remain authoritative.

## 回滚/回退

Revert this ADR together with the Gold runtime seam changes and restore the
previous public-header snapshot. Do not reset or clean the authoritative
worktree; remove only generated build and replay artifacts after verification.
