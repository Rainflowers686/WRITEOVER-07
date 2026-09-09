# WRITEOVER-07 — Overnight Engineering Report

## Purpose

This report closes the overnight audit handoff for Recovery-04. It records
engineering truth at source implementation head
b7cea9cca387f30a4b4d81b9b9f3d81f186c7859, portable test receipt head
b6dacee155b1af3492f48eb02f0030dc7cc82ebe, and final documentation receipt
head 2895050ae6c4990315d1816adf5da3c53bd374e1; it is intentionally separate from
visual or Product Gold acceptance.

## Work completed

* Repaired the unsatisfiable B1 quest replay assertion by recording objective
  presentation during the route separately from completed-at-end state.
* Verified and closed the cart/cleaner target-shadowing P0.
* Repaired authored fact seeding, cleaner motor separation, bounded guard
  health/death behavior and double visibility attenuation.
* Added finite geometry, duplicate-cell, sparse-room, binary trailing-data,
  infrastructure/fact sentinel and save-size validation.
* Made content compilation atomic for the full output set.
* Added current Recovery replay coverage to main CI.
* Added truthful terminal surface fitting and diagnostics, without claiming a
  foreground Windows Terminal result when no foreground host was available.
* Preserved fixed-step simulation timing while consuming frame_rate_cap only
  at presentation.

## Regression receipt

The final local Release binary ran 206 direct tests with 0 failed; Debug direct
tests also ran 206 with 0 failed. Debug and Release configure/build and CTest
passed. Content compiler tests passed 13/13; systemic schema tests passed
10/10; production content check passed; static audit returned COUNT=0;
contract/dependency/public-header checks returned OK; Debug and Release smoke
exited 0; the five current Recovery replay cases passed with independent
process/input/state/result fields; and all 8 save final-commit fault stages
rolled back.

The benchmark receipt uses worst_1pct_avg_ms, not p99: character rendering
measured 1.308 ms and the integrated proxy measured 2.220 ms with platform
writes excluded. This does not prove end-to-end 120 Hz terminal presentation.

## Audit disposition

All 46 findings are classified in
docs/audit/OVERNIGHT_AUDIT_REMEDIATION.md. The current disposition is:

* CONFIRMED: 36
* ALREADY_FIXED: 2
* DEFER_ASTRA: 8
* REJECTED: 0
* DUPLICATE: 0

The full capability boundary is in
docs/production/CAPABILITY_TRUTH_MATRIX.md. In particular, guard damage is a
bounded authoritative health proof rather than complete enemy combat; current
settings without consumers remain deferred; and current B1/1F navigation,
scene entities, narrative and NPC behavior are not promoted to whole-game
systems.

## Historical replay policy

The old PVS01 replay files remain preserved as historical material. They are
not weakened or silently included in the current Recovery-04 mandatory
claims when they require obsolete interaction assumptions. Their disposition
is documented with the audit ledger.

## Human/runtime acceptance boundary

The engineering gates do not certify visual quality. The available automation
surface had no controllable native Windows Terminal, so real foreground
dimensions, VT negotiation, color capability, clipping behavior and screenshots
remain PENDING_MANUAL. The non-foreground pipe smoke result is retained as a
fallback diagnostic only.

The first remote CI run for f2ab0bd (run 34313672985) exposed one real
portability defect: Linux, Linux Clang, ARM64 link and macOS treated an
unused local interaction helper as an error under -Werror. The helper was
removed in b7cea9c without changing behavior, and the affected Windows
regression was rerun locally. The next final-head run exposed two portable
operator-precedence warnings in an existing test; b6dacee added explicit
parentheses only. Final run 34317014333 passed build, test, replay, benchmark
and static/contract jobs on Windows, Linux GCC, Linux Clang, macOS ARM64 and
the ARM64 link gate. Its first macOS benchmark attempt was a one-off
unchanged-frame budget outlier; rerunning only that failed job passed without
changing the threshold.

PRODUCT_GOLD = NOT_CLAIMED
VISUAL_GOLD = NOT_CLAIMED
READY_FOR_PUBLIC_RELEASE = NOT_CLAIMED
USER_ACCEPTANCE = PENDING

## Final handoff fields

IMPLEMENTATION_HEAD = 2895050ae6c4990315d1816adf5da3c53bd374e1
LOCAL_ENGINEERING_REGRESSION = PASS
AUDIT_RECONCILIATION = COMPLETE
OPEN_FATAL = 0
OPEN_P0 = 0
OPEN_ACTIVE_P1 = 0
DEFERRED_P1 = 1 (DS-AUDIT-0042, unsupported settings explicitly reserved)
REMOTE_CI = PASS (run 34317014333)
FRONTEND_EVIDENCE = PENDING_MANUAL
FINAL_STATUS = READY_FOR_RAIN_GAMEPLAY_SYSTEM_REVIEW

## Downstream Alpha-02 note

The next bounded product slice is documented in
`docs/production/VERTICAL_SLICE_ALPHA02_CHAPTER01_REPORT.md`. It adds only
functional Chapter One content on top of the audited Recovery-04 foundation:
six existing rooms, authored transitions/entities, route-specific durable
facts, objective/storylet progression, replay receipts, and mid-route
save/load coverage. Manual first-time duration and foreground visual review
remain pending; no Gold or release claim is made. Source implementation head:
`79c893d9f8f109b54c9af7dd3814b9ee5ffb81d9` (Alpha-02 implementation plus
the cross-platform unused-helper fix). The final validated code head passed
GitHub Actions run `34353383751`; this downstream note does not promote
manual play, visual acceptance, Product Gold, or release readiness.

## Alpha-03 continuation addendum — current-head truth

The current bounded remediation commit is
`2efde27325dd26baeb2fdc33b584acdfa81c6911`, based on
`4c809a8b0340808b1db287db3f71a340bcc28ea4`. It re-issued the replay receipt
from the committed gate rather than relying on the historical Alpha-02 text:
18/18 current cases pass, including six Chapter One routes and three new
regression fixtures. A 36-row scenario matrix classified 18 authored runs as
11 completable, 5 expected denial and 2 expected failure/recovery; its other
18 rows are explicit `INVALID_SETUP`, not fake passes.

Fresh local engineering results are Debug/Release build and CTest PASS,
206/206 direct tests in both configurations, content 13/13, systemic schema
10/10, invalid seed PASS, static audit COUNT=0, contract PASS, Debug/Release
smoke PASS, 8/8 final-commit fault rollback PASS and Release benchmark
`OVERALL_BUDGET=PASS`. Benchmark fields retain their actual
`worst_1pct_avg_ms` name and are not end-to-end frame-rate proof. Clean package
smoke also passes with the authored character/text resources.

The first audit's 14 records are fully classified in
`docs/audit/CHAPTER01_AUDIT_REMEDIATION.md`; all four original P1s are closed
for the current bounded scope. Supplemental Audit-02B is not complete: its
coverage status is `FILE_COVERAGE_PASS=NO` with 0% PASS1/PASS2/PASS3 and no
findings handoff. Consequently:

```text
ALPHA03_LOCAL_REGRESSION = PASS
ALPHA03_REMOTE_CI = PASS (run 34374763958; final head a4ad377e7330a4413c49da4078ee2999a3468f83; all five jobs)
AUDIT02B = NOT_COMPLETE
FINAL_STATUS = NOT_READY

## Alpha-04 continuation addendum — current-head truth

Audit-02B has since completed its mechanical coverage gate and was reconciled
against implementation checkpoint `71d2aa01efea5e6b7f08035bd66146956eb5bcd1`.
The complete 29-record ledger is
`docs/audit/CHAPTER01_AUDIT02B_REMEDIATION.md`; it contains 0 unclassified
records, 0 current Fatal, and 0 current P0. The current Alpha-04 engineering
receipt is `docs/production/ALPHA04_CHAPTER01_NEAR_BETA_REPORT.md`.

Fresh local receipts at this implementation content are Debug/Release build
and CTest, 209/209 direct tests in each configuration, content 13/13,
systemic schema 10/10, invalid seed, static audit COUNT=0, contract, Debug/
Release smoke, 19/19 replay gate, 36-row scenario classification, 8/8 save
fault rollback probes, package positive/negative smoke, and Release benchmark.
The implementation checkpoint adds only focused runtime/test/package/contract
closure; it does not claim complete enemy combat AI or manual visual
acceptance. The normal `main` push and final-head remote CI receipt are still
pending at this pre-push addendum.
```

## Alpha-04 final remote receipt

The Alpha-04 documentation receipt was pushed normally to `main` at
`5711e68cf15343c016913b8391092d3dfb1248d4`. GitHub Actions run
`34390875567` completed successfully for that exact head. All five configured
jobs passed: Windows build/replay/package/benchmark, Linux GCC, Linux Clang,
macOS ARM64, and Linux ARM64 link. No release, tag, Steam, branch, PR, or
force-push action was used.

```text
FINAL_HEAD = 5711e68cf15343c016913b8391092d3dfb1248d4
ORIGIN_MAIN = 5711e68cf15343c016913b8391092d3dfb1248d4
REMOTE_CI = PASS (34390875567)
LOCAL_ENGINEERING_REGRESSION = PASS
AUDIT02B_RECONCILIATION = COMPLETE
FRONTEND_EVIDENCE = PENDING_MANUAL
FINAL_STATUS = READY_FOR_RAIN_GAMEPLAY_SYSTEM_REVIEW
```

## Alpha-04 post-receipt portability correction

The first documentation receipt run `34392141539` failed only on macOS arm64
in the existing shutdown test: the fixed-step inner loop could continue after
`RequestStop()` and execute a second accumulated tick. The minimal source fix
is `c6ccfe05f26be27907f10c35ce7a11facd168b1c` in
`src/core/engine.cpp`; it adds the existing `running_` state to that loop
condition. No gameplay or content behavior was changed.

Fresh local post-fix regression passed, including Debug/Release builds and
CTest, 209/209 direct tests in both configurations, content 13/13, systemic
schema 10/10, invalid seed, static audit, contract, Debug/Release smoke,
19/19 current replays, the 36-row scenario matrix, eight expected-failed save
rollback probes, package smoke, and Release benchmark. Corrected-head remote
CI is pending the normal main push.

```text
CURRENT_SOURCE_HEAD = c6ccfe05f26be27907f10c35ce7a11facd168b1c
REMOTE_CI_AFTER_CORRECTION = PENDING_PUSH
FINAL_STATUS = NOT_READY_UNTIL_CORRECTED_HEAD_REMOTE_CI_VERIFIED
```

## Corrected-head remote CI receipt

The focused cross-platform shutdown correction was pushed with its report
receipt at `6622c079d63cc3a05e819d879adeaf9c17fb664a`. GitHub Actions run
`34396017075` completed successfully after rerun attempt 2 for that exact
head. The rerun passed all five jobs, including macOS arm64 Release
benchmark; the initial benchmark-only failure remains recorded and no gate
threshold was changed.

```text
CORRECTED_SOURCE_COMMIT = c6ccfe05f26be27907f10c35ce7a11facd168b1c
PUSHED_RECEIPT_HEAD = 6622c079d63cc3a05e819d879adeaf9c17fb664a
REMOTE_CI = PASS (run 34396017075, rerun attempt 2)
FRONTEND_EVIDENCE = PENDING_MANUAL
FINAL_STATUS = READY_FOR_RAIN_GAMEPLAY_SYSTEM_REVIEW
```
