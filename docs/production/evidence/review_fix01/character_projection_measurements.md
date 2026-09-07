# REVIEW-FIX-01 Character Sprite Projection Measurement

This is a measurement of the existing Character Renderer. No art asset,
projection formula, or screen-space cap was changed by REVIEW-FIX-01.

## Method

- Reference terminal grid: `240 x 67` cells.
- Camera: level view, `60` degree vertical FOV, `kEyeStand = 1.60m`.
- Test actor: existing `FullHuman` sprite path, authored world height `1.80m`.
- Distances: `0.5m`, `1m`, `2m`, `3m`, `4m`.
- `projected_rows` is the renderer's unclipped destination height after the
  current LOD cap; `visible_rows` is the actual non-space row span emitted into
  the 67-row CharCell frame. At 0.5m the projected footprint is below the
  viewport in this floor-level camera setup, so visible rows can be zero even
  though the projected footprint is capped.

## Recorded result

| Distance | LOD | Raw projected rows | Current cap | Projected rows | Visible rows |
|---:|---|---:|---:|---:|---:|
| 0.5m | Near | 208.89 | 96 | 96 | 0 (off-viewport) |
| 1.0m | Near | 104.44 | 96 | 96 | 37 (rows 30–66) |
| 2.0m | Near | 52.22 | 96 | 52 | 40 (rows 27–66) |
| 3.0m | Near | 34.81 | 96 | 35 | 37 (rows 29–65) |
| 4.0m | Mid | 26.11 | 48 | 26 | 28 (rows 30–57) |

The current `Near=96` cap is larger than the complete 67-row terminal viewport
and is therefore a genuine close-range scaling/clipping risk. Provisional
review recommendation only: consider a future `Near` world-sprite cap of
`48 cells` (with Rain deciding after real screenshots). REVIEW-FIX-01 does not
change that cap or redesign the art.
