# WRITEOVER-07 — Alpha-04 Chapter One Near-Beta Engineering Receipt

This report records the bounded Alpha-04 continuation and Audit-02B
remediation at the current implementation head. It is an engineering receipt,
not a Product Gold, Visual Gold, release, or manual-play acceptance claim.

## Scope and safety boundary

The work stayed on `main` and reused the existing Debug/Release build trees.
It did not add Chapter Two, new floors, new story content, new weapon types,
new AI architecture, a new renderer, a tag, a release, Steam publication, a
branch, a PR, or a force push. Existing dirty visual evidence, settings files,
and the one-line `tests/test_harness.cpp` worktree change were preserved and
were not staged.

The completed supplemental Audit-02B snapshot was consumed from:

```text
D:\AAAbiancheng\00_Projects\_audit_runs\WRITEOVER-07\20260909_chapter01_audit_02B_coverage
```

Its coverage gate is mechanical `PASS`: 430 files, 432 chunks, PASS1/PASS2/
PASS3 coverage 100%, 0 unreviewed files/chunks, and 29 findings. The full
current-head classification is in
`docs/audit/CHAPTER01_AUDIT02B_REMEDIATION.md`.

## Git and current-head identity

```text
START_IMPLEMENTATION_HEAD = 29a13aa03f50c1c67de32e198490300a4969d849
CURRENT_IMPLEMENTATION_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
ORIGIN_MAIN_AT_START = 29a13aa03f50c1c67de32e198490300a4969d849
IMPLEMENTATION_COMMIT = 71d2aa0 runtime: close chapter audit and shutdown contract gaps
WORKTREE = DIRTY_PREEXISTING_ONLY plus this report/matrix/ledger until receipt commit
```

`71d2aa0` is the focused implementation checkpoint. Documentation receipt
commits after it do not change the implementation evidence described below.

## Audit-02B reconciliation

All 29 records are classified; none is left unclassified.

| Classification | Count | IDs |
|---|---:|---|
| `CONFIRMED_CURRENT` | 8 | 0001, 0017, 0018, 0019, 0020, 0021, 0023, 0024 |
| `ALREADY_FIXED_BY_PARALLEL_WORK` | 11 | 0002, 0003, 0004, 0005, 0006, 0007, 0008, 0009, 0010, 0015, 0016 |
| `STALE_AFTER_CHANGE` | 2 | 0014, 0022 |
| `FUTURE_SCOPE` | 4 | 0011, 0012, 0025, 0026 |
| `FALSE_POSITIVE` | 4 | 0013, 0027, 0028, 0029 |
| `DUPLICATE` | 0 | — |
| `DEFER_ASTRA` | 0 | — |
| `UNCLASSIFIED` | 0 | — |

The original five P1 records are closed at the current bounded scope: normal
quit, disabled-guard elevator progression, packaged art/text resources, the
full replay gate, and no-save death recovery. The ledger records reproduction,
root cause, fix, test, result, commit, and remaining risk for every record.
There are no current Fatal or P0 findings in the 02B ledger.

## Implemented in this continuation

The focused source/test checkpoint contains:

- normal player quit through the existing paused `Q` affordance;
- engine termination that calls registered module `Shutdown()` exactly once;
- idempotent input-runtime shutdown;
- a real standalone public-header compile gate instead of header-only object
  targets;
- fail-closed `SystemicWorld::Save`/`Serialize` propagation and search-event
  rollback;
- corrected test bounds and direct `<cstdint>` ownership;
- removal of brittle README test-count and active-ADS claims;
- the current 19-case recovery/Alpha/Chapter/regression replay gate;
- no changes to the existing bounded guard health/combat proof, which remains
  explicitly not complete enemy combat AI.

The earlier remediation already present at the starting head was re-verified,
not reimplemented: elevator soft-lock closure, no-save death restart,
terminal-gate enforcement, incapacitated actor body handling, bounded facility
alert consumption, guard-death fact, objective presentation receipt, and
packaged runtime resource separation.

## Verified locally at current implementation content

The following were run against the current Release/Debug trees and current
source content. Counts below are receipts, not permanent API guarantees.

| Gate | Result |
|---|---|
| Debug configure/build | PASS |
| Debug CTest | PASS, 2/2 |
| Debug direct unit executable | PASS, 209 tests, 0 failed |
| Release configure/build | PASS |
| Release CTest | PASS, 2/2 with `-C Release` |
| Release direct unit executable | PASS, 209 tests, 0 failed |
| Public-header standalone executable | PASS |
| `contentc.py --check` | PASS |
| Content compiler tests | PASS, 13/13 |
| Systemic schema check | PASS |
| Systemic schema tests | PASS, 10/10 |
| Invalid-seed check | PASS |
| Static audit | PASS, `COUNT=0` |
| Contract check | PASS |
| Debug smoke | PASS |
| Release smoke | PASS |
| Release recovery/Alpha/Chapter/regression replay gate | PASS, 19/19 |
| Scenario matrix | PASS, 36 classified / 18 executed / 18 `INVALID_SETUP` |
| Save final-commit fault matrix | PASS, 8/8 expected rollback probes |
| Release benchmark | PASS, `OVERALL_BUDGET=PASS` |

The 8 save fault runs intentionally return `REPLAY_EXPECTED_STATE_REACHED=NO`
and `LOAD_OK=NO` after the injected failure. They are
`EXPECTED_FAILED_PROBE`s: the positive assertion is rollback and unchanged
live state, not a fake successful load.

The Release benchmark reports the tool's actual
`worst_1pct_avg_ms` field; it is not relabeled as p99. Recorded current values
include 0.014 ms systemic lookup, 0.206 ms systemic update, 1.051 ms
character render workload, and 1.635 ms integrated character runtime frame
proxy with platform writes excluded. This is not end-to-end 120 Hz proof.

## Package and negative-probe evidence

The clean Windows package was built with `tools/release/package_release.py` and
passed `tools/release/package_smoke.py`. The package smoke checks executable-
relative runtime resources, user-data separation, authored character/text
resources, required content binaries, and developer-tree exclusion.

A derived test archive with
`data/characters/b1_character_art.txt` removed failed package smoke with the
expected missing-runtime-resource error. That negative run is recorded as an
`EXPECTED_FAILED_PROBE`, not as a package PASS.

## Scenario and legacy replay truth

The current scenario matrix classifies 36 rows. Eighteen valid authored
fixtures executed: 11 expected completable, 5 expected denial, and 2 expected
failure/recovery. The remaining 18 rows are explicit `INVALID_SETUP`; they are
not counted as executed successes.

Historical `tools/replay/pvs01_normal.txt`,
`pvs01_aggressive.txt`, `pvs01_stealth.txt`, and `pvs01_systemic.txt` remain
preserved. Their current classifications are `MIGRATE` for the first three
and `HISTORICAL_ONLY` for the earlier systemic demonstration. They are not
current mandatory Chapter One or Alpha-04 oracle inputs because their literal
timings/identity assumptions predate the current route contracts.

## Not verified, deferred, or still manual

- Foreground Windows Terminal manual acceptance, including host capability and
  visual presentation, remains `PENDING_MANUAL`. Non-foreground smoke/replay
  output honestly reports `win32-writeconsole`, `COMPATIBILITY`, and `ANSI16`
  because VT negotiation is unavailable through that pipe; this is not used as
  a visual acceptance claim.
- A first-time human 8–12 minute Chapter One play session was not fabricated
  from replay output.
- macOS, Linux ARM64 execution, and hosted cross-platform CI are not local
  executions; they remain pending the final normal push.
- Complete enemy combat AI, full social negotiation, future authored doors,
  final art/prose/audio, and Chapter Two remain outside this bounded receipt.
- `DEFER_ASTRA` is zero in the 02B ledger. Subjective future art/prose/audio
  work is not being used to hide a correctness finding.

## Final engineering status after documentation receipt push and remote CI

```text
LOCAL_REGRESSION = PASS
AUDIT02B_RECONCILIATION = COMPLETE
OPEN_FATAL = 0
OPEN_P0 = 0
OPEN_CURRENT_P1 = 0
ORIGIN_MAIN = 5711e68cf15343c016913b8391092d3dfb1248d4
REMOTE_CI = PASS (GitHub Actions run 34390875567; exact final pushed HEAD 5711e68cf15343c016913b8391092d3dfb1248d4; all five jobs)
FRONTEND_TERMINAL_ACCEPTANCE = PENDING_MANUAL
USER_ACCEPTANCE = PENDING
FINAL_STATUS = READY_FOR_RAIN_GAMEPLAY_SYSTEM_REVIEW
```

The final remote run covered the pushed documentation receipt head and passed
the Windows Release replay/package/benchmark path plus Linux GCC, Linux Clang,
macOS ARM64, and Linux ARM64 link jobs. Foreground Windows Terminal visual
acceptance and first-time human play remain manual boundaries; this report
does not claim `CHAPTER01_GOLD`, `PRODUCT_GOLD`, `VISUAL_GOLD`, or
`READY_FOR_RELEASE`.
