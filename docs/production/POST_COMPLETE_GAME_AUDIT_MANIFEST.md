# WRITEOVER-07 — Frozen product/campaign audit manifest

MANIFEST_KIND = final-product-campaign-audit-input

START_HEAD = 43083373a03b5583ae207001067cd866bdb9a2cc

BASELINE_HEAD = f0e30afe9c330899c4e9fdf105c6c71b3001ca6c

DELIVERY_HEAD = the documentation/evidence descendant containing this manifest;
resolve git rev-parse HEAD and match its exact GitHub Actions headSha.

AUTHORITATIVE_HANDOFF = POST_ULTRA_FINAL_HANDOFF.md

STATUS = SOURCE_CONTENT_FROZEN_PENDING_DELIVERY_RECEIPT

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
new tactical-combat model, new population, Product Gold or public Release.

## Audit source map

COMPOSITION_AND_RECOVERY = src/app/composition_root.cpp; src/app/game_main.cpp
PRODUCT = src/app/player_product.h; campaign_panel.h
PERCEPTION = src/app/perception_feed.h; player_perception.h
SAVE_ROLE_POLICY = src/app/product_save.h; existing core/save.cpp and callbacks
SETTINGS = include/writeover/core/settings.h; src/core/settings.cpp; ADR-0010
ROOF = data/rooms/room_roof_exit.json/.woc; data/scenes/recovery_scene.json/.bin
FINITE_PLANE_FIX = src/render/character_renderer.cpp (nonfinite intersection)
BENCHMARK_DIAGNOSIS = tools/bench/main.cpp (all original samples/gates retained)
TESTS = tests/test_product.cpp; existing tests registration
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
UNIT_TESTS = 229/229 PASS in both configurations
CTEST = 2/2 PASS in both configurations
CONTENT_DETERMINISTIC = PASS (19 rooms / 69 facts / 27 storylets / 17 profiles)
CONTENT_TESTS = 13/13 PASS
SYSTEMIC_SCHEMA_TESTS = 10/10 PASS; production seed schema PASS
INVALID_SEED_STARTUP = PASS
STATIC_AUDIT = COUNT=0
CONTRACT_CHECK = PASS
RECOVERY_REPLAY_GATE = 21/21 PASS; ultra_recovery_final_20260914
SCENARIO_MATRIX = 36 classified / 35 executed / 1 invalid-by-game-rules /
0 valid-state-not-covered; PASS; ultra_scenarios_20260914
ACT2_ROUTE_COVERAGE_GATE = 3/3 PASS; ultra_act2_routes_20260914
FINAL_CAMPAIGN_GATE = 4/4 routes and 4/4 independent completed-save reloads PASS;
ultra_campaign_final_20260915 at final Release source f0e30af
PRODUCT_GATE = PASS; ultra_product_no_future_20260914 (final Debug gameplay)
PRODUCT_NEW_GAME = completed world -> B1, health100, evidence0; save hashes unchanged
PRODUCT_SAVE_FAULTS = 8/8 stages rejected and rolled back; completed reload PASS
FACING_MAPPING = 36/36 exact authored selections; art-final.log
LOCAL_BENCHMARK = all gates PASS; PVS_RENDER_TIME_MS=1.175;
PVS_TOTAL_FRAME_TIME_MS=2.262; TERMINAL_FULL_TIME_MS=0.438;
TERMINAL_DELTA_TIME_MS=0.128; TERMINAL_UNCHANGED_TIME_MS=0.099;
TERMINAL_WORSTCASE_TIME_MS=0.466. Values are worst-one-percent averages.

Evidence paths above are relative to docs/production/evidence. Chapter/Act II
gates preceded the final presentation-only sticky-subtitle removal; the final
campaign/product gate and exact-head CI exercise the corrected integration.
Source/build/hash metadata and compact receipts are in
evidence/ultra_local_receipt_20260915.json.

## Production visual evidence

CURRENT_REVIEW = evidence/ultra_visual_20260914
ROOF_FINAL = roof-skyline.png (production CharCell SVG export, not native screenshot)
ROOF_PREVIOUS_ITERATION = roof.png (lower skyline, retained comparison)
PRODUCT_PANELS = boot.png; controls-48x18.png; ending.png
ART = guard_sheet.png; human_sheet.png; maintenance_sheet.png; weapon_sheet.png
ROOMS = B1 plus nine named interior review frames; retained prior-stage exports
do not claim to reflect final transient subtitle expiry changes.
Evidence README describes capture stages and acceptance limits.

## Package and exact-head CI

CANDIDATE_PACKAGE = out/ultra-package-20260915/WRITEOVER-07-audit-candidate.zip
CANDIDATE_VERSION = 0.1.0-complete-campaign-candidate
CANDIDATE_PACKAGE_COMMIT = f0e30afe9c330899c4e9fdf105c6c71b3001ca6c
CANDIDATE_PACKAGE_SHA256 = 33690097B77ED32CAEF19BF504EBA2A36C45E8E83DEBBDC687443D5AB0FF0D20
CANDIDATE_PACKAGE_BYTES = 608293
PACKAGE_GATES = clean positive smoke and three missing-resource negative probes PASS
PACKAGE_DEV_GARBAGE = 0
PACKAGE_USER_DATA_SEPARATION = PASS
PACKAGE_RELEASE_STATUS = local audit candidate only; no tag or GitHub Release

EXACT_HEAD_CI = final delivery receipt must match DELIVERY_HEAD; pending at authoring
CI_JOBS = Windows; Linux GCC; Linux Clang; macOS arm64; Linux ARM64 link
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
