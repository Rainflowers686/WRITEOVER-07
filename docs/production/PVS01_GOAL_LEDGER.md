# PVS-01 GOAL LEDGER — WRITEOVER-07

## Mission State

- MISSION_START_TIME: 2026-09-03 23:51:42
- START_HEAD: 5dadd5f822e4668161bdf92fae8c5f057690995f
- LAST_CLEAN_HEAD_BEFORE_PVS: 667b7399850e8a6ed718f4e73c17cb5e1549f7eb
- CURRENT_HEAD: pending local PVS-01 release commit
- CURRENT_PHASE: final three red teams complete; Silver release candidate
- MISSION_STATUS: READY_FOR_HUMAN_PLAYTEST_WITH_SCOPE_LIMITS

## Grand Objective

Deliver one coherent vertical slice:

`B1 Wake -> Calibration / Logistics -> Service / Medical -> 1F Lobby -> Security Checkpoint -> Restroom / Staff route -> Elevator Lobby`

The route is intentionally small. It proves one readable authored path and
shared systemic consequences instead of pretending that the full building or
full NPC population is already implemented.

## Victory Ladder

- GOLD: complete, polished, stable and ready for human playtest with final art,
  effects, audio and full route recovery evidence.
- SILVER: the intended authored path and interactions work with known,
  non-fatal production-polish gaps. **Current level.**
- BRONZE: a reproducible production renderer and B1 micro-space only.

## Implemented Route

- B1 Revival: production half-block raster, opening subtitle, maintenance NPC
  line, incapacitated body, body search, badge theft, memory/relationship
  reaction, body drag, concealment cart, camera outage and terminal session.
- Calibration / Logistics: the existing `room_01_calibration` is now part of
  the normal route rather than an unreachable fallback-only room.
- Service / Medical: authored room, medical prop, route feedback and transition
  to the lobby.
- 1F Security: authored checkpoint, credential access, bribe exchange and
  maintenance/staff route knowledge.
- Restroom / Staff and Elevator Lobby: authored side route, door/elevator props,
  deterministic transitions and readable terminal subtitles.
- F5/F9 uses the real SaveManager path and serializes player, world, RNG,
  events, narrative and SystemicWorld sections. Systemic state is restored only
  after fail-closed parsing and room validation.

The runtime keeps one SystemicWorld. The authored callbacks are deliberately
small PVS wiring; they are not a second gameplay framework and do not replace
M3/M4/M5/M6 ownership.

## Final Validation Evidence

All commands below were run against the local PVS-01 working tree after the
last code change. `PASS` means the command returned success; no status is
derived from an earlier report.

- `cmake --preset debug`: PASS
- `cmake --build --preset debug --config Debug --parallel 4`: PASS
- `cmake --preset release`: PASS
- `cmake --build --preset release --config Release --parallel 4`: PASS
- `cmake --preset ci`: PASS
- `cmake --build --preset ci --config Debug --parallel 4`: PASS
- `ctest --preset debug --output-on-failure`: PASS
- `ctest --preset ci --output-on-failure`: PASS
- `out/build/release/Release/writeover_tests.exe`: PASS, 158 tests, 0 failed
  (the two printed assertion lines are intentional test-harness self-checks)
- `powershell -ExecutionPolicy Bypass -File scripts/smoke.ps1 -Preset debug`:
  PASS; unit test, six `mapc` room checks and bounded app exit all passed
- `powershell -ExecutionPolicy Bypass -File scripts/bench.ps1 -Preset release`:
  PASS; all declared budgets passed
- `powershell -ExecutionPolicy Bypass -File scripts/contract_check.ps1`: PASS
- `python tools/audit/static_audit.py .`: PASS, `COUNT=0`
- `python tools/contentc/contentc.py --data-dir data --out-dir data --check`:
  PASS; six rooms, four facts and two storylets deterministically recompiled
- `python tools/contentc/test_contentc.py`: PASS, 5/5
- `python tools/systemic/systemic_schema_check.py --data-dir data`: PASS
- `python tools/systemic/test_systemic_schema.py`: PASS, 5/5
- `python tools/systemic/test_runtime_invalid_seed.py`: PASS; invalid seed
  prevented startup
- seed compiler explicit-cognition migration probe: PASS; explicit v1.2
  `cognition` is honored and legacy `class` remains compatible

## Benchmarks

Release benchmark output:

- raycast column sweep worst1% average: 0.098 ms
- terminal full worst1% average: 0.263 ms; budget result PASS
- terminal delta worst1% average: 0.055 ms; budget result PASS
- terminal unchanged worst1% average: 0.035 ms; budget result PASS
- terminal worst-case safety: 0.234 ms worst1% average, 50,659 bytes; PASS
- systemic lookup worst1% average: 0.012 ms; PASS
- systemic update workload worst1% average: 0.185 ms; PASS
- PVS render workload `240x67` worst1% average: 0.981 ms; PASS
- overall declared benchmark budget: PASS

The systemic benchmark is a representative foundation workload covering current
relationship, memory, evidence, body drag/discovery, item, promise, quest,
search, exchange, terminal, observability and event-bridge records. It does not
claim to be a full future M5 AI decision benchmark.

## Visual / Input Evidence

- Six release frame dumps (`B1`, `Calibration`, `Service`, `1F`, `Restroom`,
  `Elevator`) exited 0 and each produced a `240x134` logical-pixel PPM of
  96,495 bytes.
- Visual review found dark industrial floor/ceiling bands, mid-dark walls,
  distinct cyan/amber authored props, occluded scene sprites and a bottom-right
  weapon viewmodel within the declared safe area. No large-area light-gray or
  white floor was observed.
- `writeover_input_probe.exe --backend auto --context gameplay --seconds 2`:
  PASS for `keyboard-console`, `raw-input`, `raw-input` mouse buttons,
  gameplay context and focus. The probe run did not inject a complete route;
  automated end-to-end key replay remains UNVERIFIED and is intentionally left
  for human playtest.
- Existing tracked golden samples remain in `evidence/pvs01/`. The temporary
  six-scene visual captures used for this review are cleanup artifacts, not
  product assets.

## Code Red Team

PASS with no Fatal or Major finding open.

- `HideBody` now exposes the missing semantic transition and emits typed
  `BodyHidden`; the body concealment test follows physical search -> revealed
  badge -> theft -> drag -> hide -> discovery observation -> explicit response.
- `DiscoverBody` records observation/evidence/memory only. Security escalation
  occurs only through `ApplyDiscoveryResponse`, so medical call, cover-up,
  ignore and report remain distinct outcomes.
- Duplicate IDs, vector/string bounds, enum/range validation, canonical
  observability-source mirroring, item provenance and fail-closed systemic seed
  loading are covered by code/tests.
- The seed compiler now honors explicit `cognition` and retains legacy class
  migration behavior.
- Public header drift was recorded in ADR-0007 and the contract baseline; the
  contract checker is green.
- No second gameplay world, broad framework, new branch, hook, PR or unrelated
  architecture change was introduced.

## Real-Play / Effects Red Team

PASS for deterministic bounded runtime scene generation; CONDITIONAL for full
human route play because this environment cannot provide a complete automated
keyboard/mouse playthrough.

- The production executable generated all six canonical scene frames.
- The route now follows the execution contract ordering and uses authored room
  files for every scene in the slice.
- The real input backend reports the production keyboard and Raw Input pointer
  path; unit tests cover pointer deltas, buttons, focus loss/regain and context
  switching.
- F1 narrator intrusion, F3 overlay, fire/recoil/muzzle feedback, pause and
  melee feedback are wired in the production composition root. Audio playback
  is not implemented in this slice.

## Perfection Red Team

SILVER, not GOLD. The first-time-player complaint list is explicit:

1. The renderer is a deterministic procedural production path, not final art
   atlas content; the scene prop set is intentionally sparse.
2. Audio playback, full particle/debris/smoke authoring and damage/explosion
   presentation are not yet implemented.
3. `AiModule` remains a legal no-op in the PVS executable; the cleaner and
   social outcomes are foundation contracts plus authored route callbacks, not
   a complete M5 autonomous schedule.
4. There is no automated end-to-end input replay for the route; the bounded
   human playtest is the next evidence step.

These are medium, scope-bounded issues. None is a current Fatal or Major
blocker for a Silver human-playtest candidate.

## Top Quality Gaps / Controlled Defers

- PVS-M01 — Owner: M2. Replace procedural silhouettes with the formal material
  atlas and finish audio/particle/effect assets. Reason: Visual Bible fidelity
  and premium feel. Evidence: six scenes are readable and within the render
  budget, but no final atlas/audio asset exists. Revisit when the first formal
  production art/audio batch is available; required before GOLD.
- PVS-M02 — Owner: M5. Replace the PVS no-op AI wiring with the approved small
  autonomous NPC routine and cleaner decision loop. Reason: the current route
  proves systemic records but not independent behavior. Evidence: foundation
  transitions/tests pass; the composition root intentionally contains authored
  callbacks. Revisit at M5 implementation start; do not expand the PVS kernel
  now.
- PVS-M03 — Owner: PVS QA. Perform a human keyboard/mouse run through the six
  scenes and record interaction recovery, pacing and effect readability.
  Reason: the input probe proves backend selection and focus, not player intent.
  Evidence: six bounded app runs and input probe pass; route replay is
  UNVERIFIED. Revisit at the first human playtest session.
- PVS-M04 — Owner: M6/content pipeline. Extend seed authoring/compiler coverage
  for the currently documented quest, knowledge, terminal, observation and
  relationship authoring records. Reason: the current authoring validator and
  compiler intentionally cover the foundation seed subset; runtime contracts
  already exist. Evidence: current seed/schema/invalid-seed gates pass and the
  explicit cognition migration is fixed. Revisit before those records move from
  runtime-authored PVS wiring into data-driven content.

No issue is deferred with an ownerless “later” condition.

## Open Counts

- Open Fatal: 0
- Open Major: 0
- Open Medium: 4
- Open Low: 0
- Open P0/P1: 0

## Commits / Remote

- PVS-01 code and route commit: pending local commit
- PVS-01 evidence/ledger commit: pending local commit
- Remote push: pending final verification
- Only `main` is in scope; no branch or PR is to be created.

## Next Action

After the scoped commits and remote verification, stop implementation and hand
the Silver candidate to a human for the six-scene playtest. Do not start M5 AI,
formal art production or broader game development in this task.
