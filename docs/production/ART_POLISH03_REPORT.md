# WRITEOVER-07 ART-POLISH-03 / VIEWMODEL-CORRECTION-01

This receipt records a narrow visual correction pass. It is not a product
Gold declaration and it does not replace Rain's final visual decision.

## Boundary

Addressed the two current player-facing complaints:

- Full Human lower-body strokes were too close to the dark environment.
- The Pistol read too much like a lower-right horizontal presentation panel
  instead of a held first-person weapon.

The pass retained the existing Character Renderer, projection caps, HUD
layout, weapon state wiring, gameplay, map, story, and systemic foundation.
No new renderer, half-block world path, gameplay system, map content, weapon
type, branch, tag, release, PR, or Steam operation was created.

## Scene checkpoint

    START_HEAD = 564df182f228018e4e8d1bf487bdb6a93c55e2c0
    START_BRANCH = main
    REMOTE = https://github.com/Rainflowers686/WRITEOVER-07.git
    CANONICAL_ROOT = D:\AAAbiancheng\00_Projects\40_Coursework\2026_CPP_Immersive_ASCII_FPS
    CHARACTER_VISUAL_ACCEPTANCE = PENDING_RAIN_FINAL_VISUAL_DECISION

The worktree already contained user-owned dirty report work and earlier
visual evidence before this pass. Those files were preserved and were not
staged as part of this receipt.

## Implementation

### Full Human

Changed only the authored FAR/MID/NEAR character data in
data/characters/b1_character_art.txt:

- kept the existing head, eye, brow, hair, and no-nostril face direction;
- kept the authored widths bounded (MID max width 15, NEAR max width 20,
  unchanged from the previous asset bounds);
- replaced broad repeated bright blocks with sparse + leg/hip accents;
- retained the gap between the legs and the dark outer contour so the lower
  body reads as a standing human rather than a wide panel;
- did not change the near cap or projection math.

### Pistol

Changed the four existing authored Pistol frames in the same data file:

- idle_a, idle_b, fire, and reload share one right-hand holding composition;
- the muzzle/slide now starts toward the upper-left/inward direction;
- the body and grip step down toward the lower-right anchor;
- the trigger, slide/barrel, grip, fire flash, and reload variation remain
  represented by glyphs;
- no weapon system or state-selection code changed.

### HUD and Security

No HUD code changed. The existing DrawPistolViewmodel path remains separate
from the compact player HUD, and the new full weapon drawing stays in that
viewmodel layer.

Security was not edited. SECURITY_NO_CHANGE_NEEDED = YES; the accepted
Security direction was outside the current complaint.

    ART_ONLY = YES
    NARROW_VIEWMODEL_CODE_CHANGE = NO
    RENDERER_REWRITE = NO
    CAP_CHANGED = NO
    GAMEPLAY_CHANGED = NO
    SECURITY_CHANGED = NO

## Validation

All commands below were run against the final authored asset state.

    DEBUG_CONFIGURE = PASS
    DEBUG_BUILD = PASS
    DEBUG_CTEST = PASS (1/1)

    RELEASE_CONFIGURE = PASS
    RELEASE_BUILD = PASS
    RELEASE_CTEST = PASS (1/1)
    RELEASE_DIRECT_UNIT = PASS (176 tests, 0 failed)

    CONTENT_CHECK = PASS (contentc --check)
    CONTENT_TESTS = PASS (7/7)
    SYSTEMIC_SCHEMA_CHECK = PASS
    SYSTEMIC_SCHEMA_TESTS = PASS (10/10)
    STATIC_AUDIT = PASS (COUNT=0)
    CONTRACT_CHECK = PASS
    RELEASE_DIRECT_SMOKE = PASS (exit=0)

    B1_SUCCESS_REPLAY = PASS (expected/chapter checkpoint reached)
    B1_DENIED_REPLAY = PASS (denial expected state reached)
    B1_TERMINAL_DENIED_REPLAY = PASS (denial expected state reached)
    B1_BADGE_ONLY_REPLAY = PASS (ACCESS_ATTEMPTED=NO)

    RELEASE_BENCHMARK = PASS
    PVS_RENDER_TIME_MS = 0.980 (reported worst1_avg label)
    PVS_TOTAL_FRAME_TIME_MS = 1.452 (reported worst1_avg label; platform writes excluded)
    END_TO_END_120HZ_PROOF = NOT_CLAIMED

The direct console smoke/replay invocation reports the legacy conhost
compatibility path because it is not a Windows Terminal probe. That does not
change the separate foreground evidence capture, which used the actual
Windows Terminal window.

## Real visual evidence

All curated stills below were captured from:

    EXECUTABLE = out\build\release\Release\writeover_app.exe
    CAPTURE_SURFACE = dedicated foreground Windows Terminal desktop
    CAPTURE_METHOD = real runtime desktop capture
    SVG_OR_FRAME_DUMP_RECONSTRUCTION = NO
    MOCKUP = NO

Camera positions used by the real executable:

    V05 = --camera 3.5 10.5 0 0
    V06 = --camera 4.5 10.5 0 0
    V07 = --camera 5.5 10.5 0 0
    V08 = --camera 2.5 15.5 0 0

Curated evidence:

- docs/production/evidence/art_polish03/V05_FULL_HUMAN_3M_POLISHED03.png
- docs/production/evidence/art_polish03/V06_FULL_HUMAN_2M_POLISHED03.png
- docs/production/evidence/art_polish03/V07_FULL_HUMAN_1M_POLISHED03.png
- docs/production/evidence/art_polish03/V08_PISTOL_DEFAULT_POLISHED03.png
- docs/production/evidence/art_polish03/MOTION_B1_APPROACH_STRAFE_POLISH03.mp4

The motion capture is a real 15.000-second desktop capture. It contains the
approach/strafe/look/back-away probe and is engineering evidence, not the
final 60–90 second player-playthrough claim.

The previous captures remain available for comparison and were not
overwritten:

- BEFORE Full Human:
  docs/production/evidence/art_polish01/V07_FULL_HUMAN_1M_POLISHED.png
- BEFORE Pistol:
  docs/production/evidence/art_polish01/V08_PISTOL_DEFAULT_POLISHED.png

## Three red teams

### CODE RED TEAM

    RESULT = PASS

The implementation diff is limited to one existing authored character-art
data file. No C++ source, public header, renderer architecture, cap, HUD
wiring, gameplay contract, map, or Security asset changed. The pre-existing
dirty report and older evidence were not staged.

### EFFECT RED TEAM

    RESULT = PASS_FOR_THIS_NARROW_SCOPE

The 3m/2m/1m Full Human captures retain the same face and projection while
the lower-body accents and leg gap provide a clearer local silhouette. The
Pistol captures now have an inward upper-left muzzle and a lower-right grip,
so the overall weapon axis reads as a held viewmodel rather than a full
horizontal side display. Fire/reload compatibility is retained in the same
four-frame asset bank.

### PERFECTION RED TEAM

    RESULT = RAIN_REVIEW_REQUIRED

Remaining bounded risks:

1. The Full Human lower body is still intentionally sparse and may remain
   somewhat dark in the dimmest environment regions.
2. The Pistol is deliberately small authored character art; Rain may still
   prefer a different angle or a clearer hand/wrist cue.
3. The terminal font and desktop capture scale affect subjective sharpness
   and cannot be resolved honestly by automated tests alone.

These are visual acceptance questions, not reasons to widen this pass into a
renderer or gameplay redesign.

## Final verdict

    FULL_HUMAN_ART_ACCEPTANCE = PASS_FOR_NARROW_SCOPE; RAIN_PENDING
    SECURITY_ART_ACCEPTANCE = PASS_UNCHANGED; RAIN_PENDING
    PISTOL_ART_ACCEPTANCE = PASS_FOR_NARROW_SCOPE; RAIN_PENDING
    OVERALL_VISUAL_ACCEPTANCE = PENDING_RAIN_FINAL_VISUAL_DECISION

    OPEN_FATAL = 0
    OPEN_MAJOR = 0
    OPEN_P0 = 0
    OPEN_P1 = 0

    FINAL_STATUS = READY_FOR_RAIN_FINAL_VISUAL_DECISION

This pass does not claim PVS01_GOLD, PRODUCT_GOLD, or public-release
readiness.
