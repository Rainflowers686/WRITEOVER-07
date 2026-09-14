# WRITEOVER-07 — Complete Game Presenter Teachback

PURPOSE = give Rain a compact, source-grounded explanation of the complete
bounded campaign and the systems a presenter must understand

BASELINE_HEAD = see POST_ULTRA_FINAL_HANDOFF.md for the current implementation.
Historical campaign head was a509e3fa6b9edcb0ff1392d4ae7eaa2de2a913d6;
the earlier long hash in this field was incorrect.

## One-minute explanation

WRITEOVER-07 is a first-person MUD/FPS hybrid whose world is rendered from
CharCell/Unicode glyphs. The player wakes in B1, makes route and violence
choices, crosses an Act II-A service hub, takes a bounded lift directory into
eight named destinations, and reaches a durable three-ending decision. The
interesting part is not a cinematic branch tree: facts about noise, bodies,
credentials, cooperation, force, and discovery remain in the existing
systemic/narrative state and change what later rooms and endings permit.

The campaign expansion is intentionally a private vertical policy in the
composition root. It reuses existing rooms, transitions, storylets, facts,
systemic events, save/load, and HUD sources instead of introducing a generic
campaign framework.

## Architecture teachback

### Startup and composition

IMPORTANT_FILES = src/app/game_main.cpp,
src/app/composition_root.cpp, src/app/composition_root.h

IMPORTANT_CLASSES = GameRuntime/CompositionRoot-facing module wiring,
SliceRuntime, PlayerModule, RenderModule

IMPORTANT_FUNCTIONS = composition setup, SliceRuntime::SimTick, room switch,
interaction callback setup, transition predicate, replay diagnostics

WHY_THIS_DESIGN = keep the existing course-scale executable composition
explicit and auditable; the new campaign rules can be inspected in one bounded
policy object without changing every engine subsystem.

HOW_TO_EXPLAIN_IT_IN_CLASS = startup creates services/modules, the composition
root wires their callbacks, then each fixed simulation tick updates world,
AI, narrative, player, and render-facing state in a known order.

### Game loop and input

IMPORTANT_FILES = src/core/engine.cpp, src/player/input_runtime.cpp,
src/player/input_mapper.cpp, src/platform/windows/win_input.cpp,
src/platform/windows/win_raw_input.cpp

IMPORTANT_CLASSES = Engine, InputRuntime, InputMapper, PlayerController

IMPORTANT_FUNCTIONS = fixed-step update, input batch processing, context
switching, raw mouse delta handling, PlayerModule::SimTick

WHY_THIS_DESIGN = simulation remains deterministic and render presentation is
separate from the fixed step; this is why replay and benchmark results can be
compared.

HOW_TO_EXPLAIN_IT_IN_CLASS = keyboard/mouse events enter the platform backend,
the mapper turns them into context actions, PlayerModule consumes them, and
the engine advances one fixed simulation step before presenting.

### World, rooms, raycasting, and renderer

IMPORTANT_FILES = src/app/scene_runtime.cpp,
src/app/interaction_runtime.cpp, src/world/room.cpp,
src/render/raycaster.cpp, src/render/production_renderer.cpp,
src/render/character_renderer.cpp, src/render/hud.cpp

IMPORTANT_CLASSES = SceneRuntime, InteractionRuntime, Room/room codec,
Raycaster, ProductionRenderer, CharacterRenderer, HUD

IMPORTANT_FUNCTIONS = room load/switch, bounded transition test, interaction
ray, column raycast, production frame composition, CharacterFacing selection,
HUD objective/subtitle presentation

WHY_THIS_DESIGN = geometry remains a room grid plus authored world entities;
CharCell/Unicode glyphs carry shape while color carries material/light semantics.
The campaign did not replace this with RGB, half-block, Braille, or image
quantization.

HOW_TO_EXPLAIN_IT_IN_CLASS = the raycaster finds wall/floor/ceiling geometry,
the production renderer emits terminal cells, and character/prop/door layers
are authored assets projected into those cells.

### Character-Art and weapon identity

IMPORTANT_FILES = src/render/character_renderer.cpp,
src/player/combat.cpp, data/characters/b1_character_art.txt,
existing art-polish evidence under
docs/production/evidence/chapter01_creative_polish/

IMPORTANT_CLASSES = CharacterRenderer, Combat/weapon state, authored character
asset structures

IMPORTANT_FUNCTIONS = facing/LOD selection, actor projection, weapon-slot
selection, hitscan/fire, stun/nonlethal application

WHY_THIS_DESIGN = important NPCs and Pistol/Stunner remain authored
Character-Art with silhouette, pose, facing, negative space, and semantic
color. Visual pass 07 adds a larger held-object mass and a restrained human
front face while preserving the existing four-way bank and character-based
renderer.

HOW_TO_EXPLAIN_IT_IN_CLASS = color is a material/depth aid; removing color
must not erase the core body, weapon, door, or room silhouette.

### Interaction and scene transitions

IMPORTANT_FILES = src/app/interaction_runtime.cpp,
src/app/composition_root.cpp, data/scenes/recovery_scene.json

IMPORTANT_CLASSES = InteractionRuntime, SliceRuntime transition/interaction
closures

IMPORTANT_FUNCTIONS = prompt source, interaction dispatch, transition_allowed,
switch_room, terminal/door/reader handlers

WHY_THIS_DESIGN = one existing interaction path keeps terminals, doors,
readers, NPC prompts, and bounded upper-room actions consistent with replay.

HOW_TO_EXPLAIN_IT_IN_CLASS = the player aims at an authored entity; the
interaction callback checks the current room and fact state, writes facts or
calls switch_room, and the next HUD frame reflects the new objective.

### AI

IMPORTANT_FILES = src/app/modules/ai_module.cpp,
src/ai/npc.cpp, src/ai/npc_profiles.cpp, src/ai/perception.cpp,
src/ai/goap.cpp, src/systemic/systemic.cpp

IMPORTANT_CLASSES = AiModule, NPC, NPC profile, perception/GOAP types,
SystemicWorld

IMPORTANT_FUNCTIONS = perception update, noise stimulus, investigation,
inspection, combat cadence, patrol motor, memory recall, systemic event bridge

WHY_THIS_DESIGN = the project uses bounded stateful NPC behavior, not a claim
of full tactical multi-room AI. The campaign only adds a narrow quiet Transit
stimulus gate and uses existing shooting/alert behavior for escalation.

HOW_TO_EXPLAIN_IT_IN_CLASS = NPCs perceive sight/noise, enter bounded states,
write memories/events, and can resume patrol after inspection; facts are the
long-lived bridge to narrative and later rooms.

### Facts, systemic consequences, and narrative

IMPORTANT_FILES = src/world/fact_belief.cpp,
src/systemic/systemic.cpp, src/narrative/storylet.cpp,
src/narrative/dialog.cpp, src/app/modules/narrative_module.cpp,
data/facts/facts.json, data/storylets/storylets.json,
data/text/recovery_text.txt

IMPORTANT_CLASSES = FactBelief/FactStore, SystemicWorld,
StoryletScheduler, DialogQueue, NarrativeModule, TowerCampaignRuntime

IMPORTANT_FUNCTIONS = fact read/write, systemic event application,
storylet eligibility/priority, dialogue enqueue/expiry, CaseFile,
SelectableDestinations, EligibleEndings

WHY_THIS_DESIGN = durable facts express consequences directly and storylets
remain data-driven. TowerCampaignRuntime is a small policy for the bounded
directory/endings, not a second state engine.

HOW_TO_EXPLAIN_IT_IN_CLASS = a shot/noise/body/credential action becomes a
fact or systemic event; later room access, NPC response, HUD objective, and
ending eligibility read that same state.

### Save/load

IMPORTANT_FILES = src/core/save.cpp and save headers, runtime save/load code
in src/app/composition_root.cpp, existing save/replay tests

IMPORTANT_CLASSES = SaveManager/SaveCodec, runtime save state

IMPORTANT_FUNCTIONS = compose/parse, deterministic round trip, atomic
runtime save/load, checkpoint save request

WHY_THIS_DESIGN = the new campaign uses existing serialized facts, systemic
state, player/room state, and event/history sections. No schema section was
added for a speculative region engine.

HOW_TO_EXPLAIN_IT_IN_CLASS = the save format validates bounded sections and
rejects corruption; campaign unlock/ending facts are just durable state read
after reload, which is why both complete route probes verify SAVE_OK and
LOAD_OK.

### Content compiler and runtime loading

IMPORTANT_FILES = tools/contentc/contentc.py,
tools/contentc/test_contentc.py, data/rooms/*.json,
data/scenes/recovery_scene.json, data/npcs/npcs.json,
data/facts/facts.json, data/storylets/storylets.json

IMPORTANT_CLASSES = content compiler codecs/validators and runtime content
loaders

IMPORTANT_FUNCTIONS = compile/check, schema validation, room/fact/storylet/NPC
load, deterministic output check

WHY_THIS_DESIGN = JSON is reviewable authoring input; .woc/.bin are deterministic
runtime artifacts. A content change is therefore visible in both source and
compiled output.

HOW_TO_EXPLAIN_IT_IN_CLASS = author JSON, run contentc --check, run schema
tests, then the C++ runtime loads compiled content; never hand-edit only the
binary.

### Testing and cross-platform

IMPORTANT_FILES = tests/*.cpp, tools/replay/*.txt,
scripts/complete_game_campaign_gate.ps1,
tools/release/package_smoke.py, scripts/contract_check.ps1,
.github/workflows/*

IMPORTANT_CLASSES = TestHarness and test registration, replay runner,
benchmark scenarios

IMPORTANT_FUNCTIONS = writeover_tests main, replay assertions, content checks,
save-fault checks, benchmark gates, package smoke

WHY_THIS_DESIGN = tests cover deterministic mechanics and contracts while
replays cover route integration. They still do not replace a foreground
Windows terminal, audio listening, or first-time classmate playtest.

HOW_TO_EXPLAIN_IT_IN_CLASS = unit tests prove local contracts, a replay proves
an authored path, CI proves supported build/platform combinations, and manual
acceptance proves the actual player experience.

## Presenter facts to remember

- Current compiled content reports 19 rooms, 69 facts, 27 storylets, and
  17 NPC profiles.
- The current runtime campaign probe instantiates 16 NPCs; the data profile
  count is not the same as the runtime count.
- The scene graph has 32 transitions and no explicit region objects.
- The new directory has eight destinations: 1F, 8F, 12F, 18F, 24F, 30F, 36F,
  and Roof.
- Amend is the baseline ending; Disclose requires network discovery plus
  cooperative Operations; Breach requires the upper security/force trace.
  Four campaign probes and `complete_game_campaign_gate.ps1` assert the
  selected ending fact and end-screen readiness, not just Roof reachability.
- `act2_route_coverage_gate.ps1` covers the Power, Observation, and Transit
  branches independently; `package_negative_probe.py` proves required content,
  character-art, and recovery-text resources fail closed when removed.
- Act II-A remains a named existing route. The upper sequence is bounded
  application policy, not a generic Act system.
- No claim about 41 floors, B4, a public GitHub release, or visual gold should
  be made from this baseline.

## Final product-pass teaching addendum

Explain the separation first: gameplay owns what happened; the product layer
owns how an already-perceived event is shown. Displaying, hiding or forgetting
a cue cannot change facts, relationships, access or ending eligibility.

| IMPORTANT_FILES | IMPORTANT_CLASSES | IMPORTANT_FUNCTIONS | WHY_THIS_DESIGN | HOW_TO_EXPLAIN_IT_IN_CLASS |
|---|---|---|---|---|
| src/app/perception_feed.h; player_perception.h | PerceptionFeed; PlayerPerceptionObserver | Publish, Visible, History, Observe, PlayerCanPerceive | One bounded 48-entry presentation stream; same-room, FOV, range and LOS observations | We remember only what the player could perceive, then show two short cues or a scrollable history. |
| src/app/player_perception.h; composition_root.cpp | SystemicWorld; RenderModule | InspectVisibleTarget, PlayerKnownEvidence, case-file source | Existing focused targets and known_by records remain authoritative | Examine describes the visible body or equipment, not a secret inventory or the NPC's thoughts. |
| src/app/player_product.h; campaign_panel.h | PlayerProductRuntime; ProductInputLease | Handle, Rows, DrawCampaignPanel | Small private menu with bounded scrolling; raw input kept intact while held menu actions are fenced | A held mouse button must not become a shot just because a menu closed. Long text scrolls without hiding Back. |
| src/app/game_main.cpp; composition_root.cpp | Engine; application modules | main reconstruction loop; RunComposition | New Game destroys all runtime owners and constructs them from content again | Moving the player to B1 would leave old NPC memories. A new composition actually starts a new game. |
| src/app/product_save.h; composition_root.cpp | SaveManager; ProductSaveRole; RenderModule | ProductResumeName, save/load callbacks, ResetTransientPresentation | Separate save purposes use the existing codec and staged rollback; cosmetic history clears after a successful load | A pre-final save is not an ending save. Failed loads roll back; successful loads must not show future menu text. |
| include/writeover/core/settings.h; src/core/settings.cpp; ADR-0010 | Settings; SettingsRegistry | ApplyKeyValue, ComposeKeyValues | Two bounded cosmetic preferences in the existing text file; no world-wire growth | Settings control readability. They are not another campaign state and should not rewind with a world save. |
| data/rooms/room_roof_exit.json; data/scenes/recovery_scene.json; src/render/character_renderer.cpp | Room; GridWorldQuery; CharCell | RenderCharacterFrame; existing plane sampling and locomotion | Authored platform, parapet and distant geometry; retain clear cells when no finite plane exists | A ray with no floor or ceiling hit must not invent a fake wall of texture. This remains glyph rendering, not pixels. |
| tools/bench/main.cpp; tests/test_product.cpp; scripts/player_product_gate.ps1 | FrameTimeSampler; TestHarness | CharacterRuntimeFrameBenchmark; RegisterProductTests; production probes | Original workload/gate retained, with slow-stage diagnostics and real save/rebuild coverage | worst_1pct_avg_ms averages the slowest one percent; it is not p99. CPU time and wall time answer different questions. |

Presentation demo: show Boot, enter B1, Examine a visible object, open Recent
Events, show current bindings, then show an earned ending summary and separate
Replay Final Choice. Explain the 19 playable rooms versus the fictional 41-level
directory. Do not present automated SVG exports as a human Terminal playtest.

## Optimization and output-correctness addendum

| IMPORTANT_FILES | IMPORTANT_CLASSES | IMPORTANT_FUNCTIONS | WHY_THIS_DESIGN | HOW_TO_EXPLAIN_IT_IN_CLASS |
|---|---|---|---|---|
| src/render/frame_encoder.cpp; utf.cpp; utf_append.h | AnsiFrameEncoder; CharCell | AppendChannel, AppendSgr, AppendUtf8, InitialSgrState, Encode | Append into one payload; explicitly initialize terminal color state and carry it across rows | The terminal is stateful. A newline moves the cursor but does not reset ink. We remove temporary strings without confusing this protocol with the game renderer. |
| src/core/engine.cpp; presentation_cadence.h | Engine; PresentationCadence | Run, Due | Stable presentation deadlines, missed deadlines skipped, unchanged fixed simulation | If rendering takes 2 ms, starting the next interval only after it finishes charges that work twice. Schedule presentation separately from game simulation. |
| src/app/presentation_pulse.h; composition_root.cpp | PresentationPulse; RenderModule | Trigger, Extend, Active, ResetTransientPresentation, DrawVisualEffects | Cosmetic intervals use the paused game clock; successful loads clear old pulses | A flash lasts the same game time at 30 and 120 FPS. Rendering less often must not slow time or bring a future effect back through a save. |
| src/render/character_renderer.cpp | CharacterArtAsset; CameraProjection | DrawOneSprite, DrawDoorPlane, SpriteBackground, WallCell | Cache dimensions locally, preserve projection, keep steel leaves/jambs as joined authored masses | An asset width is unchanged within a draw. Compute it once. A door is still a world plane, and its frame/windows are glyph structures, not a bitmap. |
| data/characters/b1_character_art.txt | CharacterArtBank | existing far/mid/near door-frame load | Same dimensions and collision; single structural frame instead of stacked curved lines | Silhouette and negative space establish architecture before color adds steel and depth. |
| src/app/player_product.h; campaign_panel.h | PlayerProductRuntime | Open, Handle, Rows, DrawCampaignPanel | Stable ordering, focused scrolling, modal contrast, clear unavailable states, existing frame-cap preference | A menu is a small view/controller over existing actions. It does not own objectives or invent new gameplay state. |

Demonstration order: show the old/new door exports, explain the color-state
regression, then compare the identical benchmark workloads. Keep CPU benchmark
time, a synthetic cadence test and measured human display FPS as three distinct
claims. Tests are now 233 in each configuration; see POST_OPTIMIZATION_REVIEW.md
and the current receipt for all route/package/CI evidence.
