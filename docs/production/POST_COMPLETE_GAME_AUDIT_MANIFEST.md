# WRITEOVER-07 — Frozen product/campaign audit manifest

HISTORICAL_MANIFEST = pre-audit source and evidence, not the current candidate.
CURRENT_AUTHORITY = FINAL_PRODUCT_CLOSURE_REPORT.md and release/VERSIONING.md.
The post-audit closure supersedes its implementation/status claims. Original
failure evidence and source inventory remain preserved below.

MANIFEST_KIND = final-product-campaign-audit-input

START_HEAD = 43083373a03b5583ae207001067cd866bdb9a2cc

BASELINE_HEAD = d2654f8974922a1e8db9d23e1ef99819cb5f5524

DELIVERY_HEAD = d5ef249adf54431e605603c3436000cc39421c36.
EXACT_HEAD_CI = run 34872278294, SUCCESS in all five jobs without rerun.
Later documentation commits do not change the implementation baseline or claim
new tests. Player release provenance is in ../release/VERSIONING.md.

AUTHORITATIVE_HANDOFF = POST_ULTRA_FINAL_HANDOFF.md

STATUS = READY_FOR_DSV4_1F_FINAL_AUDIT

SUPERSESSION = this manifest and POST_ULTRA_FINAL_HANDOFF supersede the older
director report's open Roof implementation decision and pre-audit human gate.
Human review remains open; Rain explicitly permits a mechanical audit first.
The previous report and all earlier failed/successful evidence remain historical.

## Frozen scope and content

PLAYABLE_ROOMS = 19
EXPLICIT_REGION_RECORDS = 0
SCENE_TRANSITIONS = 32
SCENE_ENTITIES = 152
PATROL_ROUTES = 14
FACT_RECORDS = 69
STORYLET_RECORDS = 27
NPC_PROFILE_RECORDS = 17
FULL_CAMPAIGN_REPLAY_RUNTIME_NPCS = 16
UPPER_DESTINATIONS = 8
ENDINGS = 3
SAVE_SCHEMA_CHANGE = NO
PUBLIC_CONTRACT_DELTA = two bounded Settings preference fields; ADR-0010
WORLD_IDENTITY = authored CharCell/Unicode FPS/MUD; no rasterized image assets
NOT_IMPLEMENTED = 41 playable floors, generic act/region/quest/UI framework,
new tactical-combat model, new population or Product Gold.
PUBLICATION = Rain authorized the unchanged Windows candidate as a Pre-release
on 2026-09-15. It does not close the remaining human acceptance or final audit.

## Audit source map

COMPOSITION_AND_RECOVERY = src/app/composition_root.cpp; src/app/game_main.cpp
PRODUCT = src/app/player_product.h; campaign_panel.h
PERCEPTION = src/app/perception_feed.h; player_perception.h
SAVE_ROLE_POLICY = src/app/product_save.h; existing core/save.cpp and callbacks
SETTINGS = include/writeover/core/settings.h; src/core/settings.cpp; ADR-0010
ROOF = data/rooms/room_roof_exit.json/.woc; data/scenes/recovery_scene.json/.bin
FINITE_PLANE_FIX = src/render/character_renderer.cpp (nonfinite intersection)
BENCHMARK_DIAGNOSIS = tools/bench/main.cpp (all original samples/gates retained)
TESTS = tests/test_product.cpp; tests/test_core.cpp; tests/test_render.cpp; registration
PRESENTATION_CLOCK = src/core/presentation_cadence.h; src/app/presentation_pulse.h
TERMINAL_ENCODING = src/render/frame_encoder.cpp; utf.cpp; utf_append.h
ADDITIONAL_PUBLIC_CONTRACT_CHANGE = NO (optimization is private implementation)
AI_CAMPAIGN_MECHANICS_CHANGED_BY_OPTIMIZATION = NO
STATE_WRITERS_READERS = Engine schedules render deadlines; game clock owns pulse
intervals; combat/narrator trigger pulses; render reads them; successful load resets
presentation. Menu writes existing Settings.frame_rate_cap; Engine reads it.
Encoder owns only prior-frame/color-run state, never saved gameplay facts.
PRODUCT_REPLAY = tools/replay/campaign_probe_amend_product.txt; product_new_game.txt
PRODUCT_GATE = scripts/player_product_gate.ps1
CAMPAIGN_AUTHORITY = src/app/tower_campaign_runtime.cpp/.h; existing facts/systemic
AUTHORING = data/rooms, scenes, characters, text, npcs, facts, storylets, systemic
COMPILER = tools/contentc/contentc.py; tools/systemic scripts
CI = .github/workflows/ci.yml (Windows product/recovery gate added)
PLAYER_INSTRUCTIONS = docs/release/PLAYER_README.txt
PRESENTER_KNOWLEDGE = COMPLETE_GAME_TEACHBACK.md, final product addendum

## Focused audit questions

- Does every successful load clear future presentation while restoring only
  authoritative saved gameplay? Check staged commit, eight fault stages and CRC.
- Are manual, checkpoint, pre-final, completion and resume roles distinct?
  A role write and resume write are individually atomic, not jointly atomic.
- Is New Game a true composition reconstruction, including memory, facts,
  objectives, inventory and UI caches? Verify existing file preservation.
- Do held Fire/Interact/movement, repeats, focus loss and remapped bindings stay
  fenced across all menu exits, not only a single nominal path?
- Can long Case File/directory/ending/history text be reached at 48x18 and 80x24?
  Verify footer preservation, nearest focus ties and minimum-size fallback.
- Is each feed observation actually same-room, visible/audible or direct feedback?
  No consumers may infer gameplay from whether a cue was displayed.
- Check dialogue classification against the actual current speaker bank.
- Check the new Roof parapet and old valid Roof save positions; no out-of-bounds
  navigation, sprite-only doorway or invented infinite ceiling.
- Inspect the large composition root integration for callback lifetime/ordering.
  Do not turn the audit into an unsolicited framework extraction.
- Separate wall-time preemption from expensive compute using slow-frame stages
  and POSIX std::clock; never relabel worst_1pct_avg_ms as p99.

## Regression evidence

DEBUG_BUILD = PASS
RELEASE_BUILD = PASS
UNIT_TESTS = 233/233 PASS in both configurations
CTEST = 2/2 PASS in both configurations
CONTENT_DETERMINISTIC = PASS (19 rooms / 69 facts / 27 storylets / 17 profiles)
CONTENT_TESTS = 13/13 PASS
SYSTEMIC_SCHEMA_TESTS = 10/10 PASS; production seed schema PASS
INVALID_SEED_STARTUP = PASS
STATIC_AUDIT = COUNT=0
CONTRACT_CHECK = PASS
RECOVERY_REPLAY_GATE = 21/21 PASS; optimization_recovery_20260915
SCENARIO_MATRIX = 36 classified / 35 executed / 1 invalid-by-game-rules /
0 valid-state-not-covered; PASS;
optimization_scenarios_20260915
ACT2_ROUTE_COVERAGE_GATE = 3/3 PASS; optimization_act2_20260915
FINAL_CAMPAIGN_GATE = 4/4 routes and 4/4 independent completed-save reloads PASS;
optimization_campaign_20260915 at final Release source d2654f8
PRODUCT_GATE = PASS; optimization_product_20260915 (final Debug gameplay)
PRODUCT_NEW_GAME = completed world -> B1, health100, evidence0; save hashes unchanged
PRODUCT_SAVE_FAULTS = 8/8 stages rejected and rolled back; completed reload PASS
FACING_MAPPING = 36/36 exact authored selections; optimization_art_20260915.log
LOCAL_BENCHMARK = all gates PASS in both sequential final runs.
PVS_RENDER_TIME_MS=0.751-0.944; PVS_TOTAL_FRAME_TIME_MS=0.985-1.223;
TERMINAL_FULL_TIME_MS=0.143-0.145; TERMINAL_DELTA_TIME_MS=0.046-0.048;
TERMINAL_UNCHANGED_TIME_MS=0.032-0.100; TERMINAL_WORSTCASE_TIME_MS=0.136-0.149.
Metrics are worst_1pct_avg_ms; raw avg_ms/min_ms/max_ms remain in the CSVs.
LOW_CAP_ROUTES = Chapter systemic cap30 and aggressive cap60 production replays PASS.

Evidence paths above are relative to docs/production/evidence and use the final
optimization implementation. Original pass evidence is historical comparison.
Source/build/hash metadata are in evidence/optimization_local_receipt_20260915.json.
Performance stages, unchanged workload and limitations: POST_OPTIMIZATION_REVIEW.md.

## Production visual evidence

CURRENT_REVIEW = evidence/chapter01_creative_polish/optimization_v2_20260915
COMPARISON_REVIEW = evidence/chapter01_creative_polish/optimization_v1_20260915
ORIGINAL_PRODUCT_REVIEW = evidence/ultra_visual_20260914
ROOF_FINAL = roof-skyline.png (production CharCell SVG export, not native screenshot)
ROOF_PREVIOUS_ITERATION = roof.png (lower skyline, retained comparison)
PRODUCT_PANELS = current boot_final.png / controls_final.png;
current ending_final.png / settings_final.png; original ending.png is comparison
ART = guard_sheet.png; human_sheet.png; maintenance_sheet.png; weapon_sheet.png
ROOMS = B1 plus nine named interior review frames; retained prior-stage exports
do not claim to reflect final transient subtitle expiry changes.
Evidence README describes capture stages and acceptance limits.

## Package and exact-head CI

CANDIDATE_PACKAGE = out/optimized-package-20260915/WRITEOVER-07-audit-candidate.zip
CANDIDATE_VERSION = 0.1.0-complete-campaign-candidate
CANDIDATE_PACKAGE_COMMIT = d2654f8974922a1e8db9d23e1ef99819cb5f5524
CANDIDATE_PACKAGE_SHA256 = 1901D0FE2889C90818B83935CE40797C5D5914115AC2CD00A3BB9A4891B27799
CANDIDATE_PACKAGE_BYTES = 609629
PACKAGE_GATES = clean positive smoke and three missing-resource negative probes PASS
PACKAGE_DEV_GARBAGE = 0
PACKAGE_USER_DATA_SEPARATION = PASS
PACKAGE_RELEASE_STATUS = subsequently published unchanged as the historical
candidate-0.1.0-complete-campaign-20260915 Pre-release; the former local-only
statement described the state before that authorized publication.

EXACT_HEAD_CI = historical delivery d5ef249, run 34872278294, all five jobs succeeded;
the earlier pending-at-authoring text is superseded by that recorded receipt.
CI_JOBS = Windows; Linux GCC; Linux Clang; macOS arm64; Linux ARM64 link
COMPARISON_RUN = 34866959456 at 131556e: all five jobs succeeded, no retry.
COMPARISON_MACOS = PVS_RENDER_TIME_MS=1.120; PVS_TOTAL_FRAME_TIME_MS=2.090.
EXACT_RECEIPT = local evidence/optimization_delivery_receipt_20260915.json + final
chat, or verify GitHub Actions headSha directly; no commit just to sample CI again.
PREDECESSOR_RUN = 34849065030 at 4308337: only macOS Release benchmark failed
PREDECESSOR_MACOS = total worst_1pct_avg_ms=8.895, max_ms=31.339;
PVS_RENDER_TIME_MS=1.158. Do not present this as current source performance.
RETRY_POLICY = at most one justified unchanged-source failed-job rerun;
no sample/threshold/workload reductions and no documentation commit for sampling.

## Preserved boundaries / next owner

USER_TRACKED_DIRTY = tests/test_harness.cpp EOF newline; never staged
HISTORICAL_EVIDENCE = retained, including failed intermediate replay captures
PRESERVATION_CAVEAT = root-run unit fixture settings were rewritten once; see
handoff. No byte-identical preservation claim for those three untracked fixtures.
NO_RESET_RESTORE_REBASE_FORCE_PUSH = YES
NO_BROAD_CLEANUP = YES

FOREGROUND_WINDOWS_TERMINAL = NOT_PERFORMED
RAIN_VISUAL_ACCEPTANCE = NOT_CLAIMED
AUDIO_HUMAN_ACCEPTANCE = NOT_CLAIMED
FIRST_TIME_CLASSMATE_PLAYTEST = NOT_PERFORMED
MEASURED_HUMAN_PLAYTIME = NOT_AVAILABLE

NEXT_OWNER = exhaustive mechanical audit, then bounded final closure and course
materials. Human acceptance stays separate. Do not reopen art direction, renderer
identity, campaign size or save schema absent an actual audit-confirmed blocker.

## Full implementation file inventory

Relative to START_HEAD through d2654f8; A=new, M=modified. Detailed evidence files
are enumerated by the receipts and Git. The subsequent delivery commit updates
production handoffs/teachback/creative direction and adds the optimization report,
compact logs and production PNG previews; no additional source change is implied.

```text
M .github/workflows/ci.yml
M CMakeLists.txt
M data/characters/b1_character_art.txt
M data/rooms/room_roof_exit.json
M data/rooms/room_roof_exit.woc
M data/scenes/recovery_scene.bin
M data/scenes/recovery_scene.json
A docs/adr/ADR-0010-player-perception-preferences.md
M docs/production/CHAPTER01_CREATIVE_DIRECTION.md
M docs/production/COMPLETE_GAME_TEACHBACK.md
M docs/production/POST_COMPLETE_GAME_AUDIT_MANIFEST.md
A docs/production/POST_ULTRA_FINAL_HANDOFF.md
M docs/release/PLAYER_README.txt
M include/writeover/core/settings.h
A scripts/player_product_gate.ps1
M src/app/campaign_panel.h
M src/app/composition_root.cpp
M src/app/composition_root.h
M src/app/game_main.cpp
A src/app/perception_feed.h
A src/app/player_perception.h
A src/app/player_product.h
A src/app/presentation_pulse.h
A src/app/product_save.h
M src/core/engine.cpp
A src/core/presentation_cadence.h
M src/core/settings.cpp
M src/render/character_renderer.cpp
M src/render/frame_encoder.cpp
M src/render/utf.cpp
A src/render/utf_append.h
M tests/test_core.cpp
M tests/test_main.cpp
A tests/test_product.cpp
M tests/test_render.cpp
M tools/bench/main.cpp
M tools/contract_check/.contract_baseline.json
A tools/replay/campaign_probe_amend_product.txt
M tools/replay/normal_quit.txt
A tools/replay/product_new_game.txt
```
