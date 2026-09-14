# Post-expansion audit manifest

This manifest records the state after the authored expansion and the current
creative-critical pass. It is deliberately scalar and checkable; old evidence
is preserved and is not silently overwritten.

## Identity

```text
PROJECT = WRITEOVER-07
REPOSITORY = Rainflowers686/WRITEOVER-07
BRANCH = main
CANONICAL_ROOT = D:\AAAbiancheng\00_Projects\40_Coursework\2026_CPP_Immersive_ASCII_FPS
BASELINE_HEAD = 23eaddbe1a2476519fc91dd66d5dfefef069e512
REMOTE_BASELINE_BEFORE_PUSH = 187806b95506f9f7d17279eb1af5f338ee3cb9b2
ANCESTOR_PRESERVED = 5434f8a
PUSH_STATUS_AT_MANIFEST_CREATION = PUSHED_NORMALLY
PUSHED_HEAD = a1feb5c160eb91ba0980f5d156ee3cac2550f698
STATUS = CREATIVE_BASELINE_ESTABLISHED_ACT2_RECORDS_ROUTE_VERIFIED_NOT_READY_FOR_HANDOFF
```

The old `tests/test_harness.cpp` newline-only worktree edit, historical
evidence, settings files and user data were intentionally not included in the
baseline commit.

## Content inventory

```text
ROOM_COUNT = 11
FACT_COUNT = 41
STORYLET_COUNT = 19
NPC_PROFILE_COUNT = 10
ACT2_ROOM_COUNT = 5
ACT2_NEW_NPC_COUNT = 4
ACT2_NEW_FACT_COUNT = 17
ACT2_NEW_STORYLET_COUNT = 6
SCENE_BINARY_BYTES = 14645
SYSTEMIC_SEED_BYTES = 958
```

Act II room ids:

```text
room_act2_service_concourse
room_act2_records_archive
room_act2_power_utility
room_act2_transit_control
room_act2_observation_gallery
```

Act II NPC ids:

```text
records_operator
power_technician
security_response
observation_analyst
```

Act II storylet ids:

```text
storylet_act2_concourse
storylet_act2_records
storylet_act2_power
storylet_act2_observation
storylet_act2_transit
storylet_act2_checkpoint
```

The 17 Act II facts are authored in `data/facts/facts.json`; they cover entry,
dispatch/archive access, observation/camera state, power assistance/reroute,
utility noise, Transit guard outcomes, checkpoint and access denial.

## Source/change map

| Area | Files | Result |
|---|---|---|
| Authored characters/weapons | `data/characters/b1_character_art.txt` | Full Human/Maintenance face hierarchy, directional art, synchronized Pistol/Stunner states, door frame and room prop art. |
| Renderer | `src/render/character_renderer.cpp`; `include/writeover/render/character_renderer.h` | Door-frame layer, continuous ceiling material, restrained surface lift, semantic face colors, grounded weapon anchor. |
| Composition/runtime | `src/app/composition_root.cpp`; `src/app/game_main.cpp`; `src/app/scene_runtime.cpp` | Act II NPCs, terminals, facts, storylets, transitions, backtracking, signed camera pitch parsing and opening composition. |
| Content | `data/rooms/room_act2_*.json`; `data/scenes/recovery_scene.json`; `data/npcs/npcs.json`; facts/storylets/text/systemic | Five spaces with focal equipment, role NPCs, systemic branches and durable consequences. |
| Contract | `docs/adr/ADR-0012-authored-door-frame-layer.md`; `tools/contract_check/.contract_baseline.json` | The one public enum addition is recorded and hash-pinned. |
| Teaching/handoff | `docs/production/CHAPTER01_CREATIVE_DIRECTION.md`; `OVERNIGHT_EXPANSION_TEACHBACK.md`; `POST_LUNA_GPT6_HANDOFF.md` | Protected principles and future execution map. |
| Act II route gate | `scripts/act2_expansion_gate.ps1`; `tools/replay/act2_records_route_probe.txt` | Dedicated Records route proves Dispatch, terminal, operator and Concourse backtrack without weakening the Chapter One gate. |

## Visual evidence

```text
CURRENT_CONTACT_SHEET = out/creative_review_final_visual01/contact.png
CURRENT_B1_FRAME = out/creative_review_final_visual01/b1-final/frame.png
CURRENT_SECURITY_FRAME = out/creative_review_final_visual01/security-final/frame.png
CURRENT_ELEVATOR_FRAME = out/creative_review_final_visual01/elevator-final/frame.png
CURRENT_ACT2_CONCOURSE_FRAME = out/creative_review_final_visual01/concourse-final/frame.png
CURRENT_FACE_SHEET = out/creative_review_face12_art/human_sheet.png
CURRENT_FACE_ZOOM = out/creative_review_face12_art/human_face_zoom.png
CURRENT_WEAPON_SHEET = out/act2_art_review_weapon07/weapon_sheet.png
CURRENT_DOOR_CONTACT = out/creative_review_doors_final/contact.png
```

Visual observations from the current production render:

- B1 has a recovery bed/monitor/service group and a readable central route.
- Security has a checkpoint desk, equipment rack, wall-supported doors and a
  guard identity; the guard remains at the original gameplay-safe patrol
  position after a failed visibility experiment.
- Elevator has the strongest architectural threshold: frame, panel, hazard
  columns, control and waiting path.
- Concourse has dispatch equipment, side groupings and a clear transfer lane.
- Pistol and Stunner have distinct body masses, trigger/emitter areas, grips,
  hands and forearms.
- Full Human face is an authored stylized low-resolution face with the black
  eye-band issue removed; close subjective acceptance is still not claimed.

## Executed checks and results

| Check | Command/evidence | Result |
|---|---|---|
| Content compile | `python tools/contentc/contentc.py --data-dir data --out-dir data` | PASS, 11 rooms / 41 facts / 19 storylets / 10 NPCs |
| Content determinism | same command with `--check` | PASS, deterministic recompile matches |
| Systemic seed | `python tools/systemic/compile_systemic_seed.py ...` | PASS, 958-byte seed |
| Debug build | `pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset debug` | PASS |
| Release build | same command with `-Preset release` | PASS |
| Debug CTest | `ctest --preset debug --output-on-failure` | PASS, 2/2 targets |
| Release CTest | `ctest --test-dir out/build/release -C Release --output-on-failure` | PASS, 2/2 targets |
| Unit harness | executable output from both configurations | PASS, 215 tests / 0 failed |
| Contract | `scripts/contract_check.ps1` | PASS: forbidden, dependencies, public headers |
| Content tests | `python tools/contentc/test_contentc.py` | PASS, 13/13 |
| Systemic schema | `python tools/systemic/test_systemic_schema.py` | PASS, 10/10 |
| Invalid seed | `python tools/systemic/test_runtime_invalid_seed.py` | PASS |
| Static audit | `python tools/audit/static_audit.py` | PASS, COUNT=0 |
| Mandatory replay | `docs/production/evidence/overnight_expansion_recovery02` | PASS, 21/21 |
| Scenario matrix | `docs/production/evidence/overnight_expansion_scenario01` | PASS, 36 total / 35 executed / 1 rule-invalid / 0 coverage gaps |
| Visual smoke | `out/creative_review_final_visual01` | PASS, four production frames, exit 0 |
| Act II expansion route | `pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/act2_expansion_gate.ps1 -EvidenceDirectory docs/production/evidence/act2_expansion_route01` | PASS, Concourse -> Records -> terminal + operator -> Concourse; save/load and no-death assertions passed |

## Preserved negative evidence

`docs/production/evidence/overnight_expansion_recovery01` contains the first
full replay attempt. It stopped at `chapter01_aggressive` because the temporary
Security guard visibility move caused premature lethal exposure. The root cause
was corrected by restoring the original gameplay-safe guard spawn/patrol while
keeping the room equipment/door composition. The replacement run in
`overnight_expansion_recovery02` passed all 21 cases. The negative directory is
not a release receipt and must not be deleted.

The dedicated Act II route has a separate receipt because it intentionally
continues after Chapter One. Its generic application receipt does not claim
`CHAPTER_CHECKPOINT_REACHED`; the dedicated gate instead checks the exact
Act II route, interaction subtitles, save/load, no death and the final return
to the Concourse.

## Pending gates

```text
PACKAGE_SMOKE = PASS; dist/overnight_expansion_package03/WRITEOVER-07-v0.1.0-pvs01-gold-win-x64.zip; PACKAGE_COMMIT=c760abd8c25dacd2b82f25e83ccffc2b1a4939ea; SIZE=531628
RELEASE_BENCHMARK = PASS; release; PVS_RENDER_TIME_MS=0.711; PVS_TOTAL_FRAME_TIME_MS=1.292; OVERALL_BUDGET=PASS
ACT2_DEDICATED_REPLAY_FIXTURES = PASS_RECORDS_ROUTE; POWER_TRANSIT_OBSERVATION_FOCUSED_FIXTURES_OPEN
FOREGROUND_TERMINAL_FACE_ACCEPTANCE = OPEN_SUBJECTIVE
EXACT_PUSHED_HEAD_CI = PASS; run=34790805997; head=a1feb5c160eb91ba0980f5d156ee3cac2550f698
PUBLIC_ALPHA_RELEASE = NOT_AUTHORIZED
```

Do not upgrade any pending gate from its explicit state based on local unit
tests or a source-only screenshot.
