# WRITEOVER-07 — Integrated Recovery-04 Report

## Receipt identity

* Scope: final regression and full overnight-audit remediation.
* Baseline code head: 8bc65d025d756d1096d739e8872455b9a6f60e05.
* Source implementation head validated locally: b7cea9cca387f30a4b4d81b9b9f3d81f186c7859.
* Portable test receipt head: b6dacee155b1af3492f48eb02f0030dc7cc82ebe
  (assertion parentheses only; no runtime behavior change).
* Final documentation receipt head: 2895050ae6c4990315d1816adf5da3c53bd374e1.
* Branch: main.
* origin/main before the source-fix push: f2ab0bd151dfee8ba438e69af136af6ecb691cf6.
* Old tag/release: v0.1.0-pvs01-gold was not changed.
* Scope boundary: no new map, floor, weapon type, story chapter, release,
  tag, Steam operation, branch, PR, or renderer paradigm.

This report is an engineering receipt. It does not certify Product Gold,
Visual Gold, public-release readiness, or human visual acceptance.

## Implemented

The current implementation commit closes the real Recovery-04 defects found
by the overnight audit:

* B1 replay success now observes the objective while it is active, then
  separately asserts quest completion and checkpoint reach. A completed quest
  is not incorrectly required to retain an active objective.
* B1 target resolution gives an active body-drag cart the correct focus,
  keeps NPC interaction extents truthful, and keeps the cleaner route out of
  the cart footprint.
* The valid stunner/melee path establishes the recovery fact that its action
  semantically establishes and cancels an in-progress reload before changing
  weapon slot.
* Authored facts are loaded from the compiled artifact and validated
  fail-closed.
* Cleaner movement uses a motor step separate from the decision cadence and
  respects player personal space. Current grid routing avoids solid geometry
  and does not teleport on an unreachable route.
* The bounded guard health path uses intended medium sight range without
  applying distance visibility attenuation twice. LOS, room, pause, range and
  stunned/dead gates remain active.
* Room content rejects nonfinite geometry, duplicate cells, and ambiguous
  sparse-room defaults. Content outputs are staged and installed only after
  successful full-tree compilation.
* Room, infrastructure, fact, save, dialogue and console submissions reject
  the specific malformed states covered by the audit.
* Windows terminal output uses queried surface limits when a real console is
  present; oversized submissions are rejected and Win32 submission returns
  the actual API result. Non-foreground pipes retain an honest compatibility
  fallback.
* Save/load final-commit fault injection covers all eight required stages,
  and each staged failure preserves the live state.
* CI now includes the current five-case Recovery replay gate.
* fov, difficulty and frame_rate_cap have truthful bounded consumers;
  unsupported settings remain explicitly deferred in the capability matrix.

## Verified

### Current-head Recovery replays

The Release binary was rebuilt from the implementation head. The replay gate
checks these fields independently:

* PROCESS_EXIT_OK
* INPUT_CONSUMED
* EXPECTED_STATE_REACHED
* REPLAY_RESULT

The current five-case result is:

| Replay | Result | Counterfactual/state proved |
|---|---|---|
| recovery_b1_success | PASS | Valid B1 route: non-lethal body lifecycle, badge possession, terminal session, gate open and cross, save/load, cleaner consequence, narrative action, objective presentation and checkpoint. |
| recovery_b1_denied | PASS | Without the credential, reader access is denied and the gate is not opened or crossed. |
| recovery_b1_terminal_denied | PASS | Terminal denial does not create a successful terminal session or route progression. |
| recovery_b1_badge_only | PASS | Obtaining a badge alone does not imply access or checkpoint completion. |
| recovery_b1_health_death | PASS | Bounded guard damage reaches authoritative player death and the recovery assertions. |

The success receipt includes
QUEST_PRESENTED_DURING_ROUTE=YES, GATE_OPEN=YES,
GATE_CROSSED=YES, SAVE_OK=YES, LOAD_OK=YES,
NARRATIVE_VISIBLE_ACTION=YES and CHAPTER_CHECKPOINT_REACHED=YES.
The denied cases preserve their negative world state rather than passing on
process exit alone.

### Save/load fault matrix

All eight required final-commit injection points returned
failure_stage=True and rollback=True:

after_room, after_world, after_systemic, after_events, after_rng,
after_narrative, after_ai, after_player.

The comparison covered room, player, systemic, world infrastructure/facts,
events, RNG, narrative and AI state. No partial mutation was accepted.

### Local validation

| Gate | Command/result |
|---|---|
| Debug configure | cmake --preset debug — exit 0 |
| Debug build | cmake --build --preset debug --config Debug — exit 0 |
| Debug CTest | ctest --test-dir out/build/debug -C Debug --output-on-failure — 1/1 pass |
| Debug direct unit | out/build/debug/Debug/writeover_tests.exe — 206 tests, 0 failed, exit 0 |
| Release configure | cmake --preset release — exit 0 |
| Release build | cmake --build --preset release --config Release — exit 0 |
| Release CTest | ctest --test-dir out/build/release -C Release --output-on-failure — 1/1 pass |
| Release direct unit | out/build/release/Release/writeover_tests.exe — 206 tests, 0 failed, exit 0 |
| Content compiler tests | python tools/contentc/test_contentc.py — 13/13 pass |
| Production content compile | python tools/contentc/contentc.py --data-dir data --out-dir data — exit 0 |
| Production content check | python tools/contentc/contentc.py --data-dir data --out-dir data --check — exit 0 |
| Systemic schema check | SYSTEMIC_SCHEMA_CHECK=PASS (1 file) |
| Systemic schema tests | 10/10 pass |
| Invalid seed | rejected as expected |
| Static audit | COUNT=0 |
| Contract/dependency/public-header checks | all OK; no public header changed |
| Debug smoke | exit 0 |
| Release smoke (direct writeover_app.exe --smoke) | exit 0 |
| Recovery replay gate | five cases pass with independent exit/input/state fields |
| Save fault matrix | 8/8 rollback cases pass |
| Recovery spatial gates | current spatial tests pass inside the 206-test executable |

The CMake project has no ctest --preset release preset; the direct Release
CTest command above is the equivalent current configuration check. The
generic scripts/smoke.ps1 wrapper also assumes a matching CTest preset and
therefore is not a valid Release invocation here; its direct Release
writeover_app.exe --smoke equivalent passed. These are command-shape
differences, not omitted Release application tests.

### Performance receipt

The Release benchmark reports the metric using its honest name:
worst_1pct_avg_ms. It does not call that value p99.

* Character render workload at 240x67: 1.308 ms.
* Integrated character/runtime proxy at 240x67: 2.220 ms, excluding platform
  writes.
* Terminal full/delta/unchanged and systemic lookup/update budgets: PASS.

This is a bounded benchmark proxy. It is not proof of end-to-end terminal
cadence or 120 Hz presentation.

## Not verified or deliberately deferred

The following are not silently promoted:

* A foreground Windows Terminal session with actual VT negotiation,
  dimensions, color capability and screenshot was not available to the current
  automation surface. Non-foreground smoke reported
  TERMINAL_BACKEND=win32-writeconsole,
  TERMINAL_QUALITY_PRESET=COMPATIBILITY,
  TERMINAL_COLOR_CAPABILITY=ANSI16 and
  TERMINAL_PROBE=legacy-conhost. This is honest pipe/fallback evidence, not a
  Windows Terminal acceptance result.
* EventBus dynamic subscriber lifetime tokens, semantic NPC profile identity
  bytes, absolute raw mouse input, input context switching, RayResult
  truncation consumers, larger navigation scaling, and a universal
  directional-art catalogue remain bounded future work.
* gamepad_sensitivity, aim_assist, interaction_highlight and tactical_focus
  have no current production consumer. They remain DEFERRED in the matrix.
* Historical PVS01 replay fixtures are preserved but are not current
  mandatory gates when their interaction assumptions are obsolete. Each is
  classified below rather than weakened.
* No product Gold, visual Gold or public release conclusion is made.

| Fixture | Classification | Reason |
|---|---|---|
| tools/replay/pvs01_normal.txt | MIGRATE | Preserves the earlier normal route, but its literal timings/targets predate the current identity-based recovery route. Re-author if the route is revived. |
| tools/replay/pvs01_aggressive.txt | MIGRATE | The aggressive intent remains a useful future counterfactual, but its old input schedule is not a current B1 oracle. |
| tools/replay/pvs01_stealth.txt | MIGRATE | The stealth intent can be retained, but it must use the current visibility and target contracts before becoming mandatory. |
| tools/replay/pvs01_systemic.txt | HISTORICAL_ONLY | It records the earlier systemic/narrator demonstration and is superseded for current Recovery-04 assertions by the current success route and visible-action receipt. |

The cross-platform-load fixture is also preserved as historical operational
material; it is not a current local gameplay oracle.

## Audit reconciliation

The complete per-ID ledger is in
docs/audit/OVERNIGHT_AUDIT_REMEDIATION.md. All 46 IDs are classified:

* CONFIRMED: 36
* ALREADY_FIXED: 2
* DEFER_ASTRA: 8
* REJECTED: 0
* DUPLICATE: 0

The original P0 DS-AUDIT-0001 is closed by current target-resolution code and
current success replay evidence. No FATAL finding exists. Deferred entries
have an explicit reason and remaining risk; they are not used to claim that a
larger product capability already exists.

## Push and remote status

Final remote receipt:

* SOURCE_IMPLEMENTATION_HEAD = b7cea9cca387f30a4b4d81b9b9f3d81f186c7859
* FINAL_RECEIPT_HEAD = 2895050ae6c4990315d1816adf5da3c53bd374e1
* ORIGIN_MAIN_BEFORE_PUSH = f2ab0bd151dfee8ba438e69af136af6ecb691cf6
* PUSH = YES
* REMOTE_CI = PASS (GitHub Actions run 34317014333; all five jobs passed)

The workflow was inspected before push: a normal main push runs CI and does
not create a Release, move a tag, upload Steam, or create a PR. Final remote
status must be filled from the actual final-head Actions result.

The first documentation push at f2ab0bd triggered run 34313672985. Windows
passed its build/recovery path, while Linux, Linux Clang, ARM64 link and
macOS rejected the same unused local near helper under -Werror. That
cross-platform defect was removed in b7cea9c, the portable test assertion was
closed in b6dacee, and the current Windows regression was rerun before the
final push. The separate foreground-terminal evidence remains manual. The
initial macOS benchmark attempt hit a one-off unchanged-frame outlier; its
single failed job was rerun without threshold changes and passed.

## Status at this receipt

ENGINEERING_REGRESSION = PASS
AUDIT_RECONCILIATION = COMPLETE
P0 = CLOSED
OPEN_FATAL = 0
OPEN_MAJOR = 0
OPEN_ACTIVE_P1 = 0
DEFERRED_P1 = 1 (DS-AUDIT-0042, unsupported settings explicitly reserved)
FRONTEND_TERMINAL_EVIDENCE = PENDING_MANUAL
USER_ACCEPTANCE = PENDING

FINAL_STATUS = READY_FOR_RAIN_GAMEPLAY_SYSTEM_REVIEW

## Downstream Alpha-02 functional chapter handoff

Recovery-04 remains the audited foundation receipt above. The bounded
Vertical Slice Alpha-02 implementation is recorded separately in
`docs/production/VERTICAL_SLICE_ALPHA02_CHAPTER01_REPORT.md` and the newer
section of `docs/production/CAPABILITY_TRUTH_MATRIX.md`. It reuses the six
existing rooms and current systems; it does not promote the foundation to a
whole-building game, complete combat AI, final prose, visual Gold, or release
readiness. Current implementation head:
`79c893d9f8f109b54c9af7dd3814b9ee5ffb81d9` (Alpha-02 implementation plus
the cross-platform unused-helper fix). The final validated code head passed
GitHub Actions run `34353383751`; the separate Alpha-02 receipt remains the
source of its bounded Chapter One claims.

## Alpha-03 continuation addendum — current status

The first completed Chapter One audit was reconciled against current code in
`docs/audit/CHAPTER01_AUDIT_REMEDIATION.md`. The current implementation head is
`2efde27325dd26baeb2fdc33b584acdfa81c6911`, with fresh local receipts: 18/18
current replay gate, a 36-row scenario matrix with 18/18 authored executions,
8/8 final-commit save rollback stages, 206/206 direct tests in both
configurations, content 13/13, systemic schema 10/10, and Debug/Release smoke
plus benchmark PASS. The disabled-guard elevator route, no-save death
recovery, B1 terminal skip, incapacitated guard body, objective receipt,
guard-death fact, facility-alert objective consumer and runtime text package
defect are addressed within the bounded Chapter One scope.

This addendum does not reuse the older 15/15 statement as current evidence and
does not promote the bounded health/death proof to complete enemy combat.
Audit-02B is still incomplete: no `AUDIT_COMPLETE.flag`,
`FILE_COVERAGE_PASS=NO`, and 0% coverage in all three required passes. Its
findings are therefore unverified and the current Alpha-03 final status is
`NOT_READY` pending that mechanical coverage gate and subsequent reconciliation.

The two focused commits were pushed by a normal `main` update. Final remote
head is `a4ad377e7330a4413c49da4078ee2999a3468f83`; GitHub Actions run
`34374763958` completed successfully for all five jobs, including the Windows
18-case replay/package/benchmark path and the Linux/macOS/ARM gates. No release,
tag or Steam action was invoked.
