# WRITEOVER-07 CHARACTER-ART POLISH-01

This receipt records the narrow visual polish slice completed against the
canonical repository. It is an engineering and evidence receipt, not a
replacement for Rain's visual acceptance.

## Scope and preserved state

```text
START_HEAD = 7e68166f4a14a659d77f72614961e4729a41dd11
IMPLEMENTATION_HEAD = 2c8dcc9e90f9c3172722073a04323f5fbe0d783d
BRANCH = main
REMOTE = https://github.com/Rainflowers686/WRITEOVER-07.git
OLD_VISUAL_CORRECTION_WORK = PRESERVED
OLD_EVIDENCE = PRESERVED; no prior evidence was overwritten or deleted
```

In scope:

- authored Full Human FAR/MID/NEAR character-art asset polish;
- authored Pistol idle, fire, and reload frame polish;
- real Release/Windows Terminal evidence for Full Human and Pistol default;
- local regression and deterministic replay verification;
- this report and a new evidence directory.

Out of scope:

- renderer, projection cap, ray geometry, materials, gameplay, systemic/save
  foundation, maps, floors, story, weapon systems, Security art, and HUD
  wiring;
- new renderer, half-block world rendering, new content scope, release/tag,
  Steam, branch, PR, or release publication.

The pre-existing modified `VISUAL_CORRECTION01_REPORT.md` and the prior
`visual_correction01` evidence remain deliberately unstaged. They belong to
the preserved prior worktree and were not folded into this task's commits.

## Changes

The only implementation file changed by this task is:

- `data/characters/b1_character_art.txt`

No C++ renderer, projection, gameplay, test, or public header was changed.
The Character Renderer continues to load the bounded authored asset bank and
the existing 44-row near projection cap remains unchanged.

### Full Human

The FAR/MID/NEAR entries were independently authored rather than mechanically
scaling one drawing. The revision narrows the previous board-like silhouette,
adds a readable hair/head contour and larger eyes, separates the neck,
shoulders, chest, waist, and legs with negative space, and keeps the mouth/nose
marks restrained. No nostril glyph is present. The final Release captures show
the same authored identity at approximately 3m, 2m, and 1m without close-range
disappearance.

The remaining visual question is subjective: the near body uses sparse dark
strokes so some wall detail can remain visible through transparent spaces. It
is recorded for Rain's art decision rather than treated as a renderer bug.

### Pistol

All four existing Pistol frames were refined in place: `idle_a`, `idle_b`,
`fire`, and `reload`. The silhouette now has an explicit slide/body, barrel
extension, trigger cut, angled grip, and hand/underside negative space. Fire
retains the existing muzzle flash and reload retains the existing motion
profile. The weapon system, input, recoil, HUD, and effect wiring were not
changed.

### Security

```text
SECURITY_CHANGE = NO_CHANGE_NEEDED
```

Security had already passed the previous visual direction review. The current
task did not identify a necessary consistency fix, so its authored asset was
left untouched.

## Stage red teams

### Full Human stage

```text
CODE_RED_TEAM = PASS
```

The diff is confined to the existing authored asset file. The parser, semantic
CharCell renderer, LOD selection, projection cap, transparency rules, and
gameplay wiring are unchanged. The current Release test executable loads the
asset and the projection review still passes.

```text
EFFECT_RED_TEAM = PASS_FOR_SCOPE
```

The new 3m/2m/1m captures show a narrower, more articulated human silhouette
than the preserved prior captures. The head remains readable, eyes are
prominent, and no nostrils or opaque billboard rectangle were introduced. The
1m capture remains inside the corrected cap-anchor model.

```text
PERFECTION_RED_TEAM = RAIN_REVIEW_REQUIRED
```

The three highest remaining art questions are: whether the dark torso has
enough separation from the environment at 1m; whether the near silhouette is
stylized enough rather than merely sparse; and whether the face/leg rhythm is
pleasant at the actual terminal font size. These are deliberately left for
Rain's visual judgement instead of starting another unbounded art pass.

### Pistol stage

```text
CODE_RED_TEAM = PASS
EFFECT_RED_TEAM = PASS_FOR_SCOPE
PERFECTION_RED_TEAM = RAIN_REVIEW_REQUIRED
```

The Pistol frames remain on the existing right-side viewmodel path, preserve
transparent spaces, and retain fire/reload state compatibility. The new idle
capture reads more clearly as a weapon than the previous heavy rectangular
block. The remaining question is taste and terminal-scale proportion: Rain
should decide whether the long slide/barrel and right-bottom placement are
sharp enough for the product's intended character-art identity.

### Final stage

```text
FINAL_CODE_RED_TEAM = PASS
FINAL_EFFECT_RED_TEAM = PASS_FOR_SCOPE
FINAL_PERFECTION_RED_TEAM = PENDING_RAIN_ART_DECISION
OPEN_FATAL = 0
OPEN_MAJOR = 0
OPEN_P0 = 0
OPEN_P1 = 0
```

No regression was found in Environment, Security, HUD, projection, gameplay,
or the existing systemic/replay foundation. `PASS_FOR_SCOPE` means the narrow
implementation target is complete; it does not mean Rain has accepted the
art.

## Real visual evidence

The following are new captures made after the implementation checkpoint from
the Release executable at:

```text
D:\AAAbiancheng\00_Projects\40_Coursework\2026_CPP_Immersive_ASCII_FPS\out\build\release\Release\writeover_app.exe
```

The executable ran inside a dedicated maximized Windows Terminal window. The
PNG files are real desktop captures of that terminal and runtime, not SVG,
frame-dump reconstruction, mockup, or offline renderer output. They include
ordinary desktop chrome/notification state where it was present; the game
frame itself is the foreground terminal content.

```text
V05 = CAPTURED / V05_FULL_HUMAN_3M_POLISHED.png / camera 3.5,10.5,0
V06 = CAPTURED / V06_FULL_HUMAN_2M_POLISHED.png / camera 4.5,10.5,0
V07 = CAPTURED / V07_FULL_HUMAN_1M_POLISHED.png / camera 5.5,10.5,0
V08 = CAPTURED / V08_PISTOL_DEFAULT_POLISHED.png / camera 2.5,15.5,0
```

Evidence directory:

`docs/production/evidence/art_polish01/`

The directory also retains earlier candidate/debug captures from this task so
that no evidence is silently removed. In particular,
`V07_FULL_HUMAN_1M_CANDIDATE3.png`, `V08_PISTOL_DEFAULT_CAPTURECHECK.png`,
and `V08_PISTOL_DEFAULT_QUICKCHECK.png` are explicitly excluded from the
curated evidence list because they did not show a complete valid terminal
runtime frame.

### Motion evidence

```text
REAL_MOTION_CAPTURE = CAPTURED
MOTION_FILE = MOTION_B1_APPROACH_STRAFE_POLISH01.mp4
MOTION_DURATION = 15.000 seconds
MOTION_DIMENSIONS = 2560x1440
MOTION_CODEC = H.264
MOTION_NOMINAL_RATE = 30 fps
MOTION_ENCODED_FRAMES = 407 (ffprobe; average encoded cadence about 27.13 fps)
MOTION_SEQUENCE = approach, strafe, look left/right, retreat
```

The motion clip is engineering evidence for stable projection and world
movement, not a claimed 60--90 second manual playthrough. Four inspection
frames showed real camera/world changes and the foreground Release runtime.

## Local validation

All checks below were run after the asset change and after the implementation
checkpoint commit.

```text
DEBUG_CONFIGURE = PASS
DEBUG_BUILD = PASS
DEBUG_CTEST = PASS (1/1)
RELEASE_CONFIGURE = PASS
RELEASE_BUILD = PASS
RELEASE_CTEST = PASS (1/1; ctest --test-dir out/build/release -C Release)
RELEASE_DIRECT_UNIT = PASS (176 tests, 0 failed)
CONTENT_CHECK = PASS (deterministic recompile matches)
CONTENT_TESTS = PASS (7/7)
SYSTEMIC_SCHEMA_CHECK = PASS
SYSTEMIC_SCHEMA_TESTS = PASS (10/10)
STATIC_AUDIT = PASS (COUNT=0)
CONTRACT_CHECK = PASS (forbidden/dependency/public-header checks)
RELEASE_DIRECT_SMOKE = PASS (exit 0)
DEBUG_SMOKE_WRAPPER = PASS (exit 0)
RELEASE_SMOKE_WRAPPER = NOT_RUN_AS_WRITTEN (no ctest preset named release)
```

The Release smoke result above is from the actual command:

```text
out\build\release\Release\writeover_app.exe --smoke --data-dir data
```

The wrapper limitation was not fixed because it is outside this art-only
scope; Release CTest and the direct Release smoke both passed.

### Deterministic B1 replay checks

```text
B1_SUCCESS_REPLAY = PASS; expected state reached; chapter checkpoint reached
B1_DENIED_REPLAY = PASS; access attempted/denied; gate remained closed
B1_TERMINAL_DENIED_REPLAY = PASS; terminal attempted/denied; no session
B1_BADGE_ONLY_REPLAY = PASS; badge held; ACCESS_ATTEMPTED=NO
B1_HEALTH_DEATH_REPLAY = UNVERIFIED_IN_THIS_RECEIPT; prior recovery evidence preserved
```

These replays are gameplay regression evidence only. They are not used to
claim visual acceptance.

### Benchmark

The existing Release benchmark was retained and rerun. Its labels are quoted
as emitted by the benchmark; `worst_1pct_avg_ms` is not silently renamed to a
statistical p99:

```text
CHARACTER_RENDER_WORKLOAD_240x67 = worst_1pct_avg_ms 1.026
CHARACTER_TOTAL_RUNTIME_FRAME_240x67 = worst_1pct_avg_ms 1.403
PLATFORM_WRITES = EXCLUDED
BENCHMARK_BUDGET = PASS
```

This remains a renderer/runtime proxy, not proof of end-to-end terminal
presentation cadence or 120Hz delivery.

## Git and acceptance receipt

```text
IMPLEMENTATION_COMMIT = 2c8dcc9e90f9c3172722073a04323f5fbe0d783d
IMPLEMENTATION_MESSAGE = render: polish full-human and pistol character art
NEW_RENDERER = NO
CAP_CHANGED = NO
GAMEPLAY_CHANGED = NO
RELEASE_CREATED = NO
TAG_CHANGED = NO
STEAM_OPERATION = NO
```

The final documentation/evidence receipt is intentionally kept separate from
the implementation checkpoint. The final repository HEAD, remote branch
verification, and push result are recorded in the closing execution receipt
alongside this report because a report cannot contain its own future commit
hash without becoming self-referential.

```text
FULL_HUMAN_ART_ACCEPTANCE = PASS_FOR_THIS_POLISH_SCOPE; RAIN_PENDING
SECURITY_ART_ACCEPTANCE = PASS_UNCHANGED; RAIN_PENDING
PISTOL_ART_ACCEPTANCE = PASS_FOR_THIS_POLISH_SCOPE; RAIN_PENDING
OVERALL_VISUAL_ACCEPTANCE = PENDING_RAIN_ART_REVIEW
FINAL_STATUS = READY_FOR_RAIN_ART_REVIEW
```

## Before / after summary

```text
BEFORE = prior Full Human used a broad, board-like near torso and the prior
         Pistol read as a heavy rectangular block; those captures remain in
         evidence/visual_correction01 and were not overwritten.
AFTER  = Full Human FAR/MID/NEAR now use independent narrower authored
         silhouettes with clearer head/eye/chest/leg structure; Pistol frames
         have a sharper articulated weapon silhouette while using the same
         renderer and gameplay wiring.
```

```text
MOST_LIKELY_RAIN_COMPLAINT = The Full Human torso may still blend into the
                             dark wall at 1m, or the Pistol may still need a
                             final proportion/silhouette taste pass. This is
                             why user visual acceptance remains pending.
```

