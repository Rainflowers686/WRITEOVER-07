# WRITEOVER-07 — Complete Game Campaign Report

STATUS = LOCAL_COMPLETE_GAME_CAMPAIGN_BASELINE_VERIFIED_VISUAL_PASS07_NOT_PRODUCT_GOLD

BASELINE_HEAD = a509e3f5b1dc47baf3dd5a466d13ab1b593cfd53

BASELINE_COMMIT = feat: close campaign routing and delivery contracts

CANDIDATE_PACKAGE = build-campaign-release/dist-final-pass09/
writeover-07-v0.1.0-complete-campaign-candidate-windows-x64.zip

CANDIDATE_PACKAGE_SHA256 = 68589068CB1385E6DC40BFB02A2E1AD167C5F5A32D3134FBC50F3694A60D0B37

This report records the complete, bounded Chapter One plus Act II-A plus
upper-tower campaign baseline and the subsequent visual pass 07. It is a
source-and-runtime report, not a claim of visual gold, manual acceptance,
public-alpha readiness, or course-delivery completion.

## Scope and product boundary

The campaign keeps the existing CharCell / Unicode production renderer and
existing fact, storylet, systemic, save, and scene-transition systems. The
vertical-tower layer is a private application policy for eight named
destinations. It is not a generic Act, Campaign, Region, or 41-floor engine.

The player-facing arc is now:

1. B1 awakening, first weapon/consequence, calibration, medical/security
   routing, and the first elevator departure.
2. Act II-A service-concourse investigation, with records, power, observation,
   and transit alternatives.
3. A bounded lift directory that unlocks Records Core, Operations Control,
   Network Node, Security Transfer, Executive Archive, and Authority Core.
4. A durable pre-final checkpoint followed by Amend, Disclose, or Breach;
   force/alert also remains a valid discovery-poor route that skips Network.
5. Roof / Exit epilogue and campaign-complete closure.

The future 41-floor tower remains a design possibility only. B4 through Roof,
and any floors not listed in the current data, do not exist as runtime content.

## Current playable room inventory

All room rows are authored in data/rooms/<room-id>.json and compiled to the
matching .woc file. Scene-level entities, transition bounds, and patrol routes
are authored in data/scenes/recovery_scene.json and compiled to
data/scenes/recovery_scene.bin.

| ID | Player-facing name | Main function | Connections / route role | Important content |
| --- | --- | --- | --- | --- |
| room_b1_revival | B1 07 Revival / Calibration | Wake, first systemic discovery, weapon and body consequence | Main start -> Calibration | service reader, first guard/cleaner/technician/medical presence, camera and body state |
| room_01_calibration | 校准靶场 | First obstacle and route verification | B1 -> Medical | calibration interaction and service-route gate |
| room_service_medical | Service / Medical Intake | Medical assessment and first route choice | Calibration <-> Staff, Security, Elevator | intake terminal, medical route, Doctor dialogue/state |
| room_restroom_staff | Staff Restroom / Service Route | Optional quiet/staff route | Medical <-> Elevator | staff passage, quiet-route evidence |
| room_1f_security | 1F Visitor Lobby / Security Checkpoint | Credential/security escalation | Medical -> Elevator | security checkpoint, guard, camera/security response |
| room_elevator_lobby | Elevator Lobby / Restricted Core | Chapter One departure and Act II entry | 1F -> Act II-A concourse | elevator entry/checkpoint presentation |
| room_act2_service_concourse | ACT II-A / Service Concourse | Act II-A hub and route choice | Elevator <-> Records, Power, Transit | hub terminal, three corridor choices |
| room_act2_records_archive | ACT II-A / Records Archive | Review the manifest and establish records route | Concourse <-> Observation | Records Operator, archive terminal |
| room_act2_power_utility | ACT II-A / Power Utility | Optional maintenance/power route | Concourse <-> Transit | Power Technician, utility bypass |
| room_act2_observation_gallery | ACT II-A / Observation Gallery | Optional observation/discovery route | Records <-> Observation | Observation Analyst, observation feed |
| room_act2_transit_control | ACT II-A / Transit Control | Act II-A checkpoint and deep-transfer gate | Concourse/Power -> Arrival | Transit Security, checkpoint, quiet/aggressive split |
| room_1f_arrival_lobby | 01F / Arrival Lobby | Upper-tower hub and lift directory | Transit -> directory; each upper room -> return | Arrival Clerk, directory terminal, return lift |
| room_8f_records_core | 08F / Records Core | Query the Subject 07 file and authority lead | Arrival <-> Records Core | Records Archivist, case-file terminal |
| room_12f_operations_control | 12F / Operations Control | Reconcile facility response and choose cooperation/force | Arrival <-> Operations | Operations Operator, control terminal |
| room_18f_network_node | 18F / Network Node | Discover an unlisted observation route | Arrival <-> Network | Network Analyst, observation terminal |
| room_24f_security_transfer | 24F / Security Transfer | Cross upper security and expose consequence of force | Arrival <-> Transfer | Transfer Guard, security gate/reader |
| room_30f_executive_archive | 30F / Executive Archive | Open the sealed executive record | Arrival <-> Archive | Executive Liaison, archive terminal |
| room_36f_authority_core | 36F / Authority Core | Pre-final checkpoint and ending selection | Arrival <-> Authority -> Roof | Authority Presence, decision terminal |
| room_roof_exit | ROOF / Exit Platform | Ending epilogue and closure | Authority -> Roof; return is available after completion | final status terminal, roof epilogue |

CURRENT_PLAYABLE_ROOM_COUNT = 19

CURRENT_EXPLICIT_REGION_COUNT = 0

The data model has rooms and scene transitions, not first-class Region
records. For communication, the rooms can be grouped into B1/1F recovery,
Act II-A service, upper-tower directory destinations, and roof epilogue.
Those four groups are naming/flow groupings, not a runtime region system.

## Current transition graph

The authoritative transition list contains 32 bounded scene transitions. The
high-level graph is:

    B1 Revival
      -> Calibration
        -> Medical
          -> Staff (optional) -> Elevator Lobby
          -> 1F Security -> Elevator Lobby
          -> Elevator Lobby
      -> Elevator Lobby
        -> Act II-A Service Concourse
          -> Records Archive
            -> Observation Gallery (optional)
          -> Power Utility (optional)
          -> Transit Control
            -> Arrival Lobby
              -> Records Core
              -> Operations Control
              -> Network Node
              -> Security Transfer
              -> Executive Archive
              -> Authority Core
                -> Roof / Exit

Every upper-tower destination has a bounded return transition to Arrival Lobby.
Roof also has a return transition after completion. The Arrival lift directory
is the player-facing selector; it only lists destinations whose existing fact
unlock has been earned, plus the current room.

CURRENT_SCENE_TRANSITION_COUNT = 32

CURRENT_DEAD_END_COUNT = 0 intentional hard dead ends; locked/sealed bounds are
gated transitions, not dead-end rooms

CURRENT_OPTIONAL_SPACES = room_restroom_staff, room_act2_power_utility,
room_act2_observation_gallery

CURRENT_BACKTRACKABLE_SPACES = B1/Act II-A route where a transition exists, all
upper-tower destinations through Arrival Lobby, and Roof after completion

## Campaign beats

### Beat 1 — Revival and first consequence

START CONDITION = fresh runtime starts in room_b1_revival.

OBJECTIVE = find the service reader and establish a route out of B1.

PLAYER CHOICES = pistol/stunner use, body search, badge handling, camera/noise
behavior, and whether to use the Medical or Staff route.

SYSTEMIC OPTIONS = lethal or nonlethal contact, body disposition, badge
possession, facility alert, camera response, cleaner/medical observations.

NPC INVOLVEMENT = B1 security, cleaner, technician, and Dr. Vale surfaces are
driven by the existing systemic seed and story/dialogue paths.

CONSEQUENCE = the corridor records the player differently depending on noise,
body state, route, and credential handling.

END CONDITION = B1 service access is granted and the player crosses into
Calibration.

### Beat 2 — Calibration and first-floor routing

START CONDITION = fact_b1_service_access or the existing equivalent B1 route
fact is true.

OBJECTIVE = verify the route, receive medical context, and reach the security
checkpoint/elevator.

PLAYER CHOICES = Medical, optional Staff, direct Security, quiet versus loud
handling.

SYSTEMIC OPTIONS = quiet route, aggressive route, staff route, medical
assessment, security checkpoint state.

NPC INVOLVEMENT = Doctor/medical dialogue, Cleaner history, 1F guard and
security response.

CONSEQUENCE = the Chapter One checkpoint and Act II-A entry inherit the
player-visible route and security facts.

END CONDITION = elevator departure is recorded.

### Beat 3 — Act II-A investigation

START CONDITION = Act II-A concourse is entered after Chapter One completion.

OBJECTIVE = review the dispatch/records trail and obtain a filed route through
Transit Control.

PLAYER CHOICES = Records, optional Power, optional Observation, quiet
cooperation, or force/noise.

SYSTEMIC OPTIONS = records route, maintenance bypass, observation evidence,
Transit Security response, saved/loaded route state.

NPC INVOLVEMENT = Records Operator, Power Technician, Observation Analyst, and
Transit Security.

CONSEQUENCE = facts are carried into the upper-tower directory. The quiet
route suppresses the bounded Transit target stimulus until the player fires;
the aggressive route preserves the alert/force trace.

END CONDITION = fact_act2_checkpoint_reached is established and Arrival Lobby
is entered.

### Beat 4 — Upper-tower investigation

START CONDITION = Arrival Lobby is reached with the Act II-A checkpoint.

OBJECTIVE = use the lift directory to complete the authority chain.

PLAYER CHOICES = destination order, optional return/backtracking, cooperation
or force at Operations/Transfer, network disclosure discovery.

SYSTEMIC OPTIONS = records lead, operations cooperation, network observation,
security bypass, alert route, archive and authority readiness.

NPC INVOLVEMENT = Arrival Clerk, Records Archivist, Operations Operator, Network
Analyst, Transfer Guard, Executive Liaison, and Authority Presence.

CONSEQUENCE = each completed destination unlocks the next directory entry;
operations cooperation and network discovery make Disclose eligible; force or
security alert makes Breach eligible.

END CONDITION = Authority Core sets the pre-final checkpoint.

### Beat 5 — Durable decision and roof payoff

START CONDITION = fact_pre_final_checkpoint is true and the player confirms an
eligible ending in the decision overlay.

OBJECTIVE = choose what happens to the official record.

PLAYER CHOICES = Amend, Disclose when the quiet cooperation/network conditions
are present, or Breach after a force/alert route.

SYSTEMIC OPTIONS = one ending fact is written, campaign-complete is written,
roof unlock is written, and the runtime requests a save.

NPC INVOLVEMENT = Authority Presence/terminal, then roof epilogue.

CONSEQUENCE = the ending label and final status are durable in the existing
fact/save path.

END CONDITION = room_roof_exit is entered and the epilogue terminal is
available.

CURRENT_START = room_b1_revival

CURRENT_LAST_PLAYABLE_BEAT = roof epilogue after ending confirmation

CURRENT_END_CHECKPOINT = fact_pre_final_checkpoint, followed by
fact_campaign_completed and fact_roof_reached

CURRENT_ENDING_TYPE = three bounded fact-gated endings: AMEND, DISCLOSE,
BREACH

CURRENT_CHAPTER_COUNT = 1 named Chapter One plus Act II-A plus upper campaign
continuation; no generic chapter counter exists

CURRENT_ACT_COUNT_IF_ANY = Act II-A is an existing named route and the new
upper sequence is application-level progression, not a generic Act system

## Objective, lead, and discovery presentation

OBJECTIVE_AUTHORITY = existing composition-root objective source, with
TowerCampaignRuntime providing room-specific upper-campaign text

OBJECTIVE_DATA_LOCATION = existing text/storylet/content data plus
src/app/tower_campaign_runtime.cpp for bounded upper-room objective policy

OBJECTIVE_RUNTIME_OWNER = SliceRuntime / RenderModule source callbacks

OBJECTIVE_UPDATE_PATH = room switch, interaction fact writes, and the dynamic
objective source polled by the HUD

HUD_PRESENTATION_PATH = RenderModule HUD/objective source and existing subtitle
layer; the directory/case-file/ending overlays use the PlayerModule input
overlay callback and RenderModule closure text source

| Capability | State | Evidence |
| --- | --- | --- |
| One main objective | IMPLEMENTED | upper-room Objective() and existing HUD objective source |
| Multiple concurrent objectives | PARTIAL | facts can coexist, but HUD exposes one bounded main objective |
| Optional objective | IMPLEMENTED in content flow | optional Staff, Power, and Observation routes exist |
| Lead / clue | IMPLEMENTED bounded | CaseFile() exposes a single current lead |
| Discovery / journal entry | IMPLEMENTED bounded | systemic knowledge entries 9401–9405 and case-file discovery lines |
| Completed objective history | PARTIAL | durable facts/event journal preserve progress; no separate objective-history UI |

## Durable consequences and persistence

The campaign uses existing named facts and existing save/load sections. Important
new or consumed facts include:

- elevator unlocks for Records, Operations, Network, Transfer, Executive,
  Authority, and Roof;
- Act III/IV room-entry and completion facts;
- operations cooperation versus force;
- network discovery;
- guard bypass, guard down, and security alert;
- executive archive and authority readiness;
- pre-final checkpoint;
- ending_amend, ending_disclose, ending_breach, campaign_completed, and
  roof_reached.

The player can return to Arrival and revisit unlocked destinations. Save/load
was observed in both full campaign replays. The new policy deliberately does
not add a new save section or a generic region state container.

## NPC, combat, and art baseline

The current content contains 17 profile records and 16 runtime NPC instances
in the campaign probe. The new runtime profiles are Arrival Clerk, Records
Archivist, Operations Operator, Network Analyst, Transfer Guard, Executive
Liaison, and Authority Presence.

Combat remains the existing bounded implementation:

- Pistol and Stunner are distinct weapon slots with existing ammo/fire paths.
- Hitscan, health, stun/nonlethal state, body creation, search, badge
  possession, noise, and security consequences remain in the existing player,
  systemic, and AI systems.
- No new animation architecture or renderer architecture was introduced.

The protected visual contract remains authored Character-Art rendered as
CharCell/Unicode glyphs with semantic color. Visual pass 07 adds first-lookup
authored Pistol/Stunner mass and restrained Full Human/Maintenance front faces;
the old assets remain available in the bank, and the directional review still
reports 36/36 exact mappings. Real production captures cover B1 human/guard,
Security desk, and Elevator door structure. These are meaningful production
improvements, not a claim of photorealism or manual visual acceptance; the
foreground terminal, audio, first-time play, and integrated all-NPC review
remain open.

## Verification at this baseline

The following were run locally against this source baseline:

- Debug build: PASS.
- Release build of writeover_app, writeover_tests, and writeover_bench: PASS.
- Debug unit tests: 217 tests, 0 failed.
- Release unit tests: 217 tests, 0 failed.
- Content compiler check: PASS; 19 rooms, 69 facts, 27 storylets, 17 NPC
  profiles.
- Content compiler tests: 13/13 PASS.
- Systemic schema tests: 10/10 PASS.
- Systemic schema check: PASS.
- Static audit: PASS, COUNT=0.
- Contract check: PASS.
- Release benchmark: TERMINAL_FULL_BUDGET=PASS,
  TERMINAL_DELTA_BUDGET=PASS, TERMINAL_UNCHANGED_BUDGET=PASS,
  TERMINAL_WORSTCASE_SAFETY=PASS, SYSTEMIC_LOOKUP_BUDGET=PASS,
  SYSTEMIC_UPDATE_BUDGET=PASS, PVS_RENDER_BUDGET=PASS,
  PVS_TOTAL_FRAME_BUDGET=PASS, RAYCAST_BUDGET=PASS, OVERALL_BUDGET=PASS.
- Release full replay: Amend, Disclose, and Breach PASS. All three reached all
  eight new rooms, roof, save/load, CAMPAIGN_COMPLETION_REACHED=YES, and their
  required ending fact.
- Complete campaign ending gate: PASS, 4/4; each fixture independently asserts
  its expected ending fact and end-screen readiness rather than treating Roof
  alone as completion. The fourth fixture skips optional Network and still
  completes through the force/alert fallback.
- Focused Act II route coverage gate: PASS, 3/3 for Power, Observation, and
  Transit; each route entered its authored destination and asserted its key
  consequence without a transition denial or player death.
- Act II expansion gate: PASS.
- Recovery replay gate: PASS, 21/21 legacy replay cases.
- Scenario matrix: PASS; 36 classified rows, 35 executed, 1
  INVALID_BY_GAME_RULES, 0 VALID_STATE_NOT_COVERED.
- The older exact-head CI run for `b734e706...` is not authoritative for this
  source head. The final delivery receipt must show a completed-success run
  whose `headSha` equals the final pushed DELIVERY_HEAD.

The exact remote CI result is intentionally maintained as a live delivery
receipt: it must be checked against the final pushed DELIVERY_HEAD rather than
copied from an older run.

## Open acceptance boundary

OPEN_FATAL = none known from automated/runtime verification.

OPEN_P0 = none known from automated/runtime verification.

OPEN_P1 = foreground Windows terminal placement, human visual acceptance of
faces/NPC silhouettes/weapons/rooms, four-way in-game inspection, audio
listening, first-time classmate playtest, manual Breach presentation, package
smoke from the final published head, and exact-head CI.

This is a complete playable campaign baseline. It is not PRODUCT_GOLD,
VISUAL_GOLD, FINAL_RELEASE, or COURSE_REPORT_READY.
