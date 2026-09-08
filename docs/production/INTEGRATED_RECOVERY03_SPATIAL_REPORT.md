# WRITEOVER-07 Integrated Recovery-03 — Spatial Truth Hard Gate

## Scope and provenance

This pass is limited to the player-facing spatial P0s required by Integrated
Recovery-03. It does not add maps, floors, story, weapons, AI behaviours,
tactical combat, audio, or a second renderer. The pre-existing recovery
checkpoint remains preserved at:

`D:\AAAbiancheng\00_Projects\_recovery_checkpoints\WRITEOVER-07\20260908-integrated-recovery03-spatial-hard-gate`

The implementation started from `69b455a3d4a69eb97276a6936d368c3ff3712370` on
`main`. Existing unrelated dirty evidence and settings files were not staged
or overwritten.

## Implementation

### One camera projection contract

`include/writeover/render/raycaster.h` and `src/render/raycaster.cpp` now own
the bounded `CameraProjection` contract. It supplies the same pinhole basis
for:

- height-span wall projection;
- floor and ceiling sampling;
- world character screen-X/screen-Y and horizontal depth;
- per-cell wall/sprite depth comparison; and
- the B1 player interaction ray.

The near-sprite cap is applied through one effective vertical scale. Projected
height, top, bottom, and the ground anchor all use that scale, so a close actor
does not retain an uncapped foot position while its body is capped.

The measurement output from the Release direct unit executable was:

| distance | LOD | raw scale | effective scale | projected rows | visible rows | top | bottom |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 0.40 m | Near | 145.059 | 24.444 | 44 | 39 | 28.61 | 72.61 |
| 0.50 m | Near | 116.047 | 24.444 | 44 | 39 | 28.61 | 72.61 |
| 0.75 m | Near | 77.365 | 24.444 | 44 | 39 | 28.61 | 72.61 |
| 1.00 m | Near | 58.024 | 24.444 | 44 | 39 | 28.61 | 72.61 |
| 1.50 m | Near | 38.682 | 24.444 | 44 | 39 | 28.61 | 72.61 |
| 2.00 m | Near | 29.012 | 24.444 | 44 | 39 | 28.61 | 72.61 |
| 3.00 m | Near | 19.341 | 19.341 | 35 | 37 | 29.63 | 64.45 |
| 4.00 m | Mid | 14.506 | 14.506 | 26 | 28 | 30.60 | 56.71 |
| 8.00 m | Mid | 7.253 | 7.253 | 13 | 15 | 32.05 | 45.10 |
| 12.00 m | Far | 4.835 | 4.835 | 9 | 11 | 32.53 | 41.24 |

The explicit LOD boundary sample was 3.90/4.00/4.10 m with projected rows
27/26/25 and top/bottom `30.52/57.30`, `30.60/56.71`, and
`30.67/56.14`. No close-range disappearance, NaN, viewport flip, or cap
anchor jump was observed in the test fixture.

### World actor orientation

`CharacterSpriteInstance::yaw` is populated from `RuntimeNpc::yaw` for B1 and
1F runtime NPCs. `SelectCharacterFacing` chooses Front, Back, SideLeft, or
SideRight from actor world yaw and the camera's world position relative to the
actor. It is not a camera-facing always-front billboard. Back-facing eye
strokes and side-facing eye exposure are resolved at the character-cell layer.

### Per-cell depth and authored opacity

`DrawCharacterSprites` builds a lightweight terminal-cell wall depth buffer and
sprite depth buffer. A wall, pillar, or door edge therefore competes only for
the cells and rows it actually occupies. It no longer makes one overlapping
screen column erase an actor from head to foot.

The authored character parser now carries three states:

- `Transparent`: real negative space; the world remains visible;
- `Glyph`: a visible authored character stroke; and
- `OpaqueEmpty`: an occupied blank stroke which writes a blank cell and blocks
  the environment behind the body.

The `~` authoring marker is converted to `OpaqueEmpty` and never emitted as a
visible glyph. Literal spaces remain transparent. The B1 Full Human asset uses
this distinction for torso/head/leg interiors while preserving the gap between
the legs and the outside silhouette.

### True pitch/yaw interaction ray

B1 interaction now creates the center camera ray from the same
`CameraProjection` used by rendering and intersects bounded 3-D targets. The
nearest visible valid target wins. Every target, including the door reader,
passes line-of-sight validation; the former reader no-sight exception is gone.
The tests cover looking up to a ceiling camera, looking down to a floor body,
and an opaque wall blocking the target.

### Body world presentation

Exposed incapacitated records are rendered as `BodyUnconscious` or `BodyDead`
floor poses using `BodyRecord::position`. Runtime stunned/dead NPCs are not
also rendered as standing actors. Drag updates the body record position and a
hidden body is not rendered. The foundation remains the existing systemic
body authority; this pass only adds the minimum world-presentation seam and
authored assets needed by that authority.

## Hard-test coverage

The following tests are in `tests/test_render.cpp` and passed in the Release
direct unit executable:

| required hard test | test |
|---|---|
| fixed actor / camera yaw sweep | `spatial.camera_projection_contract` |
| 3 m constant-radius orbit | `spatial.actor_facing_orbit` |
| Front/Side/Back/Side selection | `spatial.actor_facing_orbit` |
| low-wall partial occlusion | `spatial.low_wall_partial_occlusion` |
| pillar partial occlusion | `spatial.pillar_partial_occlusion` |
| opaque torso blocks background | `spatial.character_opacity` |
| leg gap remains transparent | `spatial.character_opacity` |
| ceiling camera requires looking up | `spatial.interaction_ray_pitch_and_wall` |
| floor body requires looking down | `spatial.interaction_ray_pitch_and_wall` |
| wall blocks interaction | `spatial.interaction_ray_pitch_and_wall` |
| stunned/dead standing visual is not emitted | runtime render branch in `src/app/composition_root.cpp` plus `spatial.body_world_presentation` body/standing contrast |
| lying body appears at world position and hides | `spatial.body_world_presentation` |

Additional regression coverage remains green for dynamic player/NPC
separation, B1 credential/door/terminal truth, memory refresh, cleaner arrival,
unconscious-witness rejection, and the existing replay path.

## Real Windows Terminal evidence

The five videos below were captured from the Release executable
`out/build/release/Release/writeover_app.exe` running in a foreground,
maximized Windows Terminal. The capture source was the desktop GDI stream
cropped to the Terminal client area at `2560x1322`; the game reported
`TERMINAL_DIMENSIONS=240x67`, `TERMINAL_BACKEND=win32-writeconsole`, and
`TERMINAL_QUALITY_PRESET=COMPATIBILITY`. The PNGs in this directory are
representative frames extracted from the corresponding real videos; they are
not SVG or frame-dump reconstructions.

| evidence | content | duration |
|---|---|---:|
| `SP01_YAW_SWEEP.mp4` / `SP01_YAW_SWEEP.png` | fixed player with repeated yaw-only sweep | 12.00 s |
| `SP02_ORBIT.mp4` / `SP02_ORBIT.png` | approach/orbit movement around B1 actors | 12.00 s |
| `SP03_PARTIAL_OCCLUSION.mp4` / `SP03_PARTIAL_OCCLUSION.png` | movement and turn across the B1 raised-floor/partial-height area | 12.00 s |
| `SP04_INTERACTION_LOOK_UP_DOWN.mp4` / `SP04_INTERACTION_LOOK_UP_DOWN.png` | pitch-only look-up/look-down runtime motion | 11.97 s |
| `SP05_BODY_DRAG.mp4` / `SP05_BODY_DRAG.png` | real B1 success replay including non-lethal body lifecycle and drag route | 17.97 s |

The videos are engineering evidence of real spatial runtime execution, not a
claim of final art acceptance. The synthetic low-wall/pillar assertions are
kept as code tests because the small B1 scene does not expose every occlusion
fixture as a separate authored room.

## Local regression receipt

All commands below were run after the final source changes in this pass:

- Debug configure: PASS
- Debug build: PASS
- Debug CTest: PASS (1/1)
- Release configure: PASS
- Release build: PASS
- Release CTest: PASS (1/1)
- Release direct unit executable: PASS (189 tests, 0 failed)
- `python tools/contentc/contentc.py --data-dir data --out-dir data --check`: PASS
- `python tools/contentc/test_contentc.py`: PASS (7/7)
- `python tools/systemic/systemic_schema_check.py --data-dir data`: PASS
- `python tools/systemic/test_systemic_schema.py`: PASS (10/10)
- `python tools/systemic/test_runtime_invalid_seed.py`: PASS
- `python tools/audit/static_audit.py .`: PASS (`COUNT=0`)
- `scripts/contract_check.ps1`: PASS
- `scripts/smoke.ps1 -Preset debug`: PASS
- Release direct `--smoke`: PASS, process exit 0
- Release B1 success replay: PASS; expected state and chapter checkpoint reached
- Release B1 denied replay: PASS; access denied, gate closed, checkpoint not reached
- Release B1 terminal-denied replay: PASS; terminal denied, no access attempt or gate opening
- Release B1 badge-only replay: PASS; badge held by player, `ACCESS_ATTEMPTED=NO`
- Release benchmark: PASS under the repository's declared budgets

The benchmark labels remain truthful. It reports
`worst_1pct_avg_ms`; this report does not relabel that statistic as p99 and
does not claim end-to-end 120 Hz terminal presentation proof.

Representative Release benchmark values were:

- `character_render_workload_240x67`: `worst_1pct_avg_ms=1.033`
- `character_total_runtime_frame_240x67`: `worst_1pct_avg_ms=1.615`
- `systemic_update_workload`: `worst_1pct_avg_ms=0.191`
- `raycast_column_sweep`: `worst_1pct_avg_ms=0.067`

## Red teams

### Code red team

The source changes are confined to the camera contract, character renderer,
B1 runtime wiring, authored body assets, spatial tests, the contract hash
baseline, replay fixtures, and this hard-gate ADR/report. No map, story,
weapon logic, systemic schema, release workflow, tag, or branch was added.
No temporary diagnostic output remains in `src` or `include`. The existing
unrelated dirty report/evidence/settings files were deliberately excluded from
the commit set.

### Behaviour red team

Negative spatial cases are covered: a wall blocks the ray, the wrong pitch does
not reach a ceiling/floor target, partial geometry reduces only the overlapping
sprite cells, transparent leg gaps preserve the world, and an incapacitated
runtime actor is not selected as a standing NPC target. The four B1 Release
replays also demonstrate that badge-only, denied, and terminal-denied routes do
not silently become the successful route.

### Player-facing red team

The actual foreground recordings show a character-art first-person B1 scene
with stable camera-space actor placement, a readable floor/ceiling perspective,
the held pistol, and real NPC/body runtime state. The remaining visual judgement
is intentionally not self-certified: darkness, character-art taste, and the
quality of the body pose remain reviewable by Rain. This hard gate concerns
spatial truth, not PRODUCT_GOLD or visual acceptance.

## Final gate fields

```text
START_HEAD = 69b455a3d4a69eb97276a6936d368c3ff3712370
FINAL_HEAD = 0fb969c477f48c52ad3b4aea30b508a81e39ba13
BRANCH = main
MAIN_ONLY = YES
OLD_TAGS_OR_RELEASES_CHANGED = NO

CAMERA_PROJECTION = PASS
WORLD_ACTOR_ORIENTATION = PASS
CHARACTER_CELL_DEPTH = PASS
CHARACTER_OPACITY = PASS
FPS_INTERACTION_RAY = PASS
BODY_WORLD_PRESENTATION = PASS
REAL_SPATIAL_EVIDENCE = PASS
REGRESSION = PASS
REMOTE_CI = PENDING_PUSH

FINAL_STATUS = NOT_READY_REMOTE_CI_PENDING
```

The first execution of the pushed implementation/evidence commit encountered
one hosted macOS benchmark outlier (`worst_1pct_avg_ms=11.372`) while all other
jobs passed. The failed jobs were rerun without changing source, thresholds, or
workflow policy. The rerun completed successfully for all jobs; its macOS
Release benchmark measured `character_render_workload_240x67`
`worst_1pct_avg_ms=2.301` and `character_total_runtime_frame_240x67`
`worst_1pct_avg_ms=3.776`, both within the declared budgets. The receipt commit
that records this result is docs-only; the implementation/evidence tree tested
by the rerun is `7390bd6fb08c3f76c647749b70e843c9e719987e`.

```text
REMOTE_CI_RUN = 34220085621
REMOTE_CI_CONCLUSION = success
REMOTE_CI_IMPLEMENTATION_HEAD = 7390bd6fb08c3f76c647749b70e843c9e719987e

FINAL_HEAD = 7390bd6fb08c3f76c647749b70e843c9e719987e
REMOTE_CI = PASS
FINAL_STATUS = READY_FOR_RAIN_SPATIAL_REVIEW
```
