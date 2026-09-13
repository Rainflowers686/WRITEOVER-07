# Overnight expansion teach-back

Date: 2026-09-14  
Source/content baseline: `23eaddbe1a2476519fc91dd66d5dfefef069e512`
Scope: Act II-A authored spaces, visual baseline recovery, and the knowledge
needed to explain the work in class.

This is a teaching map, not a promise that the project is product gold or
course-delivery complete. It explains where the new work lives, why it is
structured this way, and what a later maintainer must verify before changing
it.

## One-minute architecture explanation

The game is a C++17 fixed-tick first-person simulation whose output is a
terminal-sized `CharCell` field. The composition root wires the existing
world, player, AI, systemic, narrative, renderer, audio and platform modules.
Authoring JSON/text is compiled deterministically into `.woc`/`.bin` files;
runtime code consumes those compiled files. The player camera uses the same
world geometry for raycasting, interaction and character projection.

The critical distinction is:

- a glyph is the visual unit;
- color is semantic material support;
- a scene entity is not automatically an interaction;
- an event, what an NPC knows, and a durable fact are different states;
- a save is staged and validated before it replaces authoritative state.

The current extension adds rooms, actors, facts, storylets and scene entities
through those existing seams. It does not add a new renderer, ECS, squad
framework, animation system, save field or framebuffer path.

## Act II-A route model

After the Chapter One elevator checkpoint, the player enters the Service
Concourse. Dispatch review opens the authored expansion. Records and Power are
alternative work routes; Records can lead to Observation, Power can lead to a
maintenance bypass into Transit, and Transit resolves the checkpoint route.
Every room can return to a previous room where the scene transition allows it.

| Room | First question for the player | Interaction proof | Delayed proof |
|---|---|---|---|
| Service Concourse | Which transfer is actually filed? | Dispatch terminal and four framed doors | The chosen route changes which next room is available |
| Records Archive | Can a person release the record? | Operator conversation or credentialed terminal | Archive facts alter later access and observation routing |
| Power Utility | Can the bypass be made safe? | Technician, relay and backup controls | Forced reroute creates utility noise for Transit |
| Transit Control | Who controls the checkpoint? | Camera loop, response guard and control terminal | Alert/bypass/control facts determine checkpoint state |
| Observation Gallery | What does the camera fail to see? | Analyst and monitor wall | A camera blind spot can support a later bypass |

The player is never required to solve a room by a single hidden verb. Each
room has a broad interaction style: observe, speak, use a terminal, take a
quiet route, or accept a louder systemic consequence. Failure is recoverable
through backtracking and the existing save/checkpoint rules.

## Important changes: classroom anchors

Every row supplies the required teach-back fields.

| Change | IMPORTANT_FILES | IMPORTANT_CLASSES | IMPORTANT_FUNCTIONS | WHY_THIS_DESIGN | HOW_TO_EXPLAIN_IT_IN_CLASS |
|---|---|---|---|---|---|
| Authored Act II rooms | `data/rooms/room_act2_*.json`; `data/scenes/recovery_scene.json`; compiled `.woc`/`.bin` | `SceneRuntime`; `RenderModule` | `SceneRuntime::Load`; `SceneRuntime::FindEntity`; `RenderModule::Render` | Large functional groups establish purpose before texture and keep navigation readable. | “A room is a spatial contract: grid, materials, focal equipment, NPC anchors and transitions are authored together, then compiled.” |
| Storylet and fact expansion | `data/facts/facts.json`; `data/storylets/storylets.json`; `data/text/recovery_text.txt` | `NarrativeModule`; storylet/fact runtime types | `NarrativeModule::SetActiveScene`; `NarrativeModule::SimTick`; `SetSubtitleOnce`; `SliceRuntime::use_act2_terminal` | Consequences must be stateful and speaker-specific, not a wall of exposition. | “The storylet chooses an eligible statement; the interaction records facts; later rooms read those facts.” |
| Four Act II NPCs | `data/npcs/npcs.json`; `data/systemic/systemic_seed.json`; `data/characters/b1_character_art.txt` | `AutonomousNpcSystem`; `CharacterArtBank` | NPC profile binding in `composition_root.cpp`; `CharacterArtBank::Find`; `SelectCharacterFacing` | Roles are readable from silhouette, context and voice, not from labels alone. | “AI state and visual identity are separate data, joined by stable content IDs.” |
| Door architecture | `data/scenes/recovery_scene.json`; `data/characters/b1_character_art.txt`; `include/writeover/render/character_renderer.h` | `CharacterArtBank`; character renderer | `DrawDoorPlane`; `ResolveFacingCell`; `CharacterSpriteKind::DoorFrame` | A frame and panel share a wall plane and depth; the door is architecture, not a floating sprite. | “The frame is a visual-only layer; it has no interaction or save semantics.” |
| Character proportions and directions | `data/characters/b1_character_art.txt`; `tests/test_render.cpp`; `tools/art_review/main.cpp` | `CharacterArtAsset`; `CharacterArtBank` | `SelectCharacterFacing`; `SpatialActorFacingOrbit`; `DrawOneSprite` | Front/back/left/right are authored poses with changed overlap and equipment. | “We validate actor yaw, camera relation, enum, asset and inspection label together.” |
| Pistol/Stunner viewmodels | `data/characters/b1_character_art.txt`; `src/render/character_renderer.cpp` | weapon renderer path and `CharacterArtBank` | `FindPistol`; `DrawWeaponViewmodel` | Big body mass, trigger area, grip, hand and forearm make the first-person object believable. | “Weapon identity is a silhouette and anchoring problem before it is a texture problem.” |
| Opening composition | `src/app/composition_root.cpp`; `src/app/game_main.cpp`; `src/render/character_renderer.cpp` | `SliceRuntime`; `RenderModule` | room spawn/switch pitch setup; `SurfaceColor`; `CeilingMaterial`; `CeilingLight` | A small downward opening pitch and continuous ceiling material make functional space readable without changing the engine paradigm. | “Camera presentation changes what the player reads first; it does not grant gameplay authority.” |
| Public interface record | `docs/adr/ADR-0012-authored-door-frame-layer.md`; `tools/contract_check/.contract_baseline.json` | public render enum | `CharacterSpriteKind::DoorFrame` | The one public enum addition is explicit, reviewable and bounded. | “A public header change requires an ADR and a refreshed hash; silent drift is a contract failure.” |
| Act II route evidence | `scripts/act2_expansion_gate.ps1`; `tools/replay/act2_records_route_probe.txt` | replay gate | `act2_expansion_gate.ps1` | A dedicated gate proves one real social/technical route without weakening the Chapter One closure receipt. | “A route that intentionally continues past an earlier checkpoint needs its own success contract.” |
| Regression evidence | `scripts/recovery_replay_gate.ps1`; `scripts/chapter01_scenario_matrix.ps1`; `docs/production/evidence/overnight_expansion_*` | replay/scenario harnesses | script entry points and production app CLI | Visual, route, save and package evidence answer different questions. | “215 unit assertions passing does not prove a classmate can read the room.” |

## Character-Art explanation

`CharacterArtBank` loads authored Unicode rows with opacity and distance/facing
metadata. Transparent spaces are real negative space; an occupied blank is not
the same as transparent air. The renderer projects individual cells through
the world camera and depth buffer. It does not sample an image or quantize a
framebuffer.

The current art rules are:

- Security reads as helmet/visor, shoulder armor, chest/waist equipment and
  planted legs.
- Full Human reads as hair boundary, head angle, restrained face plane,
  shoulder/ribcage/pelvis and relaxed weight-bearing legs.
- Maintenance family reads as cap, asymmetric strap, working arms, tool pocket
  and practical stance; Cleaner and Technician are separated by context and
  dialogue.
- Back views expose nape, shoulder/back equipment and coat seam; side views
  change chest width, arm placement, leg overlap and equipment position.
- The current near face fixes the black eye-band failure and avoids huge eyes,
  emoji smiles and nostril punctuation. It remains stylized low-resolution
  authored art; subjective terminal acceptance is still a human check.

## Safe operating sequence for the next maintainer

1. Edit source JSON/text or authored glyph rows.
2. Run the content compiler and deterministic `--check` sequentially.
3. Build the affected preset.
4. Run the relevant unit/content/replay checks.
5. Capture a real production frame at the same terminal size.
6. Critique silhouette, focal point, depth and route readability.
7. Only then stage the intended files; never broad-add evidence/settings.

Useful commands from the repository root:

```powershell
python tools/contentc/contentc.py --data-dir data --out-dir data
python tools/contentc/contentc.py --data-dir data --out-dir data --check
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset debug
out/build/debug/Debug/writeover_app.exe --smoke --frames 1 --width 240 --height 67 --data-dir data --room room_b1_revival --user-data-dir out/manual --dump-frame out/manual/frame.svg
out/build/debug/Debug/writeover_art_review.exe data/characters/b1_character_art.txt out/manual/art_review
```

The current acceptance sequence is AUTHOR -> REAL PRODUCTION RENDER ->
CRITIQUE -> REVISE -> REAL PRODUCTION RENDER. Source rows alone are not
visual acceptance.

## Boundaries to explain honestly

- `READY_FOR_LUNA_FINAL_COMPLETION` is not currently asserted by this file;
  the future handoff is explicitly `NOT_YET_AUTHORIZED`.
- The current Chapter One replay and scenario suites pass, and the dedicated
  Records Act II route gate passes; neither constitutes a first-time Act II
  classmate playtest or proves the Power/Observation/Transit branches.
- The first attempted full replay after the guard visibility experiment is
  retained as a failure diagnosis; the corrected full replay is the authority.
- Package smoke, release benchmark and exact pushed-head CI must be refreshed
  after the final documentation commit.
