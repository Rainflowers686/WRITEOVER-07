# PVS-01 GOLD Final Report — WRITEOVER-07

This is the current Gold closure report for the bounded PVS-01 route. It
replaces the earlier Silver receipt as the current report; no claim is made
that future M1-M6 content is already complete.

## Release receipt

```text
START_HEAD = 33e5e73c3f8845ed7236f749d0392fa9d53f59918
FINAL_HEAD = 58487cffea0825f92a73b8321d6ce21d2d0a3473 (implementation head; final docs receipt SHA follows in task handoff)
PREVIOUS_VICTORY = SILVER
FINAL_VICTORY = GOLD
REPOSITORY = Rainflowers686/WRITEOVER-07
BRANCH = main
ROUTE = B1 -> Calibration -> Service/Medical -> 1F Security -> Staff/Elevator
```

The report is deliberately written so the final handoff remains authoritative
for the final commit SHA and remote Actions run, which cannot be known before
the commit is created and pushed.

## User-visible Gold gates

```text
REAL_PLAY_DIRECT_CONTROL = PARTIAL (production deterministic replay PASS; no reliable direct CUA route in this environment)
REAL_PLAY_NORMAL = PASS
REAL_PLAY_AGGRESSIVE = PASS
REAL_PLAY_STEALTH_SYSTEMIC = PASS
VISUAL_FIRST_IMPRESSION = PASS (dark industrial Half-block production frame; localized props/NPCs/viewmodel; no bright large-area floor)
GAME_FEEL = PASS for scoped PVS (pistol spread/recoil/muzzle/hit, non-lethal stun, sprint motion, bounded explosion/impact feedback)
NPC_BELIEVABILITY = PASS for bounded runtime loop; not a claim of the full future M5 planner
WORLD_BELIEVABILITY = PASS for the authored B1 -> 1F route; full-building content intentionally deferred
AUDIO_QUALITY = PASS on Windows procedural WinMM SFX/VO cue; POSIX subtitles-only fallback is explicit
FULL_HUMAN_NPC_COUNT = 1
SEMI_HUMAN_NPC_COUNT = 4 active runtime NPCs
AUTONOMOUS_NPC_LOOP = PASS (real Observe -> Remember -> Evaluate -> Choose -> Act receipts; 221+ loops in final routes)
SYSTEMIC_ROUTE_COUNT = 4 (normal, aggressive, stealth, systemic)
PISTOL = PASS
NARRATOR = PASS (large staged block-glyph typography plus reduced-flicker/reduced-shake fallback)
SAVE_LOAD = PASS (seven real sections; fail-closed load)
WINDOWS_X64 = PASS locally
MACOS_ARM64 = CI Debug build/test/smoke plus Release benchmark gate; local physical runtime NOT_RUN / UNVERIFIED
LINUX_X64 = PASS locally
KUNPENG_ARM64 = PASS cross-compile/link gate; target-host runtime UNVERIFIED
CROSS_PLATFORM_SAVE = PASS (Windows Release save loaded by Linux Release)
```

## Technical validation

```text
CPP_TESTS = PASS (161 tests, 0 failed; intentional harness self-check assertions are not failures)
PYTHON_CONTENT_TESTS = PASS (7/7)
SYSTEMIC_SCHEMA_TESTS = PASS (10/10)
CONTENT_CHECK = PASS
SYSTEMIC_SCHEMA = PASS
CONTRACT_CHECK = PASS
STATIC_AUDIT = PASS (COUNT=0)
INVALID_SEED_GATE = PASS
SMOKE = PASS
BENCH = PASS locally; final remote result is the GitHub Actions receipt
CI = PASS (GitHub Actions run 33844911347; head 58487cffea0825f92a73b8321d6ce21d2d0a3473; all five jobs success)
```

The content compiler uses the actual JSON authoring data and emits bounded
`data/npcs/npcs.bin`; runtime loads that compiled binary rather than parsing
NPC JSON. The schema gate rejects invalid IDs, enums, ranges, capacities,
references, location mismatches and non-empty reserved fields.

## Performance receipt

```text
TOTAL_FRAME_P99 = 1.311 ms worst-1%-average on Windows Release integrated PVS proxy; platform device writes excluded
RENDER_P99 = 1.007 ms worst-1%-average on Windows Release
SYSTEMIC_P99 = 0.178 ms worst-1%-average for the declared systemic update workload on Windows Release
TOTAL_CPU_BUDGET = PASS (integrated proxy <= 6.0 ms)
SYSTEMIC_BUDGET = PASS (declared lookup/update workloads <= 2.0 ms)
RENDER_BUDGET = PASS (declared production render <= 3.0 ms)
```

The integrated frame proxy runs the actual bounded NPC adapter, EventBus
dispatch, production 240x134 raster, 25 NPC sprite draws, weapon viewmodel,
half-block composition and ANSI encoding. It is a current PVS/foundation
proxy, not a guarantee for future full M5/M6 content. Linux Release reruns and
the remote matrix are the final host-specific receipts.

## Runtime and systemic evidence

- Final Windows Release normal replay: route reaches `room_1f_security`,
  `NPC_COUNT=5`, `FULL_NPC_COUNT=1`, `SEMI_NPC_COUNT=4`,
  `AUTONOMOUS_LOOPS=221`, `DISCOVERY_RESPONSES=1`, `SAVE_OK=YES`,
  `LOAD_OK=YES`, `NARRATOR_TYPOGRAPHY_ACTIVE=YES`.
- Final Windows Release aggressive replay: same route and population,
  `AUTONOMOUS_LOOPS=247`, `SAVE_OK=YES`, `LOAD_OK=YES`, event journal and
  systemic event counts non-zero.
- Final Windows Release stealth replay: same route and population,
  `AUTONOMOUS_LOOPS=394`, `SAVE_OK=YES`, `LOAD_OK=YES`, body searched and
  concealment/discovery response present.
- Final Windows Release systemic replay: same route and population,
  `AUTONOMOUS_LOOPS=221`, `DISCOVERY_RESPONSES=1`, `SAVE_OK=YES`,
  `LOAD_OK=YES`, narrator typography active.
- Linux Release systemic replay loaded the production profile binary, reached
  the same route, reported `REPLAY_RESULT=PASS`, `NPC_COUNT=5` with one Full
  and four SemiHuman, `AUTONOMOUS_LOOPS=221`, `DISCOVERY_RESPONSES=1`,
  completed Save/Load and used `posix-subtitles-only`.
- A Windows-produced `saves/pvs_manual.wo07` was loaded by Linux Release using
  the cross-platform replay script. This is an actual byte-compatible
  save/load check, not just a serializer unit test.

## Red-team result

```text
FINAL_CODE_FATAL = 0
FINAL_CODE_MAJOR = 0
FINAL_EFFECT_FATAL = 0
FINAL_EFFECT_MAJOR = 0
FINAL_PERFECTION_P0 = 0
FINAL_PERFECTION_P1 = 0
```

The final code review specifically checked bounded parsing, duplicate IDs,
source-of-truth ownership, event/report separation, provenance, body drag and
discovery seams, AI runtime wiring, profile compilation, replay determinism,
save section validation, platform selection and dependency direction.

The final effect review checked the production Half-block frame, dark
luminance bands, weapon anchor, muzzle/recoil/hit feedback, bounded shake and
flash behavior, narrator typography, reduced-flicker/reduced-shake behavior,
and Windows/POSIX audio policy. The final perfection review found no scoped P0
or P1 issue; it retains the explicit controlled limits rather than calling
procedural content a large external art/audio production.

## Most likely first-time-player complaint

`MOST_LIKELY_USER_COMPLAINT = Direct human-like play and a physical macOS run were not observed in this environment; no known functional blocker remains for the deterministic PVS route.`

The next human action is a bounded playtest of the four routes and presentation
readability. It is not authorization to start new game development.

## Controlled defers

```text
TOP_3_REMAINING_QUALITY_GAPS =
1. Direct human keyboard/mouse route observation is still PARTIAL; replay is PASS.
2. Physical macOS arm64 runtime and Kunpeng runtime are external-host evidence, while CI/cross-link gates are in scope and recorded separately.
3. Formal large-scale art/audio asset expansion and complete future M5/M6 content remain outside this bounded PVS target.

DEFERRED_MANDATORY = physical macOS arm64 runtime observation; direct human playtest observation
DEFERRED_MEDIUM = future M5/M6 population/content and formal hand-authored asset expansion
DEFERRED_STRETCH = full-building 41-floor authored playable population and broader platform device polish
```

These are controlled scope/availability defers with owners and revisit
conditions in `PVS01_GOAL_LEDGER.md`; they are not hidden open Fatal/Major
findings for PVS-01 Gold.

## Final verdict

```text
CROSS_PLATFORM_COURSE_GATE = PASS (remote head equals local implementation head; all required CI jobs green)
PVS01_GOLD = YES
PVS01_VERDICT = GOLD_READY_FOR_HUMAN_PLAYTEST
```

The final task handoff supplies the exact final SHA, remote HEAD equality,
Actions conclusion and cleanup receipts. After that handoff, stop. Do not
begin M5/M6 implementation, framework expansion, or unrelated refactoring.
