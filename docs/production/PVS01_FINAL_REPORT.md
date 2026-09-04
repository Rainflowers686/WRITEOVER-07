# PVS-01 Final Report — WRITEOVER-07

## Result

PVS-01 is a **SILVER** vertical-slice release candidate:

`READY_FOR_HUMAN_PLAYTEST_WITH_SCOPE_LIMITS`

The intended authored route is present and wired through the production
composition root:

`B1 Wake -> Calibration / Logistics -> Service / Medical -> 1F Lobby -> Security Checkpoint -> Restroom / Staff route -> Elevator Lobby`

The slice is suitable for a bounded human playtest. GOLD is not claimed because
formal art/audio/particle production, autonomous NPC behavior and a repeatable
automated input replay are not part of this release.

## Repository Scope

- Repository: `Rainflowers686/WRITEOVER-07`
- Branch: `main`
- Trusted pre-PVS head: `5dadd5f822e4668161bdf92fae8c5f057690995f`
- Clean head immediately before this PVS closure: `667b7399850e8a6ed718f4e73c17cb5e1549f7eb`
- Final release head: recorded in the handoff after the scoped commits
- New branches: none
- Pull requests: none
- Gameplay framework/ECS/plugin expansion: none

## What Was Closed

### Runtime slice

- Production Half-block TrueColor framebuffer remains the active world path at
  `240x67` terminal cells / `240x134` logical pixels.
- The existing calibration room is now reachable in the normal route instead of
  being fallback-only.
- Six authored scenes are wired: B1 revival, calibration/logistics,
  service/medical, 1F security, restroom/staff and elevator lobby.
- The production composition root owns only the small PVS interactions. The
  single `SystemicWorld` remains the shared semantic kernel.
- Pistol fire produces a live viewmodel recoil/muzzle state; F3, pause, melee,
  subtitles and the narrator intrusion path are wired.
- F5/F9 uses the existing SaveManager sections and validates systemic/player
  state before applying a load.

### Systemic seams

- Body concealment now emits a typed `BodyHidden` event.
- The regression chain uses an actual body search to reveal the badge, then an
  explicit theft transition, drag start/update/end, concealment, observation-only
  discovery and a separate response decision.
- `DiscoverBody` does not auto-escalate security; reporting, medical call,
  cover-up, ignore and other responses remain distinct transitions.
- Item provenance keeps original owner, issuer, legal holder, physical holder,
  stolen/report status and revocation separate.
- Relationships are directed and bounded; promise transitions are legal-FSM
  guarded; duplicate IDs and systemic serialization bounds are fail-closed.
- Per-source observability remains separate from narrator authority and mirrors a
  canonical observation-source store.
- The seed compiler honors explicit v1.2 `cognition` and retains legacy `class`
  migration behavior.

## Validation

All results below were executed locally after the final code change.

| Gate | Result | Evidence |
|---|---|---|
| Debug configure/build | PASS | `cmake --preset debug`; `cmake --build --preset debug --config Debug --parallel 4` |
| Release configure/build | PASS | `cmake --preset release`; `cmake --build --preset release --config Release --parallel 4` |
| CI configure/build | PASS | `cmake --preset ci`; `cmake --build --preset ci --config Debug --parallel 4` |
| Debug CTest | PASS | `ctest --preset debug --output-on-failure` |
| CI CTest | PASS | `ctest --preset ci --output-on-failure` |
| Release unit executable | PASS | `out/build/release/Release/writeover_tests.exe`; 158 tests, 0 failed |
| Smoke | PASS | `scripts/smoke.ps1 -Preset debug`; unit, six mapc checks and bounded app exit |
| Benchmark | PASS | `scripts/bench.ps1 -Preset release`; every declared budget passed |
| Contract check | PASS | `scripts/contract_check.ps1` |
| Static audit | PASS | `python tools/audit/static_audit.py .`; `COUNT=0` |
| Content compiler | PASS | `python tools/contentc/contentc.py --data-dir data --out-dir data --check` |
| Content tests | PASS | `python tools/contentc/test_contentc.py`; 5/5 |
| Systemic schema | PASS | `python tools/systemic/systemic_schema_check.py --data-dir data` |
| Schema tests | PASS | `python tools/systemic/test_systemic_schema.py`; 5/5 |
| Invalid seed gate | PASS | `python tools/systemic/test_runtime_invalid_seed.py` |
| Explicit cognition compiler probe | PASS | explicit v1.2 field and legacy class both encode correctly |

The two `ASSERT FAILED` lines printed by the release test executable are
intentional self-checks of the test harness and are followed by PASS results;
the final executable summary is `158 tests, 0 failed`.

## Performance Evidence

Release `bench.ps1` output:

- raycast column sweep worst1% average: `0.098 ms`
- terminal full worst1% average: `0.263 ms`, budget PASS
- terminal delta worst1% average: `0.055 ms`, budget PASS
- terminal unchanged worst1% average: `0.035 ms`, budget PASS
- terminal worst-case safety: `0.234 ms` worst1% average / `50,659 bytes`, PASS
- systemic lookup worst1% average: `0.012 ms`, PASS
- systemic update workload worst1% average: `0.185 ms`, PASS
- PVS render workload `240x67` worst1% average: `0.981 ms`, PASS
- overall declared budget: PASS

The systemic update workload exercises the current foundation records and
transitions: relationships, memories, evidence, drag/discovery, item lifecycle,
promises, quests, searches, exchanges, terminal audit, observability and the
event bridge. It is not presented as a future full-M5 AI decision benchmark.

## Visual and Input Evidence

The release executable generated six scene frames using `--frames 1
--dump-frame`:

- B1 revival
- calibration/logistics
- service/medical
- 1F security
- restroom/staff
- elevator lobby

Every run exited `0`. Every PPM was `240x134` logical pixels and `96,495`
bytes. Visual inspection found dark industrial floor/ceiling bands, mid-dark
walls, localized cyan/amber props, visible occlusion behavior and a weapon
viewmodel within the bottom-right safe anchor. No large-area light-gray or white
floor was observed.

The release input probe reported:

- keyboard: `keyboard-console`
- pointer: `raw-input`
- mouse buttons: `raw-input`
- context: `gameplay`
- focus: `1`

Unit tests cover pointer deltas, mouse buttons, focus loss/regain and context
switching. A complete automated keyboard/mouse route replay was not available
in this environment, so that part remains `UNVERIFIED` and is explicitly left
for the human playtest.

## Red-Team Findings

No Fatal or Major finding remains open.

### PVS-M01 — formal visual/audio/effect production gap

- Severity: MEDIUM
- Type: VISUAL_SCOPE_LIMIT
- Observed: the renderer is a deterministic procedural production path with a
  deliberately small prop set; formal material atlas art, audio playback and
  full particle/debris/smoke systems are absent.
- Impact: the six scenes are legible and performant, but the slice is not yet a
  final premium-art presentation and cannot claim GOLD.
- Minimal correction: M2 supplies the first formal art/audio/effect batch and
  measures it against the existing framebuffer and budget gates.
- Must fix before human playtest: NO.
- Must fix before GOLD: YES.

### PVS-M02 — autonomous NPC runtime gap

- Severity: MEDIUM
- Type: RUNTIME_SCOPE_LIMIT
- Observed: `AiModule` is intentionally a legal no-op in the PVS executable;
  cleaner/social outcomes are represented by foundation contracts and small
  authored callbacks rather than a complete autonomous schedule.
- Impact: the systemic transitions are real and tested, but this slice does not
  prove the future M5 routine/decision loop.
- Minimal correction: implement the approved small M5 routine when M5 begins;
  do not expand the PVS kernel or add a second world now.
- Must fix before human playtest: NO.
- Must fix before M5 implementation: YES.

### PVS-M03 — end-to-end input replay evidence gap

- Severity: MEDIUM
- Type: PLAYTEST_EVIDENCE_LIMIT
- Observed: backend selection, focus and input seams pass, and bounded runtime
  frames render, but no automated key/mouse script completed the full six-scene
  route.
- Impact: movement pacing, interaction affordance and recovery from player
  mistakes still need direct human observation.
- Minimal correction: run the six-scene route with a human keyboard/mouse
  playtest and record only actionable findings.
- Must fix before human playtest: NO; this is the next playtest step.
- Must fix before GOLD: YES.

### PVS-M04 — future data-driven authoring coverage gap

- Severity: MEDIUM
- Type: CONTENT_AUTHORING_RISK
- Observed: current schema/compiler gates validate the foundation seed subset;
  several documented quest, knowledge, terminal, observation and relationship
  records remain runtime-authored for this PVS rather than fully data-driven.
- Impact: M6/content work must not silently assume that every documented record
  already compiles from JSON.
- Minimal correction: extend the existing validator/compiler at the point those
  records move into authored data; keep the current typed runtime contracts.
- Must fix before human playtest: NO.
- Must fix before M6 data-driven content: YES.

## Controlled Non-Goals

This release deliberately does not add:

- a full M5 AI planner or complete cleaner routine;
- full trading, hacking, shop, quest or ending engines;
- final 36-floor content;
- formal art atlas replacement, audio middleware or a large particle system;
- a second gameplay world, ECS, plugin framework or broad architecture layer;
- new branches, hooks, pull requests or release automation.

## Final Assessment

- Open Fatal: `0`
- Open Major: `0`
- Open Medium: `4`, all scope-bounded and explicitly owned
- Open Low: `0`
- Victory Ladder: `SILVER`
- Human playtest readiness: `YES`, with the four documented scope limits
- Final implementation action: STOP after commit, push and remote verification;
  the next action belongs to human playtest, not another coding round.

## Commit / Push Receipt

The exact final commit SHA, remote URL, branch state and remote-head equality
are recorded in the task handoff after the scoped commits complete. No claim of
remote success is made in this document before that verification.
