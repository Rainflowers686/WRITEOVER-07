# WRITEOVER-07 — Alpha-03 Chapter One hardening receipt

This report covers the Alpha-03 continuation and first-audit remediation
performed from the Alpha-02 baseline. It does not claim Chapter One Gold,
Product Gold, Visual Gold, or release readiness.

## Scope

Implemented only bounded Chapter One correctness and shipping-gate work:

* closed the disabled-guard elevator soft-lock with a durable checkpoint fact;
* routed the B1 crossing through the shared transition policy;
* added truthful no-save death recovery;
* made incapacitated runtime NPC bodies room-independent and renderable;
* corrected durable guard-death fact and quest-objective presentation receipt;
* made the current facility alert observable through the security objective;
* widened the bounded character-art parser budget;
* packaged the current runtime character/text resources and strengthened package smoke;
* made CI execute the current replay gate;
* added current negative fixtures and a bounded scenario matrix.

No new floor, chapter, weapon type, large AI system, renderer, or release was
added. Historical user visual evidence, settings and the pre-existing dirty
`tests/test_harness.cpp` change were preserved and were not staged.

## Current-head fields

```text
START_HEAD = 4c809a8b0340808b1db287db3f71a340bcc28ea4
IMPLEMENTATION_HEAD = 2efde27325dd26baeb2fdc33b584acdfa81c6911
FINAL_HEAD = 2efde27325dd26baeb2fdc33b584acdfa81c6911 (validated code head; documentation commit follows)
ORIGIN_MAIN_AT_REPORT = 4c809a8b0340808b1db287db3f71a340bcc28ea4 (not pushed yet)
BRANCH = main
```

## First audit reconciliation

```text
FIRST_AUDIT_FINDINGS = 14
FIRST_AUDIT_P1 = 4
FIRST_AUDIT_P1_CLOSED = 4
FIRST_AUDIT_LEDGER = docs/audit/CHAPTER01_AUDIT_REMEDIATION.md
OPEN_FIRST_AUDIT_FATAL = 0
OPEN_FIRST_AUDIT_P0 = 0
OPEN_FIRST_AUDIT_NORMAL_PLAY_P1 = 0
```

Every first-audit record is classified. The four P1s and all current normal-
play P2s are fixed and tested. DS-C1-AUDIT-0005 and 0007 remain explicit
objective `FUTURE_SCOPE` boundaries; neither is presented as a current live
capability. DS-C1-AUDIT-0004 is retained as the audit's corrected
`FALSE_POSITIVE`, with its residual fact defect tracked and fixed under 0012.

## Required remediation

| Gate | Current result | Evidence |
|---|---|---|
| `ELEVATOR_SOFTLOCK` | `FIXED_AND_VERIFIED` | `chapter01_security_bypass`, current 18-case gate, durable checkpoint receipt |
| `NO_SAVE_DEATH_RECOVERY` | `FIXED_AND_VERIFIED` | `chapter01_no_save_death`, `PLAYER_RESTARTED=YES`, health 100, not dead |
| `PACKAGE_RUNTIME_ASSETS` | `FIXED_AND_VERIFIED` | final Windows package build and package smoke |
| `CI_FULL_REPLAY_GATE` | `18_CASES_CONFIGURED` | gate contains 18 current cases; remote receipt pending until push |
| `B1_TERMINAL_SKIP` | `FIXED_AND_VERIFIED` | terminal-skip denial fixture |
| `INCAPACITATED_GUARD` | `FIXED_AND_VERIFIED` | generalized BodyRecord/render path and security bypass route |
| `OBJECTIVE_PRESENTATION` | `FIXED_AND_VERIFIED` | B1-prefixed presentation receipt plus completion assertion |
| `SAVE_FINAL_COMMIT` | `8/8 PASS` | fresh stage injection: room/world/systemic/events/rng/narrative/ai/player |

## Scenario matrix

```text
SCENARIO_MATRIX_TOTAL = 36
SCENARIOS_EXECUTED = 18
SCENARIOS_EXPECTED_SUCCESS = 11
SCENARIOS_EXPECTED_DENIAL = 5
SCENARIOS_EXPECTED_FAILURE_STATE = 2
SCENARIOS_INVALID_SETUP = 18
SCENARIO_MATRIX = PASS
```

The 18 runnable cases were executed against the current Release binary. The 18
`INVALID_SETUP` rows are deliberately named combinations for which no current
authored fixture exists; they are not counted as successful gameplay. The
negative routes assert denial or bounded recovery, not process exit alone.

```text
SOFTLOCKS_FOUND = 2 (disabled-guard elevator; no-save death)
SOFTLOCKS_FIXED = 2
ROUTE_SKIPS_FOUND = 1 (B1 terminal)
ROUTE_SKIPS_FIXED = 1
SECOND_DELAYED_CONSEQUENCE = VERIFIED_BOUNDED (loud/report/camera state affects later security objective/response; no new morality framework)
AI_LEGIBILITY = VERIFIED_BOUNDED (current cleaner/guard movement and objective cues; no complete AI claim)
BACKTRACKING = PASS (chapter01_backtrack)
MID_SAVE_MATRIX = PASS (chapter01_mid_save_load plus quiet/aggressive route saves)
DEATH_RECOVERY_MATRIX = PASS (checkpoint death and no-save room restart)
PACKAGE_SMOKE = PASS
```

## Regression receipt

All listed checks were run freshly at the current implementation head unless
marked otherwise below:

```text
DEBUG_CONFIGURE = PASS
DEBUG_BUILD = PASS
DEBUG_CTEST = PASS (1/1)
DEBUG_DIRECT_UNIT = PASS (206 tests, 0 failed)
RELEASE_CONFIGURE = PASS
RELEASE_BUILD = PASS
RELEASE_CTEST = PASS (1/1)
RELEASE_DIRECT_UNIT = PASS (206 tests, 0 failed)
CONTENT_CHECK = PASS
CONTENT_TESTS = PASS (13/13)
SYSTEMIC_SCHEMA_CHECK = PASS
SYSTEMIC_SCHEMA_TESTS = PASS (10/10)
INVALID_SEED = PASS
STATIC_AUDIT = PASS (COUNT=0)
CONTRACT_CHECK = PASS
DEBUG_SMOKE = PASS
RELEASE_SMOKE = PASS
RELEASE_REPLAY_GATE = PASS (18/18)
SCENARIO_MATRIX = PASS (18/18 runnable; 36 rows total)
SAVE_FAULT_MATRIX = PASS (8/8 rollback stages)
RELEASE_BENCHMARK = PASS (OVERALL_BUDGET=PASS; fields are worst_1pct_avg_ms, not p99)
```

The Release benchmark reported `character_render_workload_240x67` at
`1.208 ms` worst-one-percent average and
`character_total_runtime_frame_240x67` at `1.860 ms`, with platform writes
excluded. This is a bounded proxy and not end-to-end 120 Hz proof.

## Package and terminal boundary

The clean Windows package smoke passed with executable-relative runtime data,
authored character/text resources, no developer garbage and separated user
data. Non-foreground local pipe smoke still honestly reports the
`legacy-conhost`/`win32-writeconsole` compatibility fallback and ANSI16. A
foreground Windows Terminal acceptance session was not fabricated; it remains
manual evidence, not a code PASS.

## Legacy replay classification

The historical `pvs01_normal`, `pvs01_aggressive`, `pvs01_stealth` and
`pvs01_systemic` files remain preserved. They are `MIGRATE`/`HISTORICAL_ONLY`
where their interaction assumptions predate the current Chapter One route; they
are not weakened or used to override current negative semantics. The current
mandatory gate is the 18-case Release gate listed above.

## Audit-02B status

```text
AUDIT02B_COVERAGE = NOT_COMPLETE
AUDIT02B_FINDINGS = UNAVAILABLE
AUDIT02B_CONFIRMED_CURRENT = UNVERIFIED
AUDIT02B_ALREADY_FIXED = UNVERIFIED
AUDIT02B_FALSE_POSITIVE = UNVERIFIED
```

The supplemental root has no completion flag, has
`FILE_COVERAGE_PASS=NO`, and reports 0% for all three required coverage
passes. This is an open process blocker, not a finding count of zero. Details
are in `docs/audit/CHAPTER01_AUDIT02B_REMEDIATION.md`.

## Final status at this checkpoint

```text
LOCAL_REGRESSION = PASS
REMOTE_CI = NOT_RUN (main has not been pushed)
TEMP_CLEANUP = PARTIAL (task probe/package/unit logs are disposable; audit roots and recovery checkpoint retained)
DEFER_ASTRA = subjective art/prose/audio and foreground-terminal manual acceptance only; no objective defect is hidden here
OPEN_FATAL = 0
OPEN_P0 = 0
OPEN_NORMAL_PLAY_P1 = 0 (first audit/current scope)
FINAL_STATUS = NOT_READY
BLOCKER = Audit-02B mechanical coverage gate not complete; current 0% coverage and no findings handoff
```

The maximum truthful state is therefore not Chapter One hardened review yet.
After the supplemental audit completes its coverage gate, its records must be
classified against the then-current head before a READY status can be issued.
