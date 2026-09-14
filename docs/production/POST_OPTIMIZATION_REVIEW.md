# Post-director optimization and player-experience review

REQUEST = Rain expanded the final director pass beyond performance and appearance:
inspect other high-value, safely verifiable improvements too. This is not campaign
expansion, a new renderer, or a claim that every possible improvement is exhausted.

COMPARISON_HEAD = 131556e4de534f51ae119b3d9ced88c493ce5ca2

COMPARISON_CI = run 34866959456; all five jobs succeeded on the first attempt.
macOS PVS_RENDER_TIME_MS=1.120; PVS_TOTAL_FRAME_TIME_MS=2.090. This is comparison
evidence, not the eventual optimization delivery's exact-head CI.

CURRENT_SOURCE = d2654f8974922a1e8db9d23e1ef99819cb5f5524, recorded by the handoff
and final optimization receipt. The original product layer, outdoor Roof,
three endings, save roles and anti-future-history fixes remain in scope and intact.

## Changes with concrete player value

1. **Terminal encoding:** append UTF-8 glyphs and decimal RGB channels directly
   into the frame payload. The first optimization stage preserved the old bytes,
   proven against an independent decimal/UTF-8 oracle, full frames and split runs.
   The shared private UTF-8 helper keeps the public conversion API unchanged.
2. **Actual terminal color correctness:** an independent regression then reproduced
   the old assumption that newline/cursor movement resets SGR. It does not. Full
   frames now carry color state across rows; each frame/delta run establishes its
   first cell's colors explicitly, including white-on-black default cells. This
   second stage intentionally changes escape sequences, not intended CharCell
   colors. It also removes redundant cross-row SGR. The failing reproduction log
   is retained; SVG previews alone cannot prove this native-protocol correction.
3. **Presentation cadence:** absolute presentation deadlines replace measuring an
   interval from the end of the previous render. Render work is no longer charged
   twice. Deterministic 120 Hz arrivals with a 2 ms render model yield the requested
   30/60/120 presentations per second; the former algorithm undershoots. Late
   deadlines are skipped, not burst-rendered. Fixed simulation stays 120 Hz.
4. **Cosmetic timing and recovery:** flash/intrusion durations use the paused game
   clock, not the number of draws. Low frame rate no longer stretches effects.
   Successful load also clears shot/hit/explosion pulses, not just subtitle and
   narrator state. These private intervals never enter the save format.
5. **Sprite cost:** asset dimensions are computed once per sprite/door draw,
   avoiding repeated row-width scans. Facing, projection, occlusion and glyph
   selection remain the same. No public asset cache or lifetime hazard is added.
6. **Menus and first use:** stable option order, small-window selection following,
   visible selection band, quiet structural border, dimmed paused context, no
   gameplay HUD over modal pages, brief first-visit guidance, and explanations for
   unavailable actions. Page changes clear stale notices. Existing settings now
   expose Auto/30/60/120 frame limit without new settings fields or bindings.
7. **Environment clarity:** reduce ordinary wall texture contrast while preserving
   panel seams, hazard marks and the service-duct readability floor. An intermediate
   lower value failed the existing duct-contrast test; the test was not weakened.
8. **Architectural doors:** authored far/mid/near frame silhouettes use one joined
   jamb/header instead of stacked curved outlines. Same asset dimensions and
   gameplay geometry; steel leaf underpaint, shaded recesses and observation
   windows remain glyph-defined. Multi-angle production captures were reviewed.

## Performance evidence (same existing workloads and gates)

Two sequential local comparison runs and two final protocol-corrected runs,
before long replay/build concurrency. Values below are observed ranges, not
confidence intervals. Raw CSVs retain avg_ms, worst_1pct_avg_ms, min_ms and
max_ms under their actual names. Never call worst_1pct_avg_ms p99.

| Metric | Comparison runs | Final runs |
|---|---:|---:|
| TERMINAL_FULL_TIME_MS | 0.334–0.346 | 0.143–0.145 |
| TERMINAL_DELTA_TIME_MS | 0.134–0.140 | 0.046–0.048 |
| TERMINAL_UNCHANGED_TIME_MS | 0.064–0.113 | 0.032–0.100 |
| TERMINAL_WORSTCASE_TIME_MS | 0.333–0.347 | 0.136–0.149 |
| PVS_RENDER_TIME_MS | 0.761–0.793 | 0.751–0.944 |
| PVS_TOTAL_FRAME_TIME_MS | 1.459–1.492 | 0.985–1.223 |
| character_total_runtime_frame_240x67 avg_ms | 0.819–0.826 | 0.662–0.682 |
| full payload bytes | 38448 | 36132 |
| worst-case payload bytes | 50659 | 48349 |

Every recorded final benchmark gate passed. Two-run arithmetic averages suggest
about 58% less full encode time, 66% less delta encode time, 18% less total mean
time and 25% less worst-one-percent total time. These are local observations,
not a universal hardware guarantee. Render-only timing did not consistently
improve; unchanged-frame measurements remain noisy. No favorable sample is hidden
by omitting an unfavorable final sample.

WORKLOAD = unchanged 1200 total-frame samples / 25 NPCs / 240x67 / 6 ms gate.
DISPLAY_FPS = NOT_MEASURED; terminal host writes/display are outside these CPU
benchmarks. Do not convert inverse benchmark time into delivered display FPS.
UPPER_PRESENTATION_BOUND = existing simulation-paced 120 Hz, not unlimited FPS.

RAW_EVIDENCE = evidence/optimization_baseline_20260915_{1,2}.log;
optimization_encoder_20260915_{1,2}.log (first byte-preserving stage);
optimization_final_20260915_{1,2}.log (before terminal protocol correction);
optimization_protocol_final_20260915_{1,2}.log (current source).
EXPECTED_FAILED_REPRO = evidence/optimization_ansi_repro_20260915.log.

## Production visual review

AUTHOR_RENDER_CRITIQUE_REVISE_RENDER = completed for architectural doors/panels.
V1 = evidence/chapter01_creative_polish/optimization_v1_20260915 (17 views).
V2 = evidence/chapter01_creative_polish/optimization_v2_20260915 (17 views).
V2 boot_final/controls_final are the later final modal-HUD suppression captures.
PNGs are previews of real production CharCell SVG exports, not native screenshots
or game assets. Raw SVGs and scratch user-data remain local, untracked evidence.

PROTECTED = authored characters and weapons, real directional selections, glyph
silhouettes, room function/path/geometry, monochrome-readable structure, quiet
institutional palette, existing consequence-driven narration. No images were
rasterized into characters; no block/Braille pixel framebuffer was introduced.

REVIEW_BOUNDARY = distant people/props remain stylized and sometimes sparse.
Native font appearance, actual terminal pacing and audio listening still need
human review. This is a bounded improvement, not a photorealism claim.

## Broader scope disposition

IMPLEMENTED_THIS_EXTENSION = performance, cadence, terminal output correctness,
effect timing/load cleanup, menu stability, unavailable-action feedback, in-game
frame limit, onboarding and door/material/panel clarity.
RETAINED_ORIGINAL_PASS = boot/continue/new game, pause, controls, preferences,
inspect, bounded sensory/history/dialogue, separate save roles, death recovery,
ending summary, outdoor Roof, dynamic hints and priority-controlled narration.
RECHECKED_WITHOUT_REDESIGN = combat outcomes, facing, campaign routes/endings,
long-range consequences, backtracking, systemic state and persistence.
FROZEN = 19 rooms, 3 endings, public gameplay APIs, save/content schema, simulation
timing, existing procedural audio architecture and benchmark contracts.
HUMAN = listening, terminal/font/latency judgment, first-time classmate pacing.
FINAL_AUDIT = exhaustive fault/lifetime/security/platform edge combinations.
FINAL_CLOSURE = audit-confirmed bounded repairs, course report/PPT/diagrams/demo
and packaging metadata, not unsolicited redesign.

## Verification checkpoint

DEBUG_AND_RELEASE = 233 unit cases / CTest 2/2 passed in both builds.
CONTENT = deterministic compiler check, 13 content tests, 10 schema tests,
invalid-seed startup, static COUNT=0 and public contract check passed.
FACING = 36/36 exact authored selections.
LOW_CAP_ROUTES = actual Chapter systemic replay at cap30 and aggressive replay
at cap60 passed; objective and narrative presentation receipts present. These
use task-owned settings under out/optimization-20260915, not Rain's settings.
LONG_REPLAYS = 21 recovery; 36 classified scenarios / 35 executed / one invalid /
zero valid gaps; three Act II routes; four campaign routes and four independent
ending reloads: PASS. Product integration / New Game / eight staged rollback
faults / completed reload passed in Debug. All use d2654f8 implementation.
PACKAGE = optimized-package-20260915 candidate; positive smoke and three negative
resource probes passed. Exact hash is in the current handoff and local receipt.
FINAL_CI = resolve final optimization delivery receipt; do not inherit comparison
head CI or package hashes.

IMPORTANT_FILES / CLASSES / FUNCTIONS / DESIGN / CLASSROOM_EXPLANATION are in
the optimization addendum of COMPLETE_GAME_TEACHBACK.md. No new tutorial tree.
