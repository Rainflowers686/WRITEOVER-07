# Post-expansion completion handoff map

Date: 2026-09-14  
HANDOFF_STATUS = `NOT_YET_AUTHORIZED`  
CREATIVE_PASS_STATUS = `GPT6_CRITICAL_PATH_CONTINUES`  
BASELINE_HEAD = `23eaddbe1a2476519fc91dd66d5dfefef069e512`

This document is intentionally actionable but is not a transfer signal yet.
The owner explicitly asked the current creative pass to continue instead of
handing the project to Luna. It becomes an active handoff only after the
foreground visual boundary, Act II acceptance and exact pushed-head CI are
honestly closed.

## BASELINE_HEAD

`23eaddbe1a2476519fc91dd66d5dfefef069e512` is the source/content and
focused-route baseline.
It preserves ancestor `5434f8a`, adds the five Act II-A rooms, synchronizes
the authored weapon states, adds the bounded face/room presentation pass, and
records the door-frame public enum in ADR-0012.

## PROTECTED_CREATIVE_BASELINE

`POST_ASTRA_CREATIVE_BASELINE / CHAPTER01_AUTHORED_FPS_20260913` remains the
protected Chapter One contract. The current expansion layer is
`ACT2_AUTHORED_SPACES_23EADDB`.

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
- Doors are wall-supported frame + panel + inset/signage layers with shared
  depth, not floating camera sprites.
- B1, Security and Elevator keep their functional focal groups and clear route.
  Act II rooms use large equipment groupings before small texture.
- HUD is compact and literal. Narrator is an exact records supervisor who
  uses silence; Security, Cleaner, Technician and Medical have distinct voices.
- Quiet play is observation/discretion; aggressive play changes the record and
  creates pressure. Consequences are facts/events/knowledge, not a morality UI.

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
the current production frames. The face no longer has the black eye band or
exaggerated emoji features, but close foreground acceptance is still a
subjective boundary. This is not a photorealism claim.

## CURRENT_MAJOR_SYSTEMS

- Fixed 120 Hz simulation with separate presentation cadence.
- Height-span DDA, shared camera projection, finite floor/ceiling planes and
  character depth/occlusion.
- Authored directional LOD Character-Art and first-person weapon viewmodels.
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

- Foreground-terminal/human acceptance of the stylized Full Human face is not
  closed by an automated sheet. If Rain still reads it as uncanny at play
  distance, revise only the authored head silhouette/face plane first.
- Act II has production smoke renders and one dedicated Records route gate
  (Dispatch -> terminal -> operator -> Concourse backtrack). A focused
  first-time human playthrough and focused Power/Observation/Transit fixtures
  remain.
- Local package smoke and Release benchmark pass at the current documentation
  baseline. The pushed head `a1feb5c160eb91ba0980f5d156ee3cac2550f698` also
  has an exact-head CI PASS (run `34790805997`).

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
Record the exact command and result in the audit manifest.

## REMAINING_PACKAGE_WORK

The current Release package was built from `c760abd8c25dacd2b82f25e83ccffc2b1a4939ea`
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

1. Close or explicitly accept the foreground face and first-time Act II read.
2. Add focused Power/Observation/Transit route fixtures and rerun Chapter One
   regression plus the existing Records expansion gate.
3. Refresh Debug/Release, package smoke and benchmark.
4. Update this file's `BASELINE_HEAD` only if a deliberate source baseline
   changes; update status only after owner approval.
5. Commit documentation/package metadata coherently, push main normally, and
   watch the exact final-head CI run.
6. Only then consider `READY_FOR_LUNA_FINAL_COMPLETION`; never call it product
   gold, visual gold, course final or public release ready.

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
