# WRITEOVER-07 — Post Complete-Game Audit Manifest

MANIFEST_KIND = post-complete-game-baseline

STATUS = READY_FOR_GPT6_COMPLETE_GAME_DIRECTOR_PASS

BASELINE_HEAD = ff12db15d584ed6aefa6c4561b1089c97440b5c2

BASELINE_COMMIT = feat: polish authored combat and campaign endings

REMOTE_HEAD_AT_SOURCE_COMMIT = 6b0d44cedc34df4ef62c1ec03cbf26d530c6a746

EXACT_HEAD_CI = PENDING (visual-pass07 source head has not yet been pushed)

EXACT_HEAD_CI_JOBS = build 103902064569 PASS, linux 103902064089 PASS,
linux-clang 103902064264 PASS, linux-arm64-link 103902064556 PASS,
macos-arm64 rerun 103902063068 PASS

EXACT_HEAD_CI_MACOS_RELEASE_BENCHMARK = TERMINAL_FULL_TIME_MS=0.287,
TERMINAL_DELTA_TIME_MS=0.105, TERMINAL_UNCHANGED_TIME_MS=0.047,
TERMINAL_WORSTCASE_TIME_MS=0.364, PVS_RENDER_TIME_MS=0.500,
PVS_TOTAL_FRAME_TIME_MS=1.625, OVERALL_BUDGET=PASS

EXACT_HEAD_CI_FIRST_ATTEMPT_OUTLIER = macOS arm64 full benchmark
TERMINAL_FULL_TIME_MS=3.546 and TERMINAL_FULL_BUDGET=FAIL; failed job was
rerun without source or threshold changes and passed.

## Scope

IMPLEMENTED = bounded complete campaign from B1 to Roof with three ending
policies, eight upper destinations, progressive unlock facts, return
transitions, case-file lead/discovery presentation, existing save/load use, and
three full route probes with ending-specific assertions. Visual pass 07 adds
authored weapon mass and restrained front human/maintenance states without
changing the renderer contract.

NOT_IMPLEMENTED = generic 41-floor tower, generic Campaign/Act/Region engine,
new save schema section, unrestricted fast travel, product release, visual
gold, course-document completion, or manual human acceptance.

## Content counts

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

## Source and artifact map

ROOM_AUTHORING = data/rooms/*.json

ROOM_COMPILED = data/rooms/*.woc

SCENE_AUTHORING = data/scenes/recovery_scene.json

SCENE_COMPILED = data/scenes/recovery_scene.bin

FACT_AUTHORING = data/facts/facts.json

FACT_COMPILED = data/facts/facts.bin

STORYLET_AUTHORING = data/storylets/storylets.json

STORYLET_COMPILED = data/storylets/storylets.bin

NPC_AUTHORING = data/npcs/npcs.json

NPC_COMPILED = data/npcs/npcs.bin

SYSTEMIC_AUTHORING = data/systemic/systemic_seed.json

SYSTEMIC_COMPILED = data/systemic/systemic_seed.bin

TEXT_AUTHORING = data/text/recovery_text.txt

CAMPAIGN_POLICY = src/app/tower_campaign_runtime.cpp and
src/app/tower_campaign_runtime.h

INTEGRATION_ROOT = src/app/composition_root.cpp

REPLAY_PROBES = tools/replay/campaign_probe_amend.txt,
tools/replay/campaign_probe_disclose.txt, and
tools/replay/campaign_probe_breach.txt

CAMPAIGN_REPLAY_GATE = scripts/complete_game_campaign_gate.ps1

## Verification receipts

DEBUG_BUILD = PASS

RELEASE_BUILD = PASS

DEBUG_UNIT_TESTS = 217/217 PASS

RELEASE_UNIT_TESTS = 217/217 PASS

CONTENT_CHECK = PASS (19 rooms, 69 facts, 27 storylets, 17 NPC profiles)

CONTENT_TESTS = 13/13 PASS

SYSTEMIC_SCHEMA_TESTS = 10/10 PASS

SYSTEMIC_SCHEMA_CHECK = PASS

STATIC_AUDIT = PASS (COUNT=0)

CONTRACT_CHECK = PASS

RELEASE_BENCHMARK = PASS (terminal full/delta/unchanged/worst-case safety,
systemic lookup/update, PVS render/total frame, raycast, overall budget)

RELEASE_SMOKE = PASS (exit 0)

CAMPAIGN_AMEND_REPLAY = PASS

CAMPAIGN_DISCLOSE_REPLAY = PASS

CAMPAIGN_BREACH_REPLAY = PASS

COMPLETE_GAME_CAMPAIGN_GATE = PASS (3/3; ending-specific facts asserted)

CAMPAIGN_REPLAY_SHARED_ASSERTIONS = all eight upper rooms visited, Roof
reached, CAMPAIGN_COMPLETION_REACHED=YES, SAVE_OK=YES, LOAD_OK=YES,
TRANSITION_DENIED=NO, PLAYER_DEAD=NO

CAMPAIGN_AMEND_FACTS = records, operations, network, force, transfer,
guard_down, guard_bypassed, alerted, archive, authority, pre_final,
completed, roof, amend

CAMPAIGN_DISCLOSE_FACTS = records, operations, operations_cooperated, network,
transfer, guard_bypassed, archive, authority, pre_final, completed, roof,
disclose; no force/alert route

ACT2_EXPANSION_GATE = PASS

RECOVERY_REPLAY_GATE = PASS (21/21 cases)

SCENARIO_MATRIX = PASS (36 classified rows, 35 executed, 20 expected success,
11 expected denial, 1 recoverable failure, 3 death/restart,
1 invalid-by-game-rules, 0 valid-state-not-covered)

LOCAL_EVIDENCE_DIRECTORIES =
docs/production/evidence/complete_game_act2_gate_20260914,
docs/production/evidence/complete_game_recovery_gate_20260914,
docs/production/evidence/complete_game_scenario_matrix_20260914,
docs/production/evidence/complete_game_campaign_gate_20260914

## Required post-push gates

COMPLETE = normal push of main from this baseline plus exact-head CI for the
final documentation head.

PENDING = final package generation and package smoke from the pushed binary.

PENDING = manual foreground Windows terminal run.

PENDING = audio listening.

PENDING = manual Breach presentation and durable reload acceptance.

PENDING = first-time classmate playtest and measured playtime.

PENDING = integrated visual acceptance for faces, NPCs, weapons, rooms, doors,
HUD, and directional labels.

## Preserved evidence and boundaries

PRESERVED = existing dirty tests/test_harness.cpp newline change, settings
files, build directories, and historical docs/production/evidence content.

NO_CLEANUP_PERFORMED = YES

NO_RESET_RESTORE_REBASE_PERFORMED = YES

NO_FORCE_PUSH_PERFORMED = YES

MANUAL_ACCEPTANCE_INFERRED_FROM_REPLAY = NO

VISUAL_GOLD_CLAIM = NO

PUBLIC_ALPHA_CLAIM = NO

COURSE_DELIVERY_CLAIM = NO

## Handoff decision

NEXT_OWNER = GPT-6 complete-game director/audit pass

SAFE_NEXT_SCOPE = manual visual/audio/first-time play review, targeted
player-critical corrections, manual Breach presentation, final package/CI/course
gates

FROZEN_SCOPE = renderer contract, authored Character-Art basis, existing
Act II-A routes, fact/systemic/storylet/save ownership, bounded lift policy

STOP_CONDITION = after exact-head CI, package smoke, and manual findings are
honestly recorded; do not turn READY_FOR_GPT6_COMPLETE_GAME_DIRECTOR_PASS into
PRODUCT_GOLD without human evidence.
