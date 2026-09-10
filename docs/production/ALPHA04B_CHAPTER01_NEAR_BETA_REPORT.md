# WRITEOVER-07 — Alpha-04B Chapter One Near-Beta Receipt

This is a current-head engineering receipt for the bounded Chapter One
near-beta continuation. It is not a Product Gold, Visual Gold, release, or
manual-play acceptance claim.

## Final scope

This continuation stayed inside the existing Chapter One/B1 slice. It did not
add a floor, map, chapter, weapon type, renderer, AI framework, quest engine,
or release artifact. The spatial renderer, save foundation, and existing
gameplay contracts were not reopened.

The implementation work was limited to:

- stopping the onboarding hint after real player movement or the first real
  interaction;
- adding restrained natural-language cues for the existing Patrol,
  Investigate, Alert, and Combat states, without exposing enum names or
  diagnostic IDs;
- protecting damage/death/body/access feedback from lower-priority NPC speech;
- making the existing online B1 camera a real consumer of the existing loud
  action: an online camera raises the existing facility alert, while an
  offline camera produces a durable blind spot;
- adding a deterministic AI regression proving Investigate reaches inspection,
  resumes Patrol, and continues moving;
- adding two bounded current-head replay fixtures: a guard in another room and
  the B1 camera-offline counterfactual;
- correcting the scenario matrix so executed, game-rule-invalid, and valid-but-
  not-yet-fixtured states are not conflated.

No change was made to `tests/test_harness.cpp`. Its pre-existing one-line
working-tree change, historical visual evidence, and settings files remain
outside the implementation commit.

## Git identity and preservation

```text
START_HEAD = e8109310ce5c6499d6792fe1fd892075702abd6b
ORIGIN_MAIN_AT_START = e8109310ce5c6499d6792fe1fd892075702abd6b
IMPLEMENTATION_HEAD = bfe82ed6d9bc468b01f5f69898cf2ebc20b38ec4
IMPLEMENTATION_COMMIT = gameplay: close chapter one near-beta runtime gaps
BRANCH = main
PUSH = normal main push; no force push
REMOTE_CI_FOR_IMPLEMENTATION_HEAD = PASS (GitHub Actions run 34438350915; exact head; all five jobs)
```

The implementation commit contains exactly these six files:

```text
scripts/chapter01_scenario_matrix.ps1
scripts/recovery_replay_gate.ps1
src/app/composition_root.cpp
tests/test_ai.cpp
tools/replay/scenario_camera_offline.txt
tools/replay/scenario_guard_other_room.txt
```

The external recovery checkpoint and the supplemental audit root were retained.
No pre-existing evidence/settings file was staged or deleted.

## Implemented and verified behavior

### Player-facing Chapter One feedback

`MovementDemonstrated()` is set only by finite non-zero locomotion. The B1 and
calibration onboarding prompt therefore disappears after actual movement or an
actual interaction, rather than remaining as a permanent overlay. Existing
objective presentation remains separate from completion and is not cleared or
faked to satisfy a replay assertion.

The existing NPC state is presented through short authored natural-language
cues. The cue is low priority and the existing subtitle priority seam keeps
damage, death, access, and body-state feedback visible above it. This is a
player-facing legibility improvement, not a new dialogue system.

### Second delayed consequence

The existing B1 fire path still establishes the existing loud-action fact. It
now also consumes the authored `slice.camera` observation source:

```text
camera online + current-room loud action -> FacilityAlertLevel::Suspicious
                                  -> SECURITY_ALERT response
camera offline + current-room loud action -> BLIND_SPOT response
```

The branch is driven by the authored camera state and the existing facility
alert owner. It is not a replay-only flag and does not claim a general
surveillance system.

### AI behavior

The new `ai.investigate_resumes_patrol_after_inspection` test demonstrates an
actual runtime sequence: reachable noise enters Investigate, the NPC routes to
the noise area, inspection completes, Patrol resumes, and the NPC continues
along its authored route. This closes a behavior-proof gap without claiming
complete enemy combat AI.

The existing Guard health/damage behavior remains a bounded Chapter One
combat/health proof. It is explicitly **not** documented or promoted here as
complete enemy combat AI.

## Current scenario matrix

The current matrix contains 36 deliberately classified rows:

```text
SCENARIO_MATRIX_TOTAL = 36
SCENARIOS_EXECUTED = 30
SCENARIOS_EXPECTED_SUCCESS = 19
SCENARIOS_EXPECTED_DENIAL = 8
SCENARIOS_EXPECTED_RECOVERABLE_FAILURE = 0
SCENARIOS_EXPECTED_DEATH_RESTART = 3
SCENARIOS_INVALID_BY_GAME_RULES = 1
SCENARIOS_VALID_STATE_NOT_COVERED = 5
SCENARIO_MATRIX = PASS
```

All 30 executed rows passed their actual receipt assertions. The one invalid
row is `guard_alive_body_hidden`, which contradicts the current game rules.
The five valid states deliberately remain `VALID_STATE_NOT_COVERED`, not
successes or failures:

```text
guard_dead_body_hidden
terminal_active_badge_revoked
player_restarted_with_durable_history
wall_blocked_guard_line_of_sight
elevator_entry_without_route_fact
```

They have no non-contrived authored fixture in this bounded slice. This is an
explicit coverage limitation, not a fabricated PASS.

## Replay receipt

The current Release replay gate executes 21 cases and passed all 21:

- 5 Recovery cases;
- 4 Alpha cases;
- 6 Chapter One cases;
- 3 current regression fixtures, including guard room isolation and camera
  offline;
- 1 normal-quit case.

The application receipt continues to distinguish process exit, input
consumption, expected state reached, and replay result. Negative routes pass by
reaching the expected denied or isolated state; they are not relabeled as
ordinary gameplay success.

The save fault matrix was also rerun independently. All eight final-commit
injection stages passed rollback verification:

```text
after_room
after_world
after_systemic
after_events
after_rng
after_narrative
after_ai
after_player
SAVE_FAULT_MATRIX = PASS (8/8 expected-failed load probes rolled back)
```

The injected loads intentionally report `LOAD_OK=NO`; the positive assertion
is unchanged live state after the failed final commit.

## Local validation at the implementation head

All results below were rerun after `bfe82ed` was created.

| Gate | Result |
|---|---|
| Debug configure | PASS |
| Debug build | PASS |
| Debug direct unit executable | PASS, 210/210 |
| Debug CTest | PASS, 2/2 |
| Release configure | PASS |
| Release build | PASS |
| Release direct unit executable | PASS, 210/210 |
| Release CTest | PASS, 2/2 using `ctest --test-dir out/build/release -C Release --output-on-failure` |
| Content compiler check | PASS, deterministic |
| Content compiler tests | PASS, 13/13 |
| Systemic schema check | PASS |
| Systemic schema tests | PASS, 10/10 |
| Invalid systemic seed check | PASS |
| Static audit | PASS, `COUNT=0` |
| Contract check | PASS using `ExecutionPolicy Bypass` |
| Debug smoke | PASS |
| Release smoke | PASS |
| Release replay gate | PASS, 21/21 |
| Scenario matrix | PASS, 36 classified / 30 executed |
| Save final-commit fault matrix | PASS, 8/8 rollback probes |
| Release package smoke | PASS |
| Package missing-art negative probe | EXPECTED_FAILED_PROBE, exit 1 |
| Release benchmark | PASS, `OVERALL_BUDGET=PASS` |

`ctest --preset release` is not a project-defined preset; the Release CTest
result above uses the valid Release build-tree invocation and is not a hidden
test failure.

### Package evidence

The current Release package was rebuilt with implementation commit metadata and
passed executable-relative resource, user-data separation, developer-garbage,
and required-runtime-resource checks. A derived archive with
`data/characters/b1_character_art.txt` removed failed package smoke with:

```text
PACKAGE SMOKE ERROR: missing required player runtime resources:
['data/characters/b1_character_art.txt']
```

That negative invocation is recorded as an expected failed probe, not a
package success.

### Benchmark semantics

The Release benchmark reported the tool's actual `worst_1pct_avg_ms` field; it
was not renamed to p99. The current receipt included:

```text
raycast worst_1pct_avg_ms = 0.058 ms
terminal full worst_1pct_avg_ms = 0.341 ms
terminal delta worst_1pct_avg_ms = 0.095 ms
systemic lookup worst_1pct_avg_ms = 0.028 ms
systemic update worst_1pct_avg_ms = 0.213 ms
character render worst_1pct_avg_ms = 1.334 ms
integrated character frame proxy worst_1pct_avg_ms = 2.231 ms
platform terminal writes = excluded from integrated proxy
```

This remains a bounded proxy and is not end-to-end 120 Hz proof.

## Audit and legacy truth

The earlier Chapter One and Audit-02B ledgers remain the authoritative
classification records for their respective audits. This Alpha-04B change did
not rewrite those historical reports. The current work adds only the two
regression fixtures and current-head behavior receipts described above.

The preserved `pvs01_normal`, `pvs01_aggressive`, and `pvs01_stealth` scripts
remain `MIGRATE`; `pvs01_systemic` remains `HISTORICAL_ONLY`. None is silently
used as a mandatory current Chapter One oracle because its literal timing and
identity assumptions predate the current route contracts.

## Terminal and manual-play boundary

Non-foreground smoke/replay processes truthfully report the Win32
`WriteConsole` compatibility fallback (`legacy-conhost`, `ANSI16`, 240x67),
because a pipe is not proof of a foreground Windows Terminal VT session. A
foreground Windows Terminal capability/preset check and first-time human
near-beta play session were not fabricated from those probes.

```text
TERMINAL_NORMAL_LAUNCH = PENDING_MANUAL
MANUAL_FIRST_TIME_PLAY = PENDING_MANUAL
FRONTEND_VISUAL_ACCEPTANCE = PENDING_MANUAL
```

The remaining five valid-but-unfixtured scenario states above are also not
hidden under the manual label; they are explicit automated coverage gaps.

## Red-team conclusion

### Code red team

The implementation diff stayed within existing composition wiring, the AI
test seam, replay scripts, and two fixtures. No renderer, save format, map,
content, or foundation architecture was expanded. Pre-existing dirty files
were not staged.

### Behavior red team

The negative/counterfactual checks distinguish a Guard in another room from a
same-room attack, camera online from camera offline, real Investigate arrival
from a state-label-only transition, and active route assertions from valid
states without a fixture. The 21-case gate and 30 executed scenario rows
passed.

### Player-experience red team

The next real human check should concentrate on first-time onboarding clarity,
the natural-language state cues in motion, and whether the online-camera
consequence is understandable without debug output. The current terminal
fallback output is engineering evidence only; it is not a visual acceptance
claim.

## Final truth fields

```text
CURRENT_IMPLEMENTATION_HEAD = bfe82ed6d9bc468b01f5f69898cf2ebc20b38ec4
ORIGIN_MAIN_AFTER_IMPLEMENTATION_PUSH = bfe82ed6d9bc468b01f5f69898cf2ebc20b38ec4
WORKTREE = DIRTY_PREEXISTING_ONLY
IMPLEMENTATION_COMMIT_CREATED = 1
PUSHED = YES
REMOTE_CI = PASS (run 34438350915 for the exact implementation head; all five jobs)
FINAL_DOCUMENTATION_RECEIPT_CI = PENDING_UNTIL_THIS_RECEIPT_IS_PUSHED

ONBOARDING_SUPPRESSION = VERIFIED
OBJECTIVE_PRESENTATION = VERIFIED_BOUNDED
INTERACTION_CLARITY = VERIFIED_BOUNDED
AI_STATE_LEGIBILITY = VERIFIED_BOUNDED
SECOND_DELAYED_CONSEQUENCE = VERIFIED_BOUNDED
MEMORY_BEHAVIOR = VERIFIED (existing durable Cleaner history)
PATROL = VERIFIED_BOUNDED
INVESTIGATE = VERIFIED_BOUNDED
GUARD_COMBAT = VERIFIED_BOUNDED_HEALTH_PROOF_ONLY
PACKAGE_RUNTIME_ASSETS = VERIFIED_BOUNDED
PACKAGE_SMOKE = VERIFIED
PACKAGE_NEGATIVE = EXPECTED_FAILED_PROBE
REPLAY_GATE = PASS (21/21)
SAVE_FAULT_MATRIX = PASS (8/8)
BENCHMARK = PASS (worst_1pct_avg_ms field; not p99)
TERMINAL_NORMAL_LAUNCH = PENDING_MANUAL
MANUAL_PLAY = PENDING_MANUAL
PVS01_REPLAYS = MIGRATE / HISTORICAL_ONLY

OPEN_FATAL = 0
OPEN_P0 = 0
OPEN_NORMAL_PLAY_P1 = 0 for the current bounded route
VALID_AUTOMATED_COVERAGE_GAPS = 5
DEFER_ASTRA = subjective art/prose/audio and human visual acceptance only;
              no correctness finding is hidden by this label
FINAL_STATUS = READY_FOR_RAIN_CHAPTER01_NEAR_BETA_REVIEW
```

The final status means the bounded current route is ready for Rain's review;
it does not mean Chapter One is Gold, the product is Gold, or the project is
ready for public release.
