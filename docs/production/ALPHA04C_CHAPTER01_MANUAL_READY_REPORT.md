# WRITEOVER-07 — Alpha-04C Chapter One manual-readiness receipt

This report records the last bounded objective-closure pass before Rain's
first-time manual Chapter One session. It combines current automated evidence
with explicit manual and visual boundaries. It is not a manual acceptance,
Chapter One Gold, Product Gold, Visual Gold, or public-release receipt.

## Scope and implementation

Alpha-04C stayed inside the existing Chapter One slice: B1 Revival,
Calibration, Medical Service, Staff, Security and Elevator Lobby. It did not
add Chapter Two, a new floor, a new framework, a weapon category, a renderer,
a navigation system, a save architecture, or final art/prose/audio.

The implementation closed the five legal rows that Alpha-04B had left
unautomated. The changes use the existing runtime, interaction, systemic,
world, AI and replay seams. The source change also makes the player-facing
badge-revocation/elevator-denial states explicit; it does not inject final
authoritative state merely to satisfy a matrix row.

## Required truth fields

```text
START_HEAD = 2f57e5574b4c98b0f95f48c231c11cdcb4484f65
FINAL_IMPLEMENTATION_HEAD = e334f159e75ec95f453d5339344dd10e5d706b5f
FINAL_HEAD = e334f159e75ec95f453d5339344dd10e5d706b5f
ORIGIN_MAIN = 2f57e5574b4c98b0f95f48c231c11cdcb4484f65 at task start; final documentation push is a later docs-only receipt
BRANCH = main
WORKTREE = DIRTY_PREEXISTING_ONLY
```

`FINAL_HEAD` is the final content-bearing implementation head. The companion
documentation files are intentionally a later documentation-only commit so
the implementation receipt remains attributable to `e334f15`. The final
remote documentation-head SHA and exact five-job CI result are verified in the
closing Git receipt after this report is committed; they do not change the
implementation behavior reported here.

## What changed

```text
PLAYER_VISIBLE_CHANGES =
  - B1 and room objectives stay short and actionable and end coherently.
  - Interaction prompts track body, cart, camera, terminal, reader, doors,
    NPC and elevator state through the existing ray/focus seam.
  - An active B1 terminal session plus an online-camera loud action revokes
    the held badge with an actionable reload message; the later terminal and
    reader deny for the real revoked credential.
  - Elevator entry without a staff/security route fact is visibly denied and
    cannot create a false chapter checkpoint.
  - Existing quiet/aggressive, Cleaner-history, camera online/offline and
    AI-state cues remain player-facing at the current head.
ENGINEERING_ONLY_CHANGES =
  - Five authored replay fixtures and their semantic receipt branches.
  - Scenario-matrix classification and coverage accounting.
  - Body status, LOS, badge-revocation and elevator-denial receipt fields.
  - No public header/API change; existing tests and contracts remain in place.
```

The player-facing statements above describe observable current runtime
behavior, not a claim of final subjective presentation quality.

## Scenario closure

```text
SCENARIO_TOTAL = 36
SCENARIOS_EXECUTED = 35
SCENARIO_SUCCESS = 20
SCENARIO_DENIAL = 11
SCENARIO_RECOVERABLE_FAILURE = 1
SCENARIO_DEATH_RESTART = 3
INVALID_BY_RULES = 1
VALID_UNAUTOMATED = 0
SCENARIO_MATRIX = PASS
```

The one invalid row is `guard_alive_body_hidden`: a living Guard cannot have a
dead/hidden body under the current game rules. It is not counted as a legal
coverage gap.

The five previous uncovered rows and their current fixtures are:

| Alpha-04B row | Closure reason | Current evidence |
|---|---|---|
| `guard_dead_body_hidden` | `REPLAY_FIXTURE_MISSING` | `scenario_dead_body_hidden.txt` kills the authored Guard, searches the dead body, drags it, and hides it in the real cart transaction; receipt requires `BODY_STATUS=DEAD`, `BODY_HIDDEN=YES`, and no non-lethal hit. |
| `terminal_active_badge_revoked` | `REAL_RUNTIME_GAP` plus `REPLAY_FIXTURE_MISSING` | `scenario_terminal_active_badge_revoked.txt` creates a real B1 terminal session, fires while the online camera observes B1, revokes the held credential, and retries the terminal; receipt requires session, revocation, denial, and no gate crossing. |
| `player_restarted_with_durable_history` | `REPLAY_FIXTURE_MISSING` | `scenario_player_restarted_with_durable_history.txt` establishes the Cleaner relationship, reaches real Guard death, presses F9, and requires restart plus durable history. |
| `wall_blocked_guard_line_of_sight` | `REPLAY_FIXTURE_MISSING` plus missing receipt assertion | `scenario_wall_blocked_guard_los.txt` places the player behind authored geometry in Security; receipt requires blocked LOS, zero Guard attacks, and health 100. |
| `elevator_entry_without_route_fact` | `REPLAY_FIXTURE_MISSING` plus missing entry-state assertion | `scenario_elevator_without_route.txt` focuses and presses the restricted elevator door without a route fact; receipt requires attempted/denied entry and no chapter checkpoint. |

```text
FIVE_PREVIOUS_UNCOVERED_SCENARIOS = 5
NOW_AUTOMATED = 5
STILL_UNAUTOMATED = 0
```

All executed rows passed process-exit, input-consumed, expected-state and
semantic-result checks. A denied route is an expected player outcome, not a
test failure.

## Route, consequence and room truth

```text
SOFTLOCKS_FOUND = 0 confirmed current bounded-route soft-locks
SOFTLOCKS_FIXED = 0 confirmed soft-locks; five legal coverage/receipt seams closed
ROUTE_SKIPS_FOUND = 2 bounded skip attempts (B1 terminal skip; elevator without route fact)
ROUTE_SKIPS_FIXED = 2 expected denials with no false progression

QUIET_ROUTE = VERIFIED_BOUNDED
AGGRESSIVE_ROUTE = VERIFIED_BOUNDED
DOWNSTREAM_DIFFERENCES = VERIFIED_BOUNDED

DELAYED_CONSEQUENCE_1 = VERIFIED (Cleaner relationship/history plus hidden-body discovery changes later Cleaner response and survives route save/load)
DELAYED_CONSEQUENCE_2 = VERIFIED_BOUNDED (B1 camera online/offline changes later facility/security response; online observation can revoke an active held badge)
```

The current route distinction is not text-only:

| Early action | Immediate effect | Later effect | Player-visible evidence |
|---|---|---|---|
| Stunner/non-lethal Guard handling, then hide the body | Unconscious body enters the cart transaction | Cleaner can discover/respond; quiet route reaches staff passage | Body/drag/cart prompts, Cleaner response, quiet-route objective and staff door |
| Pistol/loud Guard handling or leaving the body exposed | Dead/exposed body and loud-action fact | Aggressive/security path and Cleaner cover-up denial differ | Hit/death text, body state, elevated response, Security objective |
| Leave camera online versus disable it before a B1 loud action | Online observation or durable blind spot | Facility alert/security response differs; active badge may be revoked online | Camera alert or blind-spot text, later Security objective, revoked-badge denial |

Medical has a real intake-terminal gate and chooses the authored
staff/security route. Staff provides the shift-change clue and a quieter
service-door route. Security remains the densest authored encounter with
stealth/badge, bribe, stunner, pistol, LOS, damage and recovery paths.
Elevator denies incomplete route records and records Chapter One completion
once. Guard behavior is bounded health/combat proof, not a claim of complete
enemy combat AI.

## Player-facing clarity truth

```text
ONBOARDING = VERIFIED
HELP = VERIFIED_BOUNDED
OBJECTIVES = VERIFIED_BOUNDED
INTERACTION_PROMPTS = VERIFIED_BOUNDED
MESSAGE_PRIORITY = VERIFIED_BOUNDED
INTERNAL_ID_LEAKAGE = VERIFIED

AI_STATE_LEGIBILITY = VERIFIED_BOUNDED
PATROL_INVESTIGATE_RESUME = VERIFIED
MULTI_STIMULUS = VERIFIED_BOUNDED
ALERT_LEGIBILITY = VERIFIED_BOUNDED
COMBAT_FEEDBACK = VERIFIED_BOUNDED
```

The objective text is room/state driven and includes concrete actions such as
searching the Guard, taking the badge, using a terminal/reader, entering
Medical/Staff/Security, and verifying the elevator checkpoint. Completion
resolves to `Chapter One complete`. Prompts are produced from the current
focused target and state, including denial contexts such as a revoked badge.

The normal-mode leakage scan found no raw `fact_`, `storylet_`, `text_`,
`npc_`, enum, hash, or replay label in player-facing text. F3 development
diagnostics are intentionally outside this claim. Existing bounded subtitle
priority prevents incidental Patrol/AI speech from replacing access, damage,
death, body, checkpoint or completion feedback.

## Death, checkpoint and persistence

```text
DEATH_RESTART = VERIFIED
CHECKPOINT = VERIFIED_BOUNDED
NORMAL_QUIT = VERIFIED
SAVE_ROUTE_MATRIX = VERIFIED_BOUNDED
SAVE_ROUNDTRIP = VERIFIED_BOUNDED
BACKTRACKING = VERIFIED_BOUNDED
```

The Release replay suite includes no-save death/restart, checkpoint death/load,
quiet and aggressive mid-route save/load, Cleaner history, camera
counterfactual, body exposed/hidden, Security and pre-elevator states. The
eight final-commit save-fault stages (`after_room`, `after_world`,
`after_systemic`, `after_events`, `after_rng`, `after_narrative`, `after_ai`,
`after_player`) all rolled back and correctly failed load as expected
negative probes. A 12,000-frame backtrack simulation also completed without
observed event/memory/subtitle growth after route completion.

Transient NPC motion is not promoted to a universal exact-frame persistence
claim; after load, authoritative room/facts/body/relationship/checkpoint truth
is the acceptance target and active motion resumes under the existing bounded
runtime semantics.

## Package and regression evidence

```text
PACKAGE_SELF_CONTAINMENT = VERIFIED_BOUNDED
CWD_INDEPENDENCE = VERIFIED_BOUNDED
CLASSROOM_PACKAGE_CANDIDATE = VERIFIED_BOUNDED
```

A clean Windows x64 candidate was built at the current implementation head,
extracted outside the repository, and started from an unrelated working
directory. Positive package smoke passed executable-relative data resolution,
authored character/text resources, user-data separation and save creation.
The required-resource negative probe removed the authored character/text
resources and failed closed with exit 1, recorded as
`EXPECTED_FAILED_PROBE`, not as a package PASS. The disposable `dist` staging
tree and negative package copies are removed during final cleanup; no public
Release, Steam upload or GitHub Release was created.

The final local receipt at `e334f15` was:

| Gate | Result |
|---|---|
| Debug configure/build | PASS |
| Debug CTest | PASS, 2/2 |
| Debug direct suite | PASS, 210/210 |
| Release configure/build | PASS |
| Release CTest | PASS, 2/2 using the explicit Release configuration |
| Release direct suite | PASS, 210/210 |
| Standalone public-header compile | PASS |
| Content compiler check | PASS, deterministic |
| Content compiler tests | PASS, 13/13 |
| Systemic schema check/tests | PASS, 10/10 |
| Invalid systemic seed | PASS, expected rejection |
| Static/dependency/contract checks | PASS; static `COUNT=0` |
| Debug smoke | PASS; foreground-style debug backend receipt was `ansi-truecolor`, 80x24, VT negotiated |
| Release smoke | PASS; non-foreground Win32 compatibility fallback was reported honestly |
| Normal quit proof | PASS in the mandatory replay gate |
| Mandatory Release replay gate | PASS, 21/21 |
| Scenario matrix | PASS, 36 total / 35 executed / 1 invalid / 0 legal gaps |
| Save final-commit fault matrix | PASS, 8/8 rollback probes |
| Save route-state roundtrip | PASS, bounded route receipts |
| Death/restart and route-skip negatives | PASS |
| Recovery-03 spatial tests | PASS, all 7 spatial cases inside the 210-test suite |
| Determinism | PASS, five seeded route pairs |
| Long simulation | PASS, 12,000 frames |
| Optional 10/20/25-NPC scale test | NOT_RUN; optional and production NPC count unchanged |
| Release benchmark | PASS, `OVERALL_BUDGET=PASS` |

The Release benchmark fields were:

```text
raycast worst_1pct_avg_ms = 0.185
terminal full worst_1pct_avg_ms = 0.324
terminal delta worst_1pct_avg_ms = 0.139
unchanged terminal worst_1pct_avg_ms = 0.113
terminal worstcase worst_1pct_avg_ms = 0.295
systemic lookup worst_1pct_avg_ms = 0.067
systemic update worst_1pct_avg_ms = 0.215
character render worst_1pct_avg_ms = 1.121
integrated frame proxy worst_1pct_avg_ms = 1.489
OVERALL_BUDGET = PASS
```

`worst_1pct_avg_ms` is the benchmark's actual field. It is not relabeled p99,
and the integrated proxy excludes platform terminal writes; this is not
end-to-end 120 Hz proof.

## Final boundaries and next action

```text
LOCAL_FULL_REGRESSION = PASS
REMOTE_FINAL_HEAD_CI = PENDING_FINAL_DOCUMENTATION_PUSH_AT_REPORT_CREATION
TEMP_CLEANUP = PENDING_FINAL_EXPLICIT_CLEANUP
CHECKPOINT_SAFE_TO_DELETE = NO

MANUAL_PLAY_CHECKLIST = CREATED; PENDING_MANUAL
MANUAL_FOREGROUND_PLAY = PENDING_MANUAL
MEASURED_HUMAN_DURATION = PENDING_MANUAL
WINDOWS_TERMINAL_ACCEPTANCE = PENDING_MANUAL
FRONTEND_VISUAL_ACCEPTANCE = PENDING_MANUAL

OPEN_FATAL = 0
OPEN_P0 = 0
OPEN_NORMAL_PLAYER_P1 = 0 for the bounded current route
OPEN_SHIPPING_P1 = 0

DEFER_ASTRA = final art, final prose and final audio; no correctness finding is hidden by this label
MOST_IMPORTANT_REMAINING_OBJECTIVE_WORK = Rain's first-time manual Chapter One acceptance using the checklist, with no replay/source coaching
MOST_IMPORTANT_REMAINING_SUBJECTIVE_WORK = final character/weapon/scene visual polish, literary prose and audio
```

The old external Recovery checkpoint is not conclusively proven redundant and
is retained. The supplemental Audit-02B forensic root is also retained. No
pre-existing visual evidence, settings file, or user modification is deleted
or staged.

The maximum truthful status after the final main push, exact-head CI and
explicit temporary cleanup is:

```text
FINAL_STATUS = READY_FOR_RAIN_CHAPTER01_MANUAL_ACCEPTANCE
```

That status means the current bounded build is ready for Rain to perform the
manual acceptance session. It does not mean that manual play, measured human
duration, foreground Windows Terminal acceptance, or frontend visual
acceptance has passed.
