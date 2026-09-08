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

The foreground lock-screen blocker was cleared before this capture pass. The
eight stills below are full-desktop PNG captures made while a maximized
Windows Terminal window was the foreground window and the Release process was
present in that terminal. The terminal window rectangle was
`-8,-8-2568,1400` on a `2560x1440` desktop. The visible title bar includes the
capture label and the real executable path; no SVG, frame-dump reconstruction,
mockup, or offline renderer was used.

```text
CAPTURE_SOURCE = D:\AAAbiancheng\00_Projects\40_Coursework\2026_CPP_Immersive_ASCII_FPS\out\build\release\Release\writeover_app.exe
CAPTURE_SURFACE = foreground maximized Windows Terminal desktop
CAPTURE_DESKTOP = 2560x1440
CAPTURE_TERMINAL_RECT = -8,-8-2568,1400
CAPTURE_ROOM = room_b1_revival

V01 = CAPTURED / real Release executable + Windows Terminal / camera 2.5,15.5,0
V02 = CAPTURED / real Release executable + Windows Terminal / security ~3m / camera 5.5,5.5,0
V03 = CAPTURED / real Release executable + Windows Terminal / security ~2m / camera 6.5,5.5,0
V04 = CAPTURED / real Release executable + Windows Terminal / security ~1m / camera 7.5,5.5,0
V05 = CAPTURED / real Release executable + Windows Terminal / Full-Human ~3m / camera 3.5,10.5,0
V06 = CAPTURED / real Release executable + Windows Terminal / Full-Human ~2m / camera 4.5,10.5,0
V07 = CAPTURED / real Release executable + Windows Terminal / Full-Human ~1m / camera 5.5,10.5,0
V08 = CAPTURED / real Release executable + Windows Terminal / pistol default / camera 2.5,15.5,0
```

The corresponding files are:

- `evidence/visual_correction01/V01_B1_DEFAULT.png`
- `evidence/visual_correction01/V02_SECURITY_3M.png`
- `evidence/visual_correction01/V03_SECURITY_2M.png`
- `evidence/visual_correction01/V04_SECURITY_1M.png`
- `evidence/visual_correction01/V05_FULL_HUMAN_3M.png`
- `evidence/visual_correction01/V06_FULL_HUMAN_2M.png`
- `evidence/visual_correction01/V07_FULL_HUMAN_1M.png`
- `evidence/visual_correction01/V08_PISTOL_DEFAULT.png`

Each still was visually inspected after capture. The security and Full-Human
series remain visible as distance decreases; the near captures do not show a
cap-anchor disappearance. The stills are evidence for Rain's art decision,
not an automated product-art acceptance.

The motion evidence uses the same Release executable without a camera
override. The player was navigated from the authored B1 spawn to approximately
four metres from `guard_7` before recording. During the 15-second capture the
real input sequence was W approach, A/D strafe, mouse look left/right, and S
retreat. Inspection frames from the video show substantive world/camera
changes across the sequence, including the close NPC pass; the clip is an
engineering motion capture, not a claim of final 60–90 second playthrough
acceptance.

```text
REAL_MOTION_CAPTURE = CAPTURED / 15.000s / H.264 / 2560x1440 / 30fps
MOTION_FILE = evidence/visual_correction01/MOTION_B1_4M_COLLISION_STRAFE.mp4
MOTION_INITIAL_ROUTE = authored spawn -> approximately 4m from guard_7
MOTION_SEQUENCE = W approach; A/D strafe; mouse look left/right; S retreat
```

## Regression evidence

```text
GAMEPLAY_REGRESSION = PASS (local Debug and Release gates listed below)
CI = SUCCESS (GitHub Actions run 34169632322; main push; no release workflow)
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
FINAL_STATUS = READY_FOR_RAIN_ART_REVIEW
```

`READY_FOR_RAIN_ART_REVIEW` means the requested real-terminal visual evidence
is now available. It does not mean that the Full-Human, Security, Pistol, or
overall art acceptance has been granted; all four remain pending Rain's review
of the actual captures.
