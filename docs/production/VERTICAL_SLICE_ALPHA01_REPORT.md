# WRITEOVER-07 — Vertical Slice Alpha-01 Report

## Receipt

This report records the bounded first playable slice on top of the existing
Recovery-04 foundation. It does not certify Product Gold, Visual Gold, a
public release, or a final art/audio pass.

```text
START_HEAD = 7b661c4f05103aa267d46f485a71e27bfbb80451
IMPLEMENTATION_HEAD = a3266ab1427eef416dd2963a54dc08be4a45096f
FINAL_HEAD = documentation receipt containing this report; runtime source is IMPLEMENTATION_HEAD
ORIGIN_MAIN_AT_START = 7b661c4f05103aa267d46f485a71e27bfbb80451
BRANCH = main
WORKTREE = DIRTY_BY_PRESERVED_PREEXISTING_EVIDENCE_SETTINGS_AND_TEST_HARNESS_NEWLINE_ONLY
```

The pre-existing dirty files were deliberately not included: old visual
evidence under `docs/production/evidence/`, `settings_ctx.cfg`,
`settings_legacy.cfg`, `settings_test.cfg`, and the final-newline-only change
in `tests/test_harness.cpp`.

## Scope and implementation

Implemented only the first-slice presentation and route closure:

- Added a compact `HudFrame::interaction_prompt` surface. It is populated
  from the current focused target and authoritative state, separate from the
  objective text.
- Added bounded Alpha objective sources for B1, calibration, medical,
  security, staff and elevator rooms. Completion is based on real facts,
  terminal/session state, gate state and room transitions, not elapsed time.
- Added bounded natural-language Alpha storylet text and facts for camera
  outage, body concealment, loud action, denied access and checkpoint
  reaction. The existing content compiler emits and verifies the runtime
  binaries.
- Added four deterministic Alpha replays and extended the existing replay gate
  to assert process exit, input consumed, expected state and pass separately.
- Corrected the Recovery health/death replay to use a real approach to the
  authored guard and a valid checkpoint restore window.
- Corrected the Release smoke wrapper to invoke the Visual Studio Release
  CTest configuration instead of a nonexistent `release` CTest preset.
- Added the minimal ADR note for the new compact onboarding field and updated
  the public-header contract snapshot.

No new room, weapon type, renderer, ECS, quest engine, behavior framework or
final art/audio system was added.

## Content and route inventory

```text
ROOMS_USED = room_b1_revival, room_01_calibration, room_service_medical,
             room_1f_security, room_restroom_staff, room_elevator_lobby
ACTIVE_NPCS = 5 runtime instances from 6 authored profiles
ACTIVE_INTERACTABLES = 11 compiled scene placements
PATROL_ROUTES = 3 authored routes (Cleaner, Guard, Technician)
COMPILED_FACTS = 15
COMPILED_STORYLETS = 7
```

The smallest recommended player path is B1 → calibration → medical, with the
security or elevator branch available from the existing authored links. The
current replay gate directly proves the B1 systemic checkpoint and its
counterfactuals; the later-room links are existing compiled content and are
not being presented as a hand-played visual acceptance session.

## Player-facing Alpha contract

```text
ONBOARDING = VERIFIED_BOUNDED
OBJECTIVE_CHAIN = B1 find route -> obtain access -> use terminal -> use reader -> cross to calibration
INTERACTION_PROMPTS = VERIFIED_BOUNDED
SYSTEMIC_ROUTE = VERIFIED
AGGRESSIVE_ROUTE = VERIFIED
OPTIONAL_VARIATION = VERIFIED_BOUNDED
```

The systemic route is the existing real input chain: Stunner hit, body search,
badge transfer, drag, cart concealment, Cleaner arrival/inspection/response,
credentialed terminal, reader, actual door crossing and save/load. The
aggressive route uses Pistol damage and leaves the body exposed before taking
the transferred credential and crossing. The denied route omits the
credential and proves that the same reader attempt fails. The memory route
creates the Cleaner relationship through player interaction, saves/loads, and
then observes a different later response; it does not inject a relationship
or body state into replay.

```text
BODY_SEARCH = VERIFIED
BODY_DRAG = VERIFIED
BODY_HIDE = VERIFIED
CLEANER_RESPONSE = VERIFIED_BOUNDED
MEMORY_CONSEQUENCE = VERIFIED
PATROL = VERIFIED_BOUNDED
INVESTIGATE = VERIFIED_BOUNDED
STEALTH = VERIFIED_BOUNDED
GUARD_COMBAT = VERIFIED_BOUNDED_HEALTH_PROOF_NOT_COMPLETE_ENEMY_COMBAT
PISTOL = VERIFIED_BOUNDED
STUNNER = VERIFIED
SMG = DEFER_ASTRA_FOR_LATER_TRUTHFUL_CONTENT
TERMINAL = VERIFIED_BOUNDED
CAMERA = VERIFIED_BOUNDED
READER_GATE = VERIFIED
NARRATIVE_REACTIONS = VERIFIED_BOUNDED_NATURAL_LANGUAGE
QUEST_COMPLETION = VERIFIED_BOUNDED
CHECKPOINT = VERIFIED_BOUNDED
```

Guard damage is intentionally described as a bounded health/death proof only:
the current slice does not claim complete enemy combat AI, squad behavior,
tactical movement or a finished combat game.

## Replay assertions and current results

The Release binary was rebuilt after the final source edit. The replay gate
ran all five Recovery cases and all four Alpha cases with independent
`PROCESS_EXIT_OK`, `INPUT_CONSUMED`, `EXPECTED_STATE_REACHED` and
`REPLAY_RESULT` assertions.

| Replay | Result | What it proves |
|---|---|---|
| `recovery_b1_success` | PASS | Existing truthful B1 body/credential/terminal/gate/save-load route. |
| `recovery_b1_denied` | PASS | Denied credential route remains denied. |
| `recovery_b1_terminal_denied` | PASS | Terminal denial does not create a session. |
| `recovery_b1_badge_only` | PASS | Badge acquisition alone does not imply access. |
| `recovery_b1_health_death` | PASS | Real Guard damage reaches player death and F9 recovery. |
| `alpha01_systemic_success` | PASS | Systemic/non-lethal route reaches checkpoint with Cleaner consequence and visible narrative action. |
| `alpha01_aggressive_success` | PASS | Pistol/aggressive route reaches checkpoint with a materially different body/alert outcome. |
| `alpha01_denied` | PASS | Same early setup without credential fails at access. |
| `alpha01_memory_consequence` | PASS | Durable relationship/history changes later Cleaner response after save/load. |

The direct Release output included, for the successful systemic route,
`SLICE_SHOT_HIT=YES`, `NONLETHAL_HIT=YES`, `BODY_CREATED=YES`,
`BODY_HIDDEN=YES`, `BODY_DISCOVERED=YES`, `ACCESS_ATTEMPTED=YES`,
`GATE_OPEN=YES`, `GATE_CROSSED=YES`, `SAVE_OK=YES`, `LOAD_OK=YES`,
`NARRATIVE_VISIBLE_ACTION=YES` and `CHAPTER_CHECKPOINT_REACHED=YES`.
The denied route reported no badge, no terminal session, a closed gate and no
checkpoint. The health route reported `PLAYER_DIED=YES` and
`PLAYER_RECOVERED=YES`.

`ALPHA_ROUTE_DURATION_ESTIMATE = NOT_MEASURED_AS_MANUAL_PLAY`; replay frame
counts are direct deterministic coverage and must not be called an 8–12 minute
human playthrough. A first-time human duration remains a manual review item.

## Validation

All commands below were run against the current local implementation before
the documentation receipt was created.

| Gate | Result |
|---|---|
| Debug configure | PASS — `cmake --preset debug` |
| Debug build | PASS — `cmake --build --preset debug --config Debug` |
| Debug CTest | PASS — 1/1 |
| Debug direct unit | PASS — 206 tests, 0 failed |
| Release configure | PASS — `cmake --preset release` |
| Release build | PASS — `cmake --build --preset release --config Release` |
| Release CTest | PASS — 1/1 |
| Release direct unit | PASS — 206 tests, 0 failed |
| Production content compile/check | PASS — deterministic `contentc --check` |
| Content compiler tests | PASS — 13/13 |
| Systemic schema check | PASS — 1 file |
| Systemic schema tests | PASS — 10/10 |
| Invalid systemic seed startup | PASS — startup rejected malformed seed |
| Static audit | PASS — `COUNT=0` |
| Contract/dependency/header checks | PASS |
| Debug smoke | PASS — `scripts/smoke.ps1 -Preset debug` |
| Release smoke | PASS — `scripts/smoke.ps1 -Preset release` |
| Recovery + Alpha replay gate | PASS — 9/9 |
| Save final-commit fault matrix | PASS — 8/8 stages rolled back with process/section equality |
| Recovery-03 spatial gates | PASS — included in the 206 direct unit tests |
| Release benchmark | PASS — all existing budget verdicts passed |

The benchmark reports its actual field name `worst_1pct_avg_ms`; it does not
rename that statistic to p99 and does not prove end-to-end 120 Hz terminal
presentation. The latest Release sample reported `PVS_RENDER_TIME_MS=1.251`
and `PVS_TOTAL_FRAME_TIME_MS=2.420` using the existing proxy, with platform
writes excluded.

## Reports, CI and manual boundary

```text
SMOKE_WRAPPER = FIXED_AND_VERIFIED
DISK_CLEANUP = TASK_CREATED_ALPHA_REPLAY_OUTPUTS_REVIEW_PENDING
REMOTE_CI = PENDING_NORMAL_MAIN_PUSH
REAL_MANUAL_PLAY = PENDING_MANUAL
FRONTEND_EVIDENCE = PENDING_MANUAL
```

The normal main push workflow was inspected: `.github/workflows/ci.yml` runs
build, content/schema, tests, smoke, replays, benchmark and static/contract
checks. It does not create a Release, move a tag or publish Steam on an
ordinary push. No release workflow was invoked.

The non-foreground smoke terminal path can report its actual VT negotiation,
backend, dimensions, color capability and preset. That is useful engineering
diagnostic evidence, not a substitute for Rain's real foreground Windows
Terminal playability and visual review. No screenshot or video generated by a
pipe/replay was promoted as manual evidence.

## Three red-team conclusions

### Code red team

Scope stayed within the bounded Alpha route, player HUD prompt, authored facts/
storylets, replay receipts, one truthful smoke wrapper and the related ADR/
contract snapshot. No old visual evidence, settings or unrelated test newline
were staged. The runtime reuses existing World, Systemic, AI, Narrative,
Character Renderer and Save/Load seams.

### Behavior red team

The negative cases are meaningful: a badge-less reader attempt fails; badge
acquisition alone does not open the route; terminal denial does not create a
session; lethal and non-lethal routes reach different body outcomes; and the
Cleaner response differs after durable relationship history. Health/death is
asserted from actual Guard damage rather than a fixed HP receipt.

### Player-experience red team

The compact objective/prompt layer removes the largest current onboarding
ambiguity without turning the HUD into a debug panel. The remaining risk is
not a hidden state assertion but first-time human comprehension of the B1 →
calibration → later-room route and the balance of placeholder text/audio.
That requires Rain's foreground manual review. It is not being self-certified.

## Deferred work and honest limits

```text
DEFER_ASTRA = final prose, final sound design, final character beauty/art pass,
              broad SMG content, complete enemy combat AI, full quest graph,
              building-wide content and foreground visual acceptance
MOST_IMPORTANT_REMAINING_FUNCTIONAL_WORK = Rain manual first-time route review;
              add more route-specific receipts only if that review exposes a gap
MOST_IMPORTANT_REMAINING_ARTISTIC_WORK = final human/audio/art direction review
```

The current maximum status after the normal push and remote CI receipt is
`READY_FOR_RAIN_VERTICAL_SLICE_ALPHA_REVIEW`. Manual foreground playability
and art acceptance remain explicitly pending. No product/release claim is
made.
