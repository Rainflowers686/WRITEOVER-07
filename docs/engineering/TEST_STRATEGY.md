# Validation tiers

The default gate protects the playable product. Broader route diagnosis remains
available; it is not deleted to shorten CI. Neither tier establishes human visual,
audio, first-time usability or measured playtime acceptance.

## FAST_REQUIRED

Build with `cmake --preset release` and `cmake --build --preset release`, then run:

```powershell
pwsh -NoProfile -File scripts/run_qa.ps1 -Tier FAST_REQUIRED
```

This Windows gate runs CTest once (the unit executable plus standalone public
headers), deterministic content and negative compiler cases, schema and negative
schema cases, bilingual source/catalog checks, release metadata tests/dry-run,
invalid-seed startup and isolated English/Chinese startup smoke. It then executes:

- DISCLOSE and discovery-poor full campaign routes, each with completed-save reload;
- the existing AMEND product flow, covering menus, Case File, save roles, New Game
  preservation and all eight transactional load fault stages;
- confirmed rebinding and real production facing/held-weapon fixtures;
- a fresh clean package, its smoke and missing-resource negative probes;
- the unchanged benchmark workload and static/public-contract/dependency checks.

The product flow already traverses a third full route. A separate AMEND replay is
not added to this tier. Campaign unit tests protect ending eligibility as well as
the production route receipts. CTest includes focused atomic-replace, corrupt-save,
wide-cell, resize, language, settings, records and onboarding regressions.

## EXTENDED

```powershell
pwsh -NoProfile -File scripts/run_qa.ps1 -Tier EXTENDED
```

This retains the 21 recovery cases, the 36-case Chapter One classification matrix
(including its intentionally invalid case), three Act II route cases and all four
full campaign routes with completed-save reloads. Existing script assertions and
fixtures remain authoritative. Unit/content/package checks are not repeated here.
Long determinism and additional benchmark experiments remain separate diagnostics,
not a claim implied by EXTENDED passing.

The full matrix should be run once for a release candidate, or when route/state
changes warrant it. A failing case may be rerun after a concrete fix; do not keep
running an unchanged suite until it happens to turn green.

## Platform split and duplication removed

Windows builds Debug and Release, then runs FAST_REQUIRED against Release.
Linux GCC and macOS arm64 run native Debug CTest/smoke and a Release benchmark.
Clang runs compilation, CTest and smoke. ARM64 Linux checks cross-link and ELF
architecture only: it is not a hardware execution test. Full campaign replay is
not duplicated on every platform.

The former direct unit invocation followed by CTest was redundant; platform
package wrappers now use CTest alone. Windows no longer repeats the same full
unit suite in both configurations on each push. Linux/macOS Release CI builds
only the benchmark target after native Debug tests. Benchmark resolution,
Character-Art actors, samples, thresholds and metric names are unchanged.

## Evidence and interpretation

`run_qa.ps1` requires a fresh directory below `docs/production/evidence`, preserves
per-step output, and writes `qa-result.json` with HEAD, executable, configuration,
exit codes, required receipts and elapsed seconds. Existing evidence is never
overwritten. A local result before commit is worktree evidence, not exact-head CI.
CI artifacts retain logs and JSON; final publication must identify its exact tag,
binary manifest, archive hash and CI head independently.

`check_localization.py` is a bounded literal/source inventory, not a C++ parser or
proof of every dynamic text combination. Render exports prove production glyph
selection/layout, not Windows Terminal font behavior. Benchmark
`worst_1pct_avg_ms` is the average of the slowest one percent, **not p99** and not
a measurement of a human player's displayed FPS.

For ordinary changes, run the affected unit group or focused replay first. Run the
whole FAST_REQUIRED tier when implementation stabilizes. In GitHub's CI manual
dispatch, enable `extended` only when the broad matrix is needed.
