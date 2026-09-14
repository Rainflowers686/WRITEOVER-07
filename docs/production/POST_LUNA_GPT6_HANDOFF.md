# Post-expansion completion handoff map

Date: 2026-09-14  
HANDOFF_STATUS = `NOT_YET_AUTHORIZED`
CREATIVE_PASS_STATUS = `CREATIVE_BASELINE_FROZEN_CI_GATE_UNRESOLVED`
BASELINE_HEAD = `0123a4c` (`feat: finalize chapter one visual baseline`)

This document is an actionable transfer map, but it is not a transfer signal
yet. The high-value creative closure is complete; the final delivery head is
not eligible for handoff while the macOS Release benchmark gate remains
unresolved. The current owner remains responsible for the project.

## BASELINE_HEAD

Commit `0123a4c` preserves ancestor `5434f8a`, adds the five Act II-A
rooms, synchronizes the authored weapon states, adds the bounded face/room
presentation pass, and records the door-frame public enum in ADR-0012. The
current source state also contains the face33 production retouch: cheek/temple
planes, sparse horizontal eye marks, tapered lower contour and no hard side
frame. It keeps the Full Human/Maintenance face on the authored continuous
skin plane routed through the existing `OpaqueEmpty` CharCell path.

## PROTECTED_CREATIVE_BASELINE

`POST_ASTRA_CREATIVE_BASELINE / CHAPTER01_AUTHORED_FPS_20260913` remains the
protected Chapter One contract. The current expansion layer is
`ACT2_AUTHORED_SPACES_23EADDB + AUTHORED_FACE_PLANES_B221E37`.

Protect these decisions:

- Character-Art/Unicode remains the world unit. No RGB, half-block, Braille,
  image quantization or pixel-framebuffer rewrite.
- Glyph structure remains primary; color adds material, light and state but
  may not replace object silhouettes.
- Security is a broad armored identity; Full Human has restrained hair/face,
  shoulders, ribcage, pelvis and weight-bearing legs; Maintenance roles use
  practical equipment and distinct voices.
- Front, Back, SideLeft and SideRight are separate authored poses; yaw,
  viewer relation, selected enum, asset and inspection label must agree.
- Pistol and Stunner are different authored masses with body/slide or emitter,
  grip, trigger suggestion, hand, wrist and forearm.
- The active pistol/stunner viewmodel hand is a cool tactical glove with a
  continuous palm-to-grip silhouette; all idle/fire/reload states use the same
  authored attachment relationship.
- Doors are wall-supported frame + panel + inset/signage layers with shared
  depth, not floating camera sprites.
- B1, Security and Elevator keep their functional focal groups and clear route.
  Act II rooms use large equipment groupings before small texture.
- HUD is compact and literal. Narrator is an exact records supervisor who
  uses silence; Security, Cleaner, Technician and Medical have distinct voices.
- Quiet play is observation/discretion; aggressive play changes the record and
  creates pressure. Consequences are facts/events/knowledge, not a morality UI.
- Opening NPC interaction is role/faction-resolved. Missing authored text is
  silence; it must never fall through to Security's voice.

## DO_NOT_REGRESS

- Do not reset, clean, restore, stash, rebase, force-push, branch, tag, make a
  release, upload to Steam, or replace main with a copied repository.
- Do not stage `tests/test_harness.cpp` unless the owner separately requests
  its unrelated newline change.
- Do not add historical evidence directories, settings files or user data to
  a broad commit.
- Do not move the Security guard merely to improve a screenshot without
  rerunning the aggressive route; the previous visibility experiment caused a
  real player death and is preserved as `overnight_expansion_recovery01`.
- Do not turn a missing package, foreground terminal, audio or CI result into
  PASS by inference.

## CURRENT_PLAYER_EXPERIENCE

Chapter One: wake/medical intrigue -> credential obstacle -> body/camera
consequence -> Medical/Staff discretion or Security pressure -> framed
Elevator departure. The opening now shows a grouped B1 recovery/monitor axis,
functional doors and a grounded first-person pistol.

Act II-A: the elevator opens to a dispatch-focused Concourse. The player can
review Records, help or bypass Power, learn the Observation blind spot and
resolve Transit Control. Each route can be quieter or more confrontational;
backtracking and existing recoverable failure remain available.

Visual status: authored low-resolution FPS density is materially improved in
the current production frames. The latest real release capture is
`docs/production/evidence/chapter01_creative_polish/face_retouch33/`; it uses
a continuous skin plane with cheek shadow and sparse facial glyphs, removing
the old punctuation texture and hard frame that made the face read like a
mask. `room_structure_retouch04/` is the current B1/Security/Elevator room
baseline. This is a bounded stylized acceptance, not a photorealism claim.

## CURRENT_MAJOR_SYSTEMS

- Fixed 120 Hz simulation with separate presentation cadence.
- Height-span DDA, shared camera projection, finite floor/ceiling planes and
  character depth/occlusion.
- Authored directional LOD Character-Art and first-person weapon viewmodels.
- Semantic `^` authored skin plane through existing `OpaqueEmpty` cells; no
  new public renderer type and no framebuffer path.
- Ray/visibility-based interaction with nearest-target ordering.
- Deterministic NPC perception, memory, intent and motor updates.
- Body, credential, camera, relationship and Act II route facts.
- Scene-scoped storylets and speaker-resolved dialogue.
- Transactional save/load and recoverable room/checkpoint restart.
- Deterministic JSON/text -> `.woc`/`.bin` content pipeline.
- Windows/POSIX platform seams; Windows procedural audio and POSIX subtitle
  fallback remain separate capabilities.

## OPEN_FATAL

`none known` after the current local content/build/test/replay checks.

## OPEN_P0

`none known` after the current local checks.

## OPEN_P1

- No known player-critical P1 remains in the Chapter One art pass after the
  face33 production render, the synchronized weapon render, and the room04
  structure render. The face remains intentionally stylized and must not be
  described as photorealistic; any future subjective objection should first
  be tested against the protected CharCell contract and revised as an asset,
  not by changing renderer paradigm.
- Act II has production smoke renders and one dedicated Records route gate
  (Dispatch -> terminal -> operator -> Concourse backtrack). A focused
  first-time human playthrough and focused Power/Observation/Transit fixtures
  remain.
- Local package smoke and the post-face Release benchmark pass locally
  (`PVS_RENDER_TIME_MS=1.237`, `PVS_TOTAL_FRAME_TIME_MS=2.169`). The exact
  creative source head `52cc29f112ff7db2cfabd1a2951b48d4e5c2e568` is PASS
  in CI run `34804411002` after one controlled macOS benchmark rerun: macOS
  `TERMINAL_FULL_TIME_MS=0.882`, `PVS_RENDER_TIME_MS=1.293`, and
  `PVS_TOTAL_FRAME_TIME_MS=4.642`, all within their gates. The initial
  macOS attempt (`3.689 ms` full-terminal time) remains historical evidence
  of platform variance, not a source change or a silently relabeled PASS.
- The later documentation delivery head
  `6ddbf2678cc1d602a9b24749a135a7247c6f5ff1` is not green in CI run
  `34805619450`: two macOS attempts failed different timing gates
  (`DELTA=1.657 ms` plus `UNCHANGED=0.270 ms`; then `FULL=2.018 ms` and
  `PVS_RENDER=12.963 ms`). Linux, clang, ARM64 link, the main build,
  recovery replay, package smoke, and all non-platform gates passed. This is
  an environment/performance-gate blocker, not a player-facing art defect;
  do not mark the handoff READY until a fresh final head gets a real PASS.

## REMAINING_OBJECTIVE_WORK

Run a first-time Act II-A read test: can a player tell that Dispatch is the
hub, identify the functional object in each room, understand the current
objective and recognize the checkpoint without a guide? Fix only observed
blockers.

## REMAINING_GAMEPLAY_WORK

Add or execute focused Act II replay fixtures for Power assistance versus
forced reroute, Observation camera loop, Transit bypass/control/alert,
backtracking and checkpoint recovery. The Records route is already covered by
`scripts/act2_expansion_gate.ps1`; preserve it while adding the remaining
fixtures.
Preserve old Chapter One fixtures and rerun the entire affected suite after
each state/interaction change.

## REMAINING_CONTENT_WORK

Do a bounded line edit for any first-time confusion, confirm each Act II NPC
has one memorable practical line, and verify room focal groups in real
terminal-sized captures. Do not add lore chapters, random wall texture or a
new character class.

## REMAINING_TEST_WORK

Refresh Debug/Release builds and tests, content deterministic check, systemic
schema/invalid-seed tests, mandatory replays, the Act II expansion gate,
scenario matrix, save-fault regression, art review and relevant render
assertions. The current local Release benchmark and package smoke are already
recorded as PASS; rerun them if source/content changes.
Record any later command and result in the audit manifest, then observe the
exact pushed-head CI whenever source or package content changes.

## REMAINING_PACKAGE_WORK

The current Release package was built from `52cc29f112ff7db2cfabd1a2951b48d4e5c2e568`
and passed clean-package smoke, executable-relative data, manifest/hash,
secret-scan and user-data-separation checks. Rebuild only if source/content
changes; no public release is implied.

## REMAINING_COURSE_REPORT_WORK

Read the actual syllabus and assemble only the required report, PPT, WBS/UML,
responsibility and submission artifacts. Explain the CharCell renderer,
height-span projection, systemic facts, storylets, save transaction and
content compiler using the teach-back anchors in
`OVERNIGHT_EXPANSION_TEACHBACK.md`.

## REMAINING_DEMO_WORK

Prepare one quiet route, one aggressive consequence route and one Act II
interaction route using separate saves. Demonstrate doors, NPC identity,
weapon distinction, a durable fact and the Elevator/Transit payoff. Label a
scripted replay as scripted; do not call it a classmate playtest.

## SAFE_FOR_LUNA

Only after `HANDOFF_STATUS` changes: routine content wording, focused test
expansion, packaging repetition, report/PPT/WBS/UML assembly, demo rehearsal,
and ordinary bug cleanup that preserves the protected contract.

## LUNA_SHOULD_NOT_REDESIGN

Do not redesign authored anatomy, directional conventions, pistol/stunner
composition, room zoning, door depth, palette/HUD hierarchy, narrator/NPC
voices, event-vs-knowledge-vs-fact authority, fixed tick, save transaction,
public binary schemas, Character-Art renderer paradigm or module boundaries.

## LIKELY_COMPLETION_SEQUENCE

1. Treat the face33/synchronized-weapon/room04 visuals as the protected
   Chapter One baseline and close only an observed first-time Act II read
   blocker.
2. Add focused Power/Observation/Transit route fixtures and rerun Chapter One
   regression plus the existing Records expansion gate.
3. Refresh Debug/Release, package smoke and benchmark.
4. Keep this file's `BASELINE_HEAD` on the deliberate creative source
   baseline; update it only when the protected source baseline changes.
5. Commit documentation/package metadata coherently, push main normally, and
   watch the exact final-head CI run.
6. Only change the status to `READY_FOR_LUNA_FINAL_COMPLETION` after a fresh
   final head gets an exact-head CI PASS; never call it product gold, visual
   gold, course final or public release ready.

## PRESENTER_CRITICAL_KNOWLEDGE

| Topic | Presenter must understand |
|---|---|
| Architecture/game loop | `composition_root.cpp` wires modules; fixed simulation time is not render time. |
| Renderer/raycasting | DDA records finite height transitions; camera projection maps wall, plane and actor cells to the terminal. |
| Character-Art | `CharacterArtBank` loads authored opacity/facing/LOD rows; negative space and `~` semantics are not image pixels. |
| Interaction | Camera ray + bounds + LOS selects a target; facts/capabilities authorize the action. |
| AI | Perception, memory, intent and motor are separate; a body consequence needs the right NPC knowledge. |
| Systemic facts | Event, observer knowledge and durable consequence are intentionally distinct. |
| Narrative/storylets | Eligibility, priority and scene flags choose lines; speaker resolution is real, and silence is valid output. |
| Save/load | Staged sections validate before commit; injected failures must preserve the previous authoritative state. |
| Content pipeline | JSON/text compiles deterministically; `contentc --check` compares generated bytes. |
| Testing | Unit, replay, scenario, render, visual, package and CI evidence answer different questions. |
| Cross-platform | Windows terminal/audio runtime and POSIX subtitle/runtime evidence must be reported separately. |

For every major change, use the same classroom template:

```text
IMPORTANT_FILES = ...
IMPORTANT_CLASSES = ...
IMPORTANT_FUNCTIONS = ...
WHY_THIS_DESIGN = ...
HOW_TO_EXPLAIN_IT_IN_CLASS = ...
```

## CURRENT_CHANGE_TEACHBACK

IMPORTANT_FILES =
`data/characters/b1_character_art.txt`,
`src/render/character_renderer.cpp`,
`src/app/composition_root.cpp`,
`data/scenes/recovery_scene.json`,
`data/scenes/recovery_scene.bin`,
`docs/production/CHAPTER01_CREATIVE_DIRECTION.md`.

IMPORTANT_CLASSES = `CharacterArtAsset`, `CharacterArtBank`,
`CharacterCellOpacity`, the B1 interaction path owned by `RunComposition`,
and the existing scene prop/door projection types.

IMPORTANT_FUNCTIONS = `DecodeArtRow`, `SpriteBackground`, `SpriteForeground`,
`DrawOneSprite`, the B1 role-resolved subtitle branch in
`src/app/composition_root.cpp`, and the existing scene content compiler.

WHY_THIS_DESIGN = keep the world Character-Art/CharCell based while giving
faces a continuous authored material plane, weapons a readable body-to-hand
mass, and rooms a functional equipment grouping. Silence on an absent line is
safer than borrowing Security's voice.

HOW_TO_EXPLAIN_IT_IN_CLASS = explain that the renderer still projects authored
Unicode cells and opacity; `^`/`&` only label occupied skin material inside
that same cell path. The face, weapon, door and room changes are content and
bounded presentation corrections, not a framebuffer rewrite. The replay proof
then shows that the narrative speaker is selected from role/faction rather
than a generic fallback.
