# WRITEOVER-07 — Post Complete-Game GPT-6 Handoff

STATUS = COMPLETE_GAME_CAMPAIGN_BASELINE_VERIFIED_MANUAL_ACCEPTANCE_OPEN

BASELINE_HEAD = a509e3f5b1dc47baf3dd5a466d13ab1b593cfd53

DELIVERY_HEAD = final pushed HEAD; it may be a documentation-only descendant
of BASELINE_HEAD and must be verified live before claiming exact-head CI.

BASELINE_COMMIT = feat: close campaign routing and delivery contracts

CANDIDATE_PACKAGE = build-campaign-release/dist-final-pass10/
writeover-07-v0.1.0-complete-campaign-candidate-windows-x64.zip

CANDIDATE_PACKAGE_COMMIT = 670aa2441628f632f564b6961827ede73333770e

CANDIDATE_PACKAGE_SHA256 = 0804E354AC0138760CE3EFC92BB7B8056586E1B0BBAA96CB333BFB6BE509DA8A

CANDIDATE_PACKAGE_GATES = positive smoke PASS; missing content, character-art,
and recovery-text negative probes PASS; package is a candidate, not a Release

This handoff supersedes the older Chapter One, Astra/Luna, and pre-complete
game handoff notes for the current source baseline. It does not mean final
release, visual gold, manual acceptance, course-report completion, or public
alpha readiness. The source baseline is now locally verified; delivery still
requires a candidate package from BASELINE_HEAD and a GitHub Actions run whose
headSha exactly matches the final pushed DELIVERY_HEAD. This is not a Luna
handoff.

## Protected baseline

PROTECTED_CREATIVE_BASELINE =

- WRITEOVER-07 remains a Character-Art first-person FPS/MUD hybrid.
- The world remains CharCell/Unicode authored structure. No RGB framebuffer,
  half-block framebuffer, Braille raster, or automatic image-to-character
  conversion.
- Dense low-resolution FPS composition is allowed through authored glyph
  blocks, semantic color, material grouping, negative space, depth, and
  silhouette. Color may support material and atmosphere but may not carry the
  only readable object structure.
- Security, Cleaner, Technician, Medical/Full Human, Pistol, and Stunner are
  authored assets. Do not replace them with generated raster art or a generic
  sprite sheet.
- Front/Back/SideLeft/SideRight must agree with actor yaw, viewer relation,
  CharacterFacing, asset selection, and inspection label. Side views must
  actually change silhouette/overlap; Back must show back-of-head/back
  equipment relationships.
- Rooms are composed from architecture, functional zoning, focal equipment,
  navigation path, material, and restrained small texture. B1, Security,
  Elevator, Arrival, Authority, and Roof deserve the strongest composition.
- Doors are architectural thresholds: wall opening, frame, panel, observation
  window/signage, and depth relation. A floating door sprite is a regression.
- HUD has one clear main objective, restrained subtitle hierarchy, semantic
  palette, readable interaction prompts, and no debug/rainbow-terminal feel.
- Narrator is observant, dry, institutionally literate, sparse, and willing to
  leave silence. Security is procedural; Cleaner practical/observant;
  Technician technical/casual/distracted; Medical clinical but human.
- First-ten-minute rhythm is wake/intrigue -> first obstacle -> systemic
  discovery -> consequence -> escalation -> payoff.
- Memorable beats should use reactive Character-Art, systemic consequences,
  Security escalation, facility/camera reaction, Cleaner/body consequence, and
  the Authority/Roof decision. Do not add a large cinematic layer.

DO_NOT_REGRESS =

- Do not add a generic Campaign/Act/Region engine for the bounded tower.
- Do not make the 41-floor concept sound implemented. Current runtime has
  19 rooms and eight upper-directory destinations.
- Do not remove the existing Act II-A Records/Power/Observation/Transit
  choices or the B1 route consequences.
- Do not bypass the existing fact/systemic/storylet/save paths with a second
  hidden state store.
- Do not make the lift directory a free teleport that ignores unlock facts,
  current-room state, or return transitions.
- Do not restore the old inappropriate opening Security line.
- Do not “fix” any visual concern by turning glyph rendering into pixels.
- Do not stage, reset, clean, stash, or overwrite Rain’s unrelated settings,
  test_harness newline edit, build outputs, or historical evidence.

## Current player experience

CURRENT_PLAYER_EXPERIENCE =

The player can complete B1, Chapter One, Act II-A, and the bounded upper-tower
investigation. The upper route starts at Arrival Lobby after Act II-A Transit
checkpoint. The lift directory progressively exposes Records Core, Operations
Control, Network Node, Security Transfer, Executive Archive, Authority Core,
and Roof. The player can return to Arrival between upper rooms. Authority Core
writes a pre-final checkpoint and presents eligible endings. Amend, Disclose,
Breach, and a discovery-poor Breach route now each have a dedicated Release
probe and ending-specific assertion; manual foreground presentation remains
open.

Automated route observations:

- Amend replay: all eight new rooms, save/load, campaign completion, force and
  alert facts, `amend=YES`, roof reached, no transition denial, player alive.
- Disclose replay: all eight new rooms, save/load, campaign completion,
  cooperative Operations, network discovery, no facility alert,
  `disclose=YES`, roof reached, no transition denial, player alive.
- Breach replay: all eight new rooms, save/load, campaign completion, force and
  alert facts, `breach=YES`, roof reached, no transition denial, player alive.
- Discovery-poor Breach replay: skips optional Network, uses the Operations
  force/alert fallback, reaches Transfer/Archive/Authority/Roof, and asserts
  `network=NO`, `force=YES`, `transfer=YES`, `breach=YES`.
- Focused Act II route gate: Power, Observation, and Transit each enter their
  authored room and assert their key player-facing consequence without a
  transition denial or player death.
- Current replay output distinguishes legacy CHAPTER_CHECKPOINT_REACHED from
  CAMPAIGN_COMPLETION_REACHED; a complete campaign should use the latter.

## Current major systems

CURRENT_MAJOR_SYSTEMS =

- Existing fixed-step engine and callback-wired composition root.
- Room grid, scene entity, transition, patrol-route, and compiled content
  loaders.
- Production CharCell/Unicode renderer, raycaster, CharacterRenderer, HUD,
  terminal delta encoder, and Win32/POSIX terminal backends.
- Existing Pistol/Stunner combat, hitscan, health, stun, bodies, noise, LOS,
  guard response, and systemic event bridge.
- Fact belief store, storylet scheduler, dialog queue, narrator observability,
  and existing narrative module.
- Existing save/load codec and runtime checkpoint save/load.
- New private TowerCampaignRuntime for destination unlocks, single objective,
  case-file lead/discoveries, and ending eligibility only.
- Visual pass 07 first-lookup authored weapon states and Full Human/Maintenance
  Front Far/Mid/Near states in `data/characters/b1_character_art.txt`; the
  existing four-way bank remains intact and no raster path was added.
- 19 room authoring records, 32 scene transitions, 69 facts, 27 storylets,
  17 NPC profiles, 16 NPC instances in the full replay probe.

## Open work classified for GPT-6

OPEN_FATAL = none known from automated/runtime evidence.

OPEN_P0 = none known from automated/runtime evidence.

OPEN_P1 =

- Foreground Windows terminal acceptance remains human-open.
- Integrated visual acceptance remains open for human confirmation of face
  restraint, major NPC proportions, four-way in-game inspection, Pistol/Stunner
  first-person placement, room focal structure, door multi-angle depth, and HUD
  typography. The checked-in pass materially improves these areas but does not
  promote them to subjective visual PASS.
- Audio listening and narrator/dialogue timing remain human-open.
- First-time classmate playtest and measured playtime remain open.
- Manual Breach presentation and durable reload acceptance remain open; the
  dedicated automated route is now covered.
- The final candidate archive must be built from BASELINE_HEAD, and exact-head
  CI must be checked against the final pushed DELIVERY_HEAD; the older
  `b734e706...` CI receipt is intentionally not authoritative for either.

REMAINING_OBJECTIVE_WORK = manually validate objective readability,
case-file lead/discovery usefulness, directory selection affordance, and
ending objective/closure text in the foreground terminal.

REMAINING_GAMEPLAY_WORK = add or repair only player-critical defects found by
the integrated campaign/manual run; do not grow systems speculatively. Confirm
the manual Breach presentation and any death/restart/reload edge case that
appears in manual play.

REMAINING_CONTENT_WORK = targeted text/room/NPC refinement after visual and
first-time-play review. Preserve the bounded eight-destination scope unless a
new player-critical gap is proven.

REMAINING_TEST_WORK = add narrowly targeted regression for discovered defects;
keep the existing 217-test suite, content checks, schema checks, the four
campaign ending probes, three focused Act II route probes, save-fault coverage,
static audit, contract check, package positive/negative checks, and benchmark
contracts green.

REMAINING_PACKAGE_WORK = package the Windows candidate from BASELINE_HEAD using
the existing release scripts, run positive package smoke plus the missing
content/art/text negative probe, and record archive/hash without staging
build/dist artifacts.

REMAINING_COURSE_REPORT_WORK = inspect actual course requirements, then fill
the code/playable-package/test-report/design-report/PPT/WBS/UML/algorithm/STL/
pattern/ARM64/problems/member/demo deliverables. Do not fabricate team-member
responsibilities.

REMAINING_DEMO_WORK = perform a real five-minute route rehearsal with the
foreground terminal, a visible first-person weapon, one systemic consequence,
the lift directory, and one ending. Record what fits in five minutes rather
than claiming the complete campaign is five minutes.

SAFE_FOR_GPT6 =

- Foreground manual visual review and evidence capture.
- Targeted Character-Art, four-way pose, door, room-composition, weapon,
  HUD/palette, narrator-timing, and dialogue polish within existing renderer
  and content systems.
- Focused Breach replay/manual coverage.
- Final package generation, package smoke, course-material inspection, and
  bounded report/demo work.
- Small player-critical bug fixes with new focused regression tests.

GPT6_SHOULD_NOT_REDESIGN =

- renderer architecture or terminal transport;
- Character-Art architecture;
- save schema or a new region/act/campaign framework;
- lift directory into unrestricted teleportation;
- existing Act II-A route graph;
- fact/systemic/storylet ownership;
- the visual contract that glyphs remain structural.

LIKELY_COMPLETION_SEQUENCE =

1. Run the manual checklist in the real foreground Windows terminal and listen
   to the audio.
2. Capture only player-critical findings; fix small defects in the correct
   layer and add focused tests.
3. Inspect the checked-in visual pass in the foreground terminal, then capture
   only player-critical revisions for B1, Security, Elevator, major NPCs,
   four-way facing, Pistol, and Stunner.
4. Run all three ending paths, including Breach, with save/load and backtrack;
   the automated four-route ending gate and three-route Act II coverage gate
   are already present.
5. Run Debug/Release build, tests, content/schema/static/contract checks,
   replay suite, scenario/save matrix, benchmark, and package smoke.
6. Commit documentation/evidence separately from source/content where practical.
7. Push main normally, observe the exact final-head CI run, and only then
   update the release/hand-off status.
8. Keep the final status honest: ready for a director/audit pass is not
   product gold.

## Presenter critical knowledge

PRESENTER_CRITICAL_KNOWLEDGE =

ARCHITECTURE = game_main starts the executable; composition_root wires modules
and callbacks; engine owns fixed-step/presentation cadence; application policy
remains explicit.

GAME_LOOP = platform input becomes mapped actions; PlayerModule consumes input;
world/AI/narrative/systemic state update; RenderModule presents a terminal frame.

RENDERER = raycaster and production renderer emit CharCell/Unicode structure;
CharacterRenderer projects authored bodies/props/doors; HUD and terminal
encoder are separate presentation layers.

CHARACTER_ART = assets are authored by role/facing/LOD, not image-quantized;
orientation is a spatial contract, not merely a label.

INTERACTION = prompts and dispatch are room/entity/fact-aware; terminals,
readers, NPCs, doors, and transitions share the existing interaction seam.

AI = perception, noise, inspection, investigation, patrol, combat, last
stimulus/memory, and systemic event bridge are bounded; do not claim full
tactical cross-room AI.

SYSTEMIC_FACTS = shots, noise, bodies, badges/credentials, route choices,
security alert, cooperation, discovery, archive, authority, ending, and roof
facts are durable consumers of player action.

NARRATIVE = storylets are data-driven and priority/condition gated; the
TowerCampaignRuntime only owns bounded directory/objective/case-file/ending
policy.

SAVE_LOAD = existing validated sections serialize the durable runtime state;
new campaign facts fit naturally without a new schema section.

CONTENT_PIPELINE = author JSON under data, compile/check with
tools/contentc/contentc.py, run content tests/schema checks, then load .woc/.bin
at runtime.

TESTING = unit tests prove contracts; replays prove integrated routes; content,
schema, static, contract, benchmark, package, and CI gates have separate
meaning; none replaces human terminal/audio/playtest acceptance.

CROSS_PLATFORM = Windows uses Win32 input/terminal paths and POSIX has separate
backends; preserve existing CMake target/platform gates and do not assume a
Windows terminal capture proves macOS/ARM64 behavior.

## Important files/classes/functions

IMPORTANT_FILES = src/app/composition_root.cpp,
src/app/tower_campaign_runtime.cpp,
src/app/tower_campaign_runtime.h,
src/app/interaction_runtime.cpp,
src/render/character_renderer.cpp,
src/render/production_renderer.cpp,
src/core/save.cpp,
src/systemic/systemic.cpp,
src/narrative/storylet.cpp,
data/scenes/recovery_scene.json,
data/rooms/*.json,
data/facts/facts.json,
data/storylets/storylets.json,
data/npcs/npcs.json,
tools/replay/campaign_probe_amend.txt,
tools/replay/campaign_probe_disclose.txt,
tools/replay/campaign_probe_breach.txt,
tools/replay/campaign_probe_discovery_poor.txt,
tools/replay/act2_power_route_from_concourse_probe.txt,
tools/replay/act2_observation_terminal_route_probe.txt,
tools/replay/act2_transit_route_probe.txt,
scripts/complete_game_campaign_gate.ps1,
scripts/act2_route_coverage_gate.ps1,
tools/release/package_negative_probe.py

IMPORTANT_CLASSES = SliceRuntime, PlayerModule, RenderModule,
TowerCampaignRuntime, SceneRuntime, InteractionRuntime, ProductionRenderer,
CharacterRenderer, SystemicWorld, StoryletScheduler, SaveManager/SaveCodec

IMPORTANT_FUNCTIONS = switch_room, transition_allowed, open_campaign_directory,
open_case_file, open_final_decision, SetInputOverlayCallback,
TowerCampaignRuntime::SelectableDestinations,
TowerCampaignRuntime::EligibleEndings,
TowerCampaignRuntime::CaseFile, campaign objective source, interaction
dispatch, runtime save/load, replay diagnostics

WHY_THIS_DESIGN = it adds enough authored content for a complete campaign while
keeping the architectural seam narrow, testable, and reversible.

HOW_TO_EXPLAIN_IT_IN_CLASS = show one player action becoming a fact, show a
later room/terminal reading that fact, then show the ending eligibility and
save/load preserving it. Mention the visual contract and the honest manual
acceptance boundary.
