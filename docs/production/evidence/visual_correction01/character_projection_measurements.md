# VISUAL-CORRECTION-01 Character Projection Measurements

These measurements run through the Release `CharacterArtBank` and
`DrawCharacterSprites` path. They are engineering evidence only; they are not
art acceptance.

## Model

- Reference frame: `240 x 67` CharCells.
- Vertical FOV: `60` degrees.
- Camera eye height: `1.60m`.
- Test actor height: `1.80m`.
- `raw_scale = focal / depth`.
- `max_scale = screen_space_cap / actor_height`.
- `effective_scale = min(raw_scale, max_scale)`.
- Ground, top, and projected height all use `effective_scale`.
- Selected caps: Near `44`, Mid `48`, Far `24` rows.

## Distance sweep

`top` and `bottom` are the continuous projected model bounds. `visible` is the
actual non-space glyph span written into the 67-row frame, after viewport
clipping and authored transparent rows.

| Distance | LOD | Raw scale | Effective scale | Projected rows | Visible rows | Model top..bottom | Visible bounds | Screen % |
|---:|---|---:|---:|---:|---:|---:|---|---:|
| 0.40m | Near | 145.059 | 24.444 | 44 | 39 | 28.61..72.61 | 28..66 | 65.7 |
| 0.50m | Near | 116.047 | 24.444 | 44 | 39 | 28.61..72.61 | 28..66 | 65.7 |
| 0.75m | Near | 77.365 | 24.444 | 44 | 39 | 28.61..72.61 | 28..66 | 65.7 |
| 1.00m | Near | 58.024 | 24.444 | 44 | 39 | 28.61..72.61 | 28..66 | 65.7 |
| 1.50m | Near | 38.682 | 24.444 | 44 | 39 | 28.61..72.61 | 28..66 | 65.7 |
| 2.00m | Near | 29.012 | 24.444 | 44 | 39 | 28.61..72.61 | 28..66 | 65.7 |
| 3.00m | Near | 19.341 | 19.341 | 35 | 37 | 29.63..64.45 | 29..65 | 52.0 |
| 4.00m | Mid | 14.506 | 14.506 | 26 | 28 | 30.60..56.71 | 30..57 | 39.0 |
| 8.00m | Mid | 7.253 | 7.253 | 13 | 15 | 32.05..45.10 | 32..46 | 19.5 |
| 12.00m | Far | 4.835 | 4.835 | 9 | 11 | 32.53..41.24 | 32..42 | 13.0 |

All ten distances produced finite values and nonzero visible authored glyphs.
No close-range actor disappeared or inverted its feet/top ordering. At the
default camera height, the cap-bounded near model extends below the bottom
viewport edge (`bottom=72.61`) while its visible authored glyphs remain a
bounded 39-row span; this is ordinary viewport clipping, not a full-frame
sprite or a cap-anchor jump. The Near cap binds from `0.40m` through `2.00m`;
at `3m` the raw projection is already below the cap.

## Cap study

At both `0.50m` and `1.00m`, the raw scale is above every candidate cap. The
corrected model therefore produces the following bounded comparison:

| Candidate cap | Effective scale | Projected rows | Projected screen percentage |
|---:|---:|---:|---:|
| 40 | 22.222 | 40 | 59.7 |
| 44 | 24.444 | 44 | 65.7 |
| 48 | 26.667 | 48 | 71.6 |
| 52 | 28.889 | 52 | 77.6 |

`44` is the conservative working default for the current Near art: it is
below the former 96-row cap and below the 48/52 alternatives, while retaining
more authored detail than 40. Rain's visual review remains the authority for
whether this default is aesthetically acceptable.

## LOD boundary

The measured projected rows at `3.9m / 4.0m / 4.1m` are `27 / 26 / 25`, with
model top `30.52 / 30.60 / 30.67` and bottom `57.30 / 56.71 / 56.14`.
The authored LOD changes at `4.0m`, but the geometric projection remains
continuous within one row per step.

## Wall anchoring

The fixed-screen-cell wall probe translated the camera along one metal wall
face from surface coordinate `2.96` to `3.18`. The central wall glyph changed
from `U+2551 (║)` to `U+0020 (space)`. A screen-coordinate pattern would have
kept that central cell identical; this is a deterministic world-coordinate
anchor check, not a claim of manual motion acceptance.

## Release test output markers

```text
CHARACTER_CAP_STUDY distance_m=0.50 cap=40 raw_scale=116.047 effective_scale=22.222 projected_rows=40 screen_pct=59.7
CHARACTER_CAP_STUDY distance_m=0.50 cap=44 raw_scale=116.047 effective_scale=24.444 projected_rows=44 screen_pct=65.7
CHARACTER_CAP_STUDY distance_m=0.50 cap=48 raw_scale=116.047 effective_scale=26.667 projected_rows=48 screen_pct=71.6
CHARACTER_CAP_STUDY distance_m=0.50 cap=52 raw_scale=116.047 effective_scale=28.889 projected_rows=52 screen_pct=77.6
CHARACTER_SPRITE_LOD_BOUNDARY d=3.9/4.0/4.1 rows=27/26/25 top=30.52/30.60/30.67 bottom=57.30/56.71/56.14
CHARACTER_WALL_ANCHOR surface_u=2.96/3.18 glyph=U+2551/U+0020
PLAYER_NPC_SEPARATION stop_x=2.228 actor_x=3.000 minimum_center_distance=0.772
176 tests, 0 failed
```
