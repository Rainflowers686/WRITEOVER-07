# WRITEOVER-07 VISUAL-CORRECTION-01

This checkpoint addresses only close-range Character Renderer projection,
minimal player/standing-NPC separation, and wall-pattern world anchoring. It
does not redesign the authored faces, security art, pistol art, maps, story,
weapons, or the renderer paradigm. No release, tag, branch, PR, or Steam
operation was performed.

## Projection and collision result

```text
PROJECTION_MODEL_FIXED = YES
CLOSE_RANGE_DISAPPEARANCE = FIXED; 0.40m through 12.00m all emitted non-space glyphs
SELECTED_NEAR_CAP = 44 rows (working default; Rain art decision pending)
NPC_DYNAMIC_COLLISION = YES; app-local query decorator over the existing IWorldQuery
MINIMUM_PLAYER_NPC_DISTANCE = 0.772m center distance in the standing-NPC test
LOD_POP = PASS; 3.9m/4.0m/4.1m projected rows = 27/26/25
WALL_PATTERN_WORLD_ANCHORED = YES
WALL_SWIMMING_CHECK = PASS in deterministic world-coordinate unit probe
```

The sprite path now computes `raw_scale`, a cap-derived `max_scale`, and one
`effective_scale = min(raw_scale, max_scale)`. Projected height, foot/ground
row, top row, and vertical placement derive from that same effective scale.
Invalid/non-finite sprite center or scale data is rejected before projection.

The player query still delegates static collision to the existing world query.
It additionally rejects overlap with conscious standing runtime NPCs in the
active room, while Stunned/Dead actors remain traversable floor bodies. The
controller test approaches a standing actor, stops at `0.772m`, confirms the
candidate overlap is blocked, strafes around it, and confirms the non-standing
case is not treated as a wall.

The wall resolver now derives its surface coordinate from the ray hit in world
space and its vertical coordinate from the projected wall segment. A fixed
screen-cell probe changes from metal seam `U+2551 (║)` to panel space when the
camera translates along the wall, which is the expected world-anchor result.

Detailed distance/cap data is in
[`character_projection_measurements.md`](evidence/visual_correction01/character_projection_measurements.md).

## Required real-terminal evidence

The Release executable was launched through Windows Terminal for the capture
attempt. The workstation's foreground window was the system lock screen
(`Windows 默认锁屏界面`); the Windows Terminal process was present in the
background but could not be brought to the foreground from this task. No lock
screen image was retained as evidence and no backend/frame-dump conversion was
substituted.

```text
V01 = NOT_RUN / BLOCKED_LOCK_SCREEN
V02 = NOT_RUN / BLOCKED_LOCK_SCREEN
V03 = NOT_RUN / BLOCKED_LOCK_SCREEN
V04 = NOT_RUN / BLOCKED_LOCK_SCREEN
V05 = NOT_RUN / BLOCKED_LOCK_SCREEN
V06 = NOT_RUN / BLOCKED_LOCK_SCREEN
V07 = NOT_RUN / BLOCKED_LOCK_SCREEN
V08 = NOT_RUN / BLOCKED_LOCK_SCREEN
REAL_MOTION_CAPTURE = NOT_RUN / BLOCKED_LOCK_SCREEN
```

The `docs/production/evidence/visual_correction01/` directory intentionally
contains measurement text only until a real, foreground Windows Terminal
session can be captured. The old Character-Art evidence remains historical and
was not relabeled as evidence for this correction.

## Regression evidence

```text
GAMEPLAY_REGRESSION = PASS (local Debug and Release gates listed below)
CI = PENDING_PUSH
```

Executed locally against the canonical root:

- Debug configure/build: PASS.
- Debug CTest: PASS, 1/1 target.
- Debug smoke: PASS, exit `0`.
- Release configure/build: PASS.
- Release CTest: PASS, 1/1 target.
- Direct Release unit executable: PASS, `176 tests, 0 failed`.
- Content compiler check: PASS, deterministic recompile matches.
- Content compiler tests: PASS, `7/7`.
- Systemic schema check: PASS.
- Systemic schema tests: PASS, `10/10`.
- Static audit: PASS, `COUNT=0`.
- Contract check: PASS.
- Direct Release smoke: PASS, exit `0`.
- Release benchmark: PASS; `character_render_workload_240x67`
  `worst_1pct_avg_ms=0.957`, and `character_total_runtime_frame_240x67`
  `worst_1pct_avg_ms=1.424` with platform writes excluded.
- B1 success replay: PASS, expected state reached and chapter checkpoint
  reached.
- B1 gate-denied replay: PASS, access attempted/denied and gate remained
  closed.
- B1 terminal-denied replay: PASS, terminal attempted/denied and no session.
- B1 badge-only replay: PASS, badge held and `ACCESS_ATTEMPTED=NO`.

The benchmark's field name remains `worst_1pct_avg_ms`; it is not a strict
p99 quantile and does not prove end-to-end terminal presentation at 120Hz.

## Acceptance boundary

```text
FULL_HUMAN_ART_ACCEPTANCE = PENDING
SECURITY_ART_ACCEPTANCE = PENDING
PISTOL_ART_ACCEPTANCE = PENDING
OVERALL_VISUAL_ACCEPTANCE = PENDING
FINAL_STATUS = NOT_READY
```

`NOT_READY` is caused only by the missing real foreground V01–V08 and motion
evidence, not by a failed local projection, wall-anchor, collision, or
engineering regression gate. Rain must review actual terminal captures before
making the art decision.
