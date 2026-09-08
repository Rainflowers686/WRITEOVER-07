# WRITEOVER-07 VIEWMODEL-CORRECTION-02

This report records the narrow close-range character projection and first-person
viewmodel correction at baseline `29fa5a370c050753591d326cf4f1863ab513f918`.
It is an engineering and visual-evidence receipt, not a Gold or release
acceptance claim.

## Scope

Implemented only the two requested blockers:

- Full Human MID/NEAR authored lower-body readability.
- Pistol composition as a held, lower-right first-person viewmodel.

The accepted face direction, Security art, environment art, Near cap `44`,
maps, story, gameplay systems, weapon logic, HUD top-left layout, renderer
architecture, tags, releases, and Steam workflow were not expanded or
redesigned.

The one renderer-side presentation adjustment is deliberately local to
Full-Human MID/NEAR foreground shading. It raises a small minimum foreground
luminance for that authored silhouette in close range; it does not brighten the
environment and is not a general outline system.

## Changes

Implementation checkpoint: `486d915`

Changed files in that checkpoint:

- `data/characters/b1_character_art.txt`
- `src/render/character_renderer.cpp`
- `tests/test_render.cpp`

Report and current visual evidence are added separately after validation.
Pre-existing dirty reports, settings files, and older evidence were preserved
and were not staged for this task.

### Full Human

- MID is now 17 authored rows, within the requested 14--18 range.
- NEAR is now 26 authored rows, within the requested 22--28 range.
- The face rows were retained: larger expressive eyes, existing hair direction,
  and no nostrils.
- From the neck down, the new contour includes shoulders, chest/waist, hips,
  separated thighs, knee transitions, shins, and separate feet.
- The central negative space between the legs is structural rather than a few
  isolated accent glyphs.
- The existing Near cap remains `44`.
- The local Full-Human foreground floor is applied only to MID/NEAR authored
  Full-Human foreground cells at close range.

The measured production projection after the correction remains stable at the
near distances: `0.40m`, `0.50m`, `0.75m`, `1.00m`, `1.50m`, and `2.00m` each
project to 44 total rows with 39 visible rows in the 67-row reference frame;
no close-range disappearance, NaN, or below-viewport flip was observed. At
`3.00m` the measured result is 35 total rows, and the existing LOD boundary
around `3.90m/4.00m/4.10m` is 27/26/25 rows.

### Pistol

- The four existing authored frames remain the same weapon states: idle A,
  idle B, fire, and reload.
- The pistol art now uses a 28-row diagonal 3/4 composition (reload is also
  28 rows) with a short inward muzzle/slide, a visible hand marker, and a
  lower-right forearm/hand mass rather than a complete horizontal card.
- The art remains within the existing bounded parser limits; no weapon type or
  weapon logic was added.
- `DrawPistolViewmodel` now uses a local grip/hand anchor. The anchor places the
  hand near normalized screen `(0.82, 0.92)` and derives the art origin from
  that anchor, instead of pinning the complete bounding box to the bottom-right
  safe rectangle.
- The existing fire/reload path remains intact. The real fire capture changed
  the displayed ammo from `12/48` to `11/48`; the reload capture returned to
  `12/48`.
- The runtime test measured the authored idle pose bounds as
  `x=153..204, y=39..66` on a 240x67 frame. This leaves the central aiming
  region readable while keeping the weapon in the requested lower-right/inward
  region.

This correction required both authored art and one narrow placement-code
change. It did not require a new renderer, a generic viewmodel framework, or a
HUD rewrite. The HUD remains compact and separate from the viewmodel.

### Unchanged areas

- Security art: `NO_CHANGE_NEEDED`.
- Environment palette and floor/ceiling direction: unchanged.
- Gameplay, combat semantics, systemic/save foundation, maps, story, and
  weapon logic: unchanged.
- Half-block pixel-world path: not restored or used for this correction.

## Local validation

All commands below were run against the current implementation checkpoint and
the rebuilt Windows binaries. A failed `ctest --preset release` invocation was
not relabeled as a test failure: this repository exposes no `release` CTest
preset, so Release CTest was run with the actual generated directory.

| Gate | Result | Evidence |
|---|---|---|
| Debug configure | PASS | `cmake --preset debug` |
| Debug build | PASS | `cmake --build --preset debug --config Debug` |
| Release configure | PASS | `cmake --preset release` |
| Release build | PASS | `cmake --build --preset release --config Release` |
| Debug CTest | PASS | 1/1 test passed |
| Release CTest | PASS | `ctest --test-dir out/build/release -C Release --output-on-failure`, 1/1 passed |
| Debug direct unit | PASS | rebuilt executable completed with 178 tests, 0 failed |
| Release direct unit | PASS | rebuilt executable completed with 178 tests, 0 failed |
| Content compiler check | PASS | deterministic recompile matched all generated content |
| Content compiler tests | PASS | 7/7 |
| Systemic schema check | PASS | 1 seed file validated |
| Systemic schema tests | PASS | 10/10 |
| Static audit | PASS | `COUNT=0` |
| Contract check | PASS | forbidden/dependency/public-header checks all OK |
| Debug smoke wrapper | PASS | `scripts/smoke.ps1 -Preset debug`, mapc + smoke OK |
| Release direct smoke | PASS | exit 0; direct console reports its expected legacy-conhost compatibility backend |
| B1 success replay | PASS | expected state and chapter checkpoint reached; non-lethal/body/search/hide/discovery/terminal/open/cross/save/load all YES |
| B1 gate-denied replay | PASS | expected state reached; `ACCESS_ATTEMPTED=YES`, `ACCESS_DENIED=YES`, `GATE_OPEN=NO`, `GATE_CROSSED=NO` |
| B1 terminal-denied replay | PASS | expected state reached; `TERMINAL_ATTEMPTED=YES`, `TERMINAL_DENIED=YES`, no session |
| B1 badge-only replay | PASS | expected state reached; badge held, `ACCESS_ATTEMPTED=NO` |
| B1 health/death replay | PASS | expected state reached; save/load, death, and recovery all exercised |
| Release benchmark | PASS | `PVS_RENDER_TIME_MS=0.912`, `PVS_TOTAL_FRAME_TIME_MS=1.330`, `OVERALL_BUDGET=PASS` |

The benchmark's emitted field is `worst_1pct_avg_ms`; this report does not
rename it to p99 and does not claim end-to-end 120 Hz terminal presentation.
The integrated frame measurement also states that platform writes are
excluded.

## Real visual evidence

The following files were captured from the Windows Release executable launched
in a dedicated, maximized Windows Terminal window. Still captures use the
Windows Terminal client rectangle and contain no desktop taskbar or unrelated
notification. The motion file is a real 10.000-second H.264 capture of the
Terminal client rectangle at 2560x1392; it contains walking, strafe, turns,
approach/back-away movement, and one real fire click.

- `evidence/viewmodel_correction02/H01_FULL_HUMAN_3M.png`
- `evidence/viewmodel_correction02/H02_FULL_HUMAN_2M.png`
- `evidence/viewmodel_correction02/H03_FULL_HUMAN_1M.png`
- `evidence/viewmodel_correction02/W01_PISTOL_DEFAULT.png`
- `evidence/viewmodel_correction02/W02_PISTOL_FIRE.png`
- `evidence/viewmodel_correction02/W03_PISTOL_RELOAD.png`
- `evidence/viewmodel_correction02/MOTION_VIEWMODEL_10S.mp4`

These are runtime captures, not SVG reconstruction, frame-dump conversion,
mockups, or offline renderer output. Extracted motion frames were used only for
inspection and are not part of the project evidence directory.

## Human visual questions

1. At 1m, are both Full Human legs immediately distinguishable? **Yes for the
   current authored character-art readability target.** The H03 capture shows
   separate left/right leg contours, a central gap, knees/shins, and feet.
2. Does the lower body remain readable in grayscale? **Structurally yes:** the
   separation is carried by contour and negative space, with the local
   Full-Human foreground floor reducing the previous merge into the wall. A
   human stylistic acceptance of the dark palette remains pending.
3. Does the pistol clearly originate from the player? **Yes at the composition
   level:** the hand/grip anchor is lower-right and the forearm continues to
   the bottom edge.
4. Does the pistol point toward the center of the scene? **Yes:** the muzzle
   and slide axis run inward/up-left from the lower-right hand position.
5. Is a hand/forearm visually connected to it? **Yes, in the intentionally
   stylized authored character language.**
6. Does it still look like an icon or UI card? **No in the checked static pose:**
   it is no longer a complete horizontal lower-right card; it is a partial
   diagonal held pose. Rain's final visual judgment remains pending.
7. Does the weapon occupy too much screen? **No in the measured reference
   frame:** the non-space bounds are `x=153..204, y=39..66` and the center is
   unobstructed.
8. Does it obscure the center aiming region? **No.**

## Three red teams

### Code red team

PASS for scope. The implementation checkpoint contains only the requested
character asset, local Full-Human shading/placement change, and focused render
tests. No Security asset, environment, HUD top-left, gameplay, system,
renderer architecture, map, story, branch, tag, release, or Steam change was
introduced. The pre-existing dirty files remain outside the checkpoint.

### Effect red team

PASS for this narrow engineering target. Compared with the previous evidence,
Full Human now has authored mid/near lower-body structure instead of a short
source pattern being replicated into a chunky, hollow silhouette. The pistol's
measured and captured pose now enters from the lower-right, points inward, and
keeps the center clear. Fire and reload captures preserve the same held
composition.

### Perfection red team

The remaining risks are deliberately recorded rather than hidden:

- The Full Human palette is still dark and stylized; some environments may
  make the lower body feel subdued even though the contour is readable.
- The pistol is intentionally sparse character art. A viewer may still ask for
  more authored hand detail, but that would be an art-direction decision beyond
  this projection/placement correction.
- Terminal font metrics and color perception remain part of Rain's real visual
  acceptance; automated bounds and captures cannot replace that judgment.

These are review items, not reasons to expand this task into a renderer or
gameplay redesign.

## Final engineering verdict

CURRENT_BASELINE = `29fa5a370c050753591d326cf4f1863ab513f918`

IMPLEMENTATION_COMMIT = `486d915`

CURRENT_HEAD = `recorded in the final handoff after the report/evidence receipt commit`

ORIGIN_MAIN = `unchanged until the focused main push`

WORKTREE = `DIRTY_PREEXISTING_FILES_PRESERVED`

FULL_HUMAN_LOWER_BODY = `PASS_FOR_SCOPE; RAIN_REVIEW_PENDING`

PISTOL_STATIC_POSE = `PASS_FOR_SCOPE; RAIN_REVIEW_PENDING`

PISTOL_HAND_CONNECTION = `PASS_FOR_SCOPE; RAIN_REVIEW_PENDING`

PISTOL_CENTER_DIRECTION = `PASS`

HUD = `PASS_UNCHANGED`

ENVIRONMENT = `PASS_UNCHANGED`

GAMEPLAY_REGRESSION = `PASS`

OPEN_FATAL = `0`

OPEN_MAJOR = `0`

OPEN_P0 = `0`

OPEN_P1 = `0`

NO_GOLD_CLAIM = `YES`

NO_RELEASE_CREATED = `YES`

FINAL_STATUS = `READY_FOR_RAIN_VIEWMODEL_REVIEW`

Rain's visual acceptance is still pending. This checkpoint is ready for Rain
to inspect the H01--H03 and W01--W03 runtime captures and decide whether the
Full Human lower body and the pistol now meet the intended final visual bar.
