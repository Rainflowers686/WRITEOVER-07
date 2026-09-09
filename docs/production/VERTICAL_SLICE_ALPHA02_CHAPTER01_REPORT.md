# WRITEOVER-07 — Vertical Slice Alpha-02 / Functional Chapter One

This receipt records the bounded functional Chapter One implementation. It is
an engineering and playability checkpoint, not a claim of Chapter One Gold,
Product Gold, visual acceptance, or release readiness.

## Receipt fields

```text
START_HEAD = 015d81d7117b3e5355b181d870d234134ac5e54a
ORIGIN_MAIN_AT_START = b7ddb63d0527202d99d0ebaf3085426a2ffed9df
IMPLEMENTATION_HEAD = c6b3872611ed3a299cb5e05b9b0569083b88d187
CROSS_PLATFORM_FIX_HEAD = 79c893d9f8f109b54c9af7dd3814b9ee5ffb81d9
FINAL_HEAD = 79c893d9f8f109b54c9af7dd3814b9ee5ffb81d9 (validated code/fix head; this report update is a documentation receipt)
ORIGIN_MAIN = 79c893d9f8f109b54c9af7dd3814b9ee5ffb81d9
BRANCH = main
PUSHED = YES
REMOTE_CI = SUCCESS (GitHub Actions run 34353383751; final validated code/fix head)
WORKTREE = DIRTY_ONLY_BY_PRESERVED_PREEXISTING_EVIDENCE_SETTINGS_AND_TEST_HARNESS_NEWLINE
```

The implementation checkpoint contains only the Chapter One content/runtime
changes and the deterministic real-input replay fixtures. Existing visual
evidence, local settings files, and the unrelated `tests/test_harness.cpp`
newline change were deliberately kept out of the commit.

## Scope and inventory

```text
ROOMS_USED = 6
NPCS_USED = 5 runtime instances from 6 authored profiles
INTERACTABLES = 19 compiled SceneEntity placements
PATROL_ROUTES = 3
FACTS = 25 compiled facts
STORYLETS = 13 compiled storylets
```

The slice reuses the existing room set:

```text
room_b1_revival
room_01_calibration
room_service_medical
room_1f_security
room_restroom_staff
room_elevator_lobby
```

No room, weapon type, renderer, ECS, quest engine, behavior-tree system, or
save architecture was added. The chapter uses the existing content compiler,
SceneRuntime, InteractionRuntime, systemic records, event bus, narrative
storylets, AI motor/decision seams, and save/load pipeline.

## Chapter flow

```text
B1 wake / orient
  -> non-lethal systemic or aggressive B1 consequence
  -> credential / terminal / reader truth
  -> calibration
  -> medical service branch
  -> quiet staff route OR security escalation route
  -> elevator lobby
  -> authoritative Chapter One checkpoint
```

`CHAPTER_FLOW = VERIFIED_BOUNDED`.

The route is intentionally compact. The 15–25 minute first-time design target
is not a measured human-play duration; replay frame counts are not used as a
substitute for that measurement.

## Route receipts

`SYSTEMIC_ROUTE = VERIFIED`: B1 non-lethal/body concealment, durable Cleaner
history, credential acquisition, terminal access, reader/door crossing,
calibration, medical assessment, quiet staff route, elevator completion, save /
load, natural-language narrative text, and checkpoint are all asserted in the
Release replay.

`AGGRESSIVE_ROUTE = VERIFIED`: B1 lethal/loud action creates a runtime body and
different durable route facts, reaches the medical/security branch, permits
security checkpoint progression and bounded Guard health/combat proof, then
reaches the elevator. The final state is intentionally not identical to the
quiet route.

`THIRD_VARIATION = VERIFIED_BOUNDED`: durable Cleaner relationship/history
changes the later Cleaner response. Existing camera state and its consequence
remain available as a second bounded variation; no third campaign branch was
created.

Early decisions and later effects are explicit:

| Early durable consequence | Later effect |
|---|---|
| Non-lethal action plus hidden body and no loud action | `fact_chapter_quiet_route` opens the staff/service path; the player can reach the staff room and elevator without the security escalation route. |
| Lethal/loud action or exposed body | `fact_chapter_aggressive_route` selects the security path; medical/security state and Guard interaction differ before the elevator checkpoint. |
| Positive Cleaner relationship/history | Cleaner discovery evaluates durable history and can select `HelpCoverUp`; the result survives save/load. |
| Camera disabled | B1 surveillance/outage facts and current narrator reaction change; this is bounded consequence propagation, not a general surveillance simulation. |

## Room implementation

```text
CALIBRATION = VERIFIED_BOUNDED
MEDICAL = VERIFIED_BOUNDED
SECURITY = VERIFIED_BOUNDED
STAFF = VERIFIED_BOUNDED
ELEVATOR = VERIFIED_BOUNDED
```

Calibration now has an authored terminal, a new objective, and a required
transition into medical. Medical has an intake terminal and authored links to
calibration, staff, security, and the elevator. Staff is a compact quiet-route
space. Security has a checkpoint route and bounded Guard interaction. The
elevator requires the appropriate route fact/checkpoint state; merely reaching
an arbitrary coordinate does not complete Chapter One.

## Player-facing systems

```text
OBJECTIVES = VERIFIED_BOUNDED
INTERACTION_PROMPTS = VERIFIED_BOUNDED
PATROL = VERIFIED_BOUNDED
INVESTIGATE = VERIFIED_BOUNDED
STEALTH = VERIFIED_BOUNDED
COMBAT = VERIFIED_BOUNDED
BODY = VERIFIED
CLEANER = VERIFIED_BOUNDED
MEMORY = VERIFIED
CAMERA = VERIFIED_BOUNDED
TERMINALS = VERIFIED_BOUNDED
```

Objectives, prompts, terminal sessions, audit records, badge ownership and
door state all use the existing authoritative seams. Chapter text resolves to
natural-language UTF-8 strings; storylet identifiers are not shown as player
dialogue. The current Guard path proves active-room/visibility/range/cadence
damage, health loss, death and recovery. It is deliberately documented as a
bounded health/combat proof, not complete enemy combat AI.

The existing Cleaner motor reaches the authored work area before inspecting;
the decision is not a frame-threshold teleport. Patrol/investigate and stealth
remain bounded current-room capabilities. The slice does not claim building-
wide navigation, squad tactics, a complete combat AI, a full quest graph, or
final dialogue/audio/art.

## Persistence and failure boundaries

```text
MID_SAVE_LOAD = VERIFIED
DEATH_RECOVERY = VERIFIED_BOUNDED
BACKTRACKING = VERIFIED_BOUNDED
SOFTLOCK_TESTS = VERIFIED_BOUNDED
SAVE_FAULT_MATRIX = PASS (8/8 final-commit stages rolled back)
```

The quiet and aggressive Chapter One replays both save/load during the route
and continue to the checkpoint. The backtrack replay returns from medical to
calibration and back before completing. Recovery health/death and denied-route
replays remain green. The final-commit fault matrix covers
`after_room`, `after_world`, `after_systemic`, `after_events`, `after_rng`,
`after_narrative`, `after_ai`, and `after_player`; each injected failure leaves
the live state rolled back.

The soft-lock coverage is bounded rather than an exhaustive manual attack on
every possible order. Covered negative/counterfactual surfaces include no
badge, badge-only, terminal denial, reader denial, route denial, death
recovery, mid-route save/load, backtracking, quiet/aggressive divergence, and
durable Cleaner consequence.

## Replay evidence

Every current replay keeps these assertions separate:

```text
PROCESS_EXIT_OK
INPUT_CONSUMED
EXPECTED_STATE_REACHED
```

The current Release replay gate passed 15/15 cases:

```text
recovery_b1_success
recovery_b1_denied
recovery_b1_terminal_denied
recovery_b1_badge_only
recovery_b1_health_death
alpha01_systemic_success
alpha01_aggressive_success
alpha01_denied
alpha01_memory_consequence
chapter01_systemic
chapter01_aggressive
chapter01_denied_or_blocked
chapter01_mid_save_load
chapter01_backtrack
chapter01_memory_consequence
```

The Chapter One cases are real-input fixtures rather than direct state
injection. The denied case intentionally reaches a denied expected state; it
does not count denial as successful access.

## Local regression receipt

The final local run was performed against the current Release/Debug build,
not stale Alpha-01 output:

```text
DEBUG_CONFIGURE_BUILD = PASS
DEBUG_CTEST = PASS (1/1)
DEBUG_DIRECT_UNIT = PASS (206 tests, 0 failed)
RELEASE_CONFIGURE_BUILD = PASS
RELEASE_CTEST = PASS (1/1)
RELEASE_DIRECT_UNIT = PASS (206 tests, 0 failed)
CONTENT_CHECK = PASS
CONTENT_COMPILER_TESTS = PASS (13/13)
SYSTEMIC_SCHEMA_CHECK = PASS
SYSTEMIC_SCHEMA_TESTS = PASS (10/10)
INVALID_SEED_REGRESSION = PASS
STATIC_AUDIT = PASS (COUNT=0)
CONTRACT_CHECK = PASS
DEBUG_SMOKE = PASS
RELEASE_SMOKE = PASS
RECOVERY_AND_ALPHA_REPLAYS = PASS
CHAPTER01_REPLAYS = PASS (6/6)
SAVE_FAULT_MATRIX = PASS (8/8)
```

The Release benchmark passed its current budgets. Values are recorded with
their actual name, `worst_1pct_avg_ms`, rather than being relabeled p99:

```text
CHARACTER_RENDER_WORKLOAD_240x67 worst_1pct_avg_ms = 1.254
INTEGRATED_TOTAL_FRAME_PROXY_240x67 worst_1pct_avg_ms = 1.869
PVS_RENDER_BUDGET = PASS
PVS_TOTAL_FRAME_BUDGET = PASS
```

The benchmark is a controlled proxy with platform writes excluded. It is not
an end-to-end 120 Hz proof.

## Remote and manual boundary

```text
REMOTE_CI = SUCCESS (GitHub Actions run 34353383751; 5 jobs passed)
REMOTE_CI_HEAD = 79c893d9f8f109b54c9af7dd3814b9ee5ffb81d9
MANUAL_FOREGROUND_PLAY = PENDING_MANUAL
MEASURED_HUMAN_DURATION = NOT_MEASURED
FRONTEND_VISUAL_ACCEPTANCE = PENDING_MANUAL
```

The normal main push workflow is CI-only; the inspected release workflow is
tag/dispatch driven and is not triggered by an ordinary main push. No release,
tag, Steam action, branch, PR, or force push is part of this slice. Remote CI
must be filled from the actual final pushed head, not inferred from local
green output.

## Deferred scope

`DEFER_ASTRA = final prose/voice/audio, final visual/art acceptance, full
building content, complete enemy combat AI, broad SMG onboarding/art, full ADS,
general-purpose quest graph, manual first-time duration measurement, and
foreground Windows Terminal acceptance.`

The next useful human review is Chapter One comprehension and route quality,
not Chapter Two expansion. If that review finds a route-specific soft lock,
the fix should remain bounded to the current six-room slice.

## Status

```text
PRODUCT_GOLD = NOT_CLAIMED
CHAPTER01_GOLD = NOT_CLAIMED
VISUAL_GOLD = NOT_CLAIMED
READY_FOR_RELEASE = NOT_CLAIMED
USER_ACCEPTANCE = PENDING
FINAL_STATUS = READY_FOR_RAIN_CHAPTER01_FUNCTIONAL_REVIEW
```
