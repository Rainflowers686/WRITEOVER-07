# WRITEOVER-07 — Recovery-04 Capability Truth Matrix

This is the current bounded capability receipt for final documentation receipt
head `2895050ae6c4990315d1816adf5da3c53bd374e1` (source implementation head
`b7cea9cca387f30a4b4d81b9b9f3d81f186c7859`; portable test receipt
`b6dacee155b1af3492f48eb02f0030dc7cc82ebe`). The final documentation receipt
adds no runtime behavior. `VERIFIED` means that the live
code path and a current assertion, replay, or gate exercised the capability.
`PARTIAL` means that a bounded recovery seam is real but the general product
capability is not claimed. `DEFERRED` is an explicit boundary, not a hidden
PASS.

## Recovery-04 capability matrix

| Capability | Status | Current evidence | Boundary / not claimed |
|---|---|---|---|
| TERMINAL_NORMAL_LAUNCH | PARTIAL | Runtime probes VT capability and fits the render surface to queried backend limits; non-foreground Release smoke reports the honest `legacy-conhost` / `win32-writeconsole` fallback. | A real foreground Windows Terminal session was not available to this run; terminal-host visual acceptance remains manual. |
| SCENE_ENTITY_AUTHORED | VERIFIED_BOUNDED | Current B1/1F terminal, camera, cart, reader, door and relevant props are consumed from compiled recovery scene records. | Not a general ECS or whole-building entity database. |
| ROOM_LINK_AUTHORED | VERIFIED_BOUNDED | Current recovery room links are compiled and loaded as bounded room transitions with destination transform/yaw. | No building-wide portal or sector renderer is claimed. |
| GENERAL_NAVIGATION | VERIFIED_BOUNDED | Deterministic grid routing and obstacle validation are exercised in the current rooms. | No navmesh, crowd simulation, or large-building scalability claim. |
| PATROL_RUNTIME | VERIFIED_BOUNDED | Cleaner/guard patrol states produce actual movement through authored points; current Release success and AI tests exercise movement. | One bounded recovery population only. |
| INVESTIGATE_RUNTIME | VERIFIED_BOUNDED | Reachable noise/evidence causes route, arrival, inspection and bounded response; unreachable/blocked paths do not teleport. | No tactical planner or general multi-agent coordination. |
| GUARD_COMBAT | VERIFIED_BOUNDED | Active-room, LOS, range, cadence, stun/death and pause checks gate authoritative player damage; health/death replay passes. | This is a bounded health/combat proof, not complete enemy combat AI. |
| STEALTH_COUNTERFACTUAL | VERIFIED_BOUNDED | Posture, movement, light and distance produce deterministic visibility differences; guard sight avoids double distance attenuation. | No full stealth subsystem or perception simulation. |
| MEMORY_BEHAVIORAL_USE | VERIFIED_BOUNDED | Semantic memory refresh/dedup is used by a later cleaner response; durable relationship/history survives save/load in the current recovery route. | No general social simulation. |
| PISTOL_PRESENTATION | VERIFIED_BOUNDED | Existing Character-Art pistol viewmodel remains the active Pistol presentation and is covered by prior visual evidence. | This audit does not certify new art acceptance. |
| SMG_TRUTH | PARTIAL | Existing selectable slot has distinct weapon identity and bounded logic/art wiring. | No new SMG art or expanded weapon validation was required by Recovery-04; current recovery evidence does not claim a complete SMG player loop. |
| STUNNER_TRUTH | VERIFIED_BOUNDED | Existing Stunner slot drives the non-lethal path, creates an unconscious body, and cancels an in-progress reload before switching slot. | No new weapon type or advanced stunner combat is claimed. |
| ADS | DISABLED_BY_DESIGN | No active player-facing ADS capability is claimed by this recovery; `combat_.aiming` is not presented as a feature. | A future ADS implementation needs its own bounded contract and evidence. |
| DIRECTIONAL_ART | PARTIAL | Runtime world orientation and bounded direction selection exist; current authored visual coverage is limited to recovery archetypes/LODs. | Not final directional art for every NPC or room. |
| LOD_HYSTERESIS | VERIFIED_BOUNDED | Runtime visual state carries stable LOD selection around current thresholds; character tests exercise hysteresis. | No claim of a full asset authoring catalogue. |
| NARRATIVE_TEXT | VERIFIED_BOUNDED | Recovery storylet text IDs resolve through authored UTF-8 text resources; missing required production text fails closed. | Only current recovery lines are authored. |
| NARRATIVE_VISIBLE_ACTION | VERIFIED_BOUNDED | A fact change makes a storylet eligible, its action runs, and a natural-language subtitle enters the visible queue; success replay records the action. | No full narrator authority/observability loop. |
| QUEST_PRESENTATION | VERIFIED_BOUNDED | `ObjectiveWasPresented` records that the active objective was shown during the route; completion clears the active objective without invalidating that receipt. | No complete quest graph or quest UI. |
| TRANSACTIONAL_FINAL_COMMIT | VERIFIED | All eight final-commit fault stages (`after_room`, `after_world`, `after_systemic`, `after_events`, `after_rng`, `after_narrative`, `after_ai`, `after_player`) roll back with failure and semantic/byte equality. | The tested fault seam is bounded to the existing save sections. |
| HISTORY_SAVE_LOAD | VERIFIED_BOUNDED | Player/world/systemic/events/RNG/narrative/AI state is staged and restored; the B1 success route and fault matrix pass. | No claim of compatibility with arbitrary unversioned external saves. |

## Current settings truth

| Setting | Status | Evidence / boundary |
|---|---|---|
| `fov` | VERIFIED | Used by the current camera/render projection. |
| `difficulty` | VERIFIED_BOUNDED | Passed into the current NarrativeModule; it does not claim a complete difficulty system. |
| `frame_rate_cap` | VERIFIED_PRESENTATION_ONLY | Consumed by the Engine render/presentation loop, clamped to the existing 120 Hz ceiling; fixed simulation timing remains unchanged. |
| `gamepad_sensitivity` | DEFERRED | No current gamepad backend consumer in the bounded recovery slice. |
| `aim_assist` | DEFERRED | No active aim-assist capability is exposed. |
| `interaction_highlight` | DEFERRED | No separate highlight renderer is exposed. |
| `tactical_focus` | DEFERRED | No tactical-focus feature is exposed. |

## Replay and binary receipt

The current Release binary was rebuilt from the code above. Its SHA-256 is
`6FB7398C4BDA08A0C68B18EB11A6631E560EBFC451651CA114EFEA1CBED94E0C`.
`scripts/recovery_replay_gate.ps1` independently checked
`PROCESS_EXIT_OK`, `INPUT_CONSUMED`, `EXPECTED_STATE_REACHED`, and
`REPLAY_RESULT` for:

* `recovery_b1_success` — PASS; badge held, terminal opened, gate opened and
  crossed, body lifecycle and checkpoint reached.
* `recovery_b1_denied` — PASS; no badge, access denied, gate stayed closed and
  was not crossed.
* `recovery_b1_terminal_denied` — PASS; terminal denial remained a denial.
* `recovery_b1_badge_only` — PASS; obtaining a badge alone did not imply gate
  access.
* `recovery_b1_health_death` — PASS; current guard damage reaches authoritative
  death and recovery assertions.

The direct Release unit executable ran 206 tests with 0 failed. The final
commit fault matrix ran all 8 stages with rollback asserted. Benchmark values
are reported as `worst_1pct_avg_ms`, not p99; the current Release character
render workload measured 1.308 ms and the integrated proxy measured 2.220 ms
with platform writes excluded. These are not end-to-end 120 Hz proof.

The final GitHub Actions run for this receipt is `34317014333` and completed
successfully for all five jobs. A foreground Windows Terminal visual session
was not available to the current automation surface, so that evidence remains
`PENDING_MANUAL`.

## Explicit non-claims

The following remain outside this recovery claim: complete enemy combat,
general-purpose ECS, building-wide content, full ADS, general gamepad/aim
assist/tactical focus, a universal directional-art catalogue, and foreground
Windows Terminal visual acceptance. Historical PVS01 replay files remain
preserved; they are not silently treated as current mandatory gameplay gates
when their interaction assumptions are obsolete.

## Vertical Slice Alpha-01 receipt

Source implementation head: `a3266ab1427eef416dd2963a54dc08be4a45096f`.
This section records the bounded first-playable slice; it does not promote the
project to product gold or replace the manual foreground-terminal review.

| Capability | Status | Evidence / boundary |
|---|---|---|
| FIRST_10_MINUTE_SLICE | VERIFIED_BOUNDED | Existing B1 route now has contextual onboarding, objective progression, two materially different replayed approaches, delayed consequence, narrative reaction and a calibration checkpoint. The replay is intentionally shorter than a first-time human playthrough. |
| ONBOARDING | VERIFIED_BOUNDED | First B1 frames expose compact WASD/mouse/F/LMB guidance through the normal HUD; no tutorial menu or internal identifiers are added. |
| INTERACTION_PROMPTS | VERIFIED_BOUNDED | Focused body/cart/camera/terminal/reader/NPC prompts are derived from the same target and state used by interaction callbacks. |
| OBJECTIVE_FLOW | VERIFIED_BOUNDED | B1 objective advances from body/access to terminal/reader/crossing; calibration and later authored recovery rooms expose bounded next-step text. |
| SYSTEMIC_ROUTE | VERIFIED | `alpha01_systemic_success` passes with non-lethal hit, body search/drag/hide, Cleaner discovery/response, badge, terminal, door crossing, save/load, narrative action and checkpoint. |
| AGGRESSIVE_ROUTE | VERIFIED | `alpha01_aggressive_success` passes with a real lethal Pistol route, runtime body, credential transfer and checkpoint; it deliberately leaves the body exposed and does not claim the same final state as the systemic route. |
| OPTIONAL_VARIATION | VERIFIED_BOUNDED | `alpha01_memory_consequence` proves a prior Cleaner relationship/history changes a later discovery response; the earlier Recovery denied route remains the exposed/no-access counterfactual. |
| BODY_GAMEPLAY | VERIFIED | Search, credential transfer, drag movement and cart concealment are exercised by the Release systemic route; body state remains systemic rather than subtitle-only. |
| CLEANER_CONSEQUENCE | VERIFIED_BOUNDED | Cleaner motor reaches the authored area, inspects after arrival and emits a bounded response. Response is read from the systemic event ledger. |
| MEMORY_DELAYED_CONSEQUENCE | VERIFIED | Relationship/history is created through player interaction, survives the Alpha save/load route and produces `HelpCoverUp` in the later Cleaner decision. |
| PATROL_ENCOUNTER | VERIFIED_BOUNDED | Authored Cleaner/Guard/Technician patrol routes and motor movement are exercised by AI tests and live replay loop counts. |
| INVESTIGATE_ENCOUNTER | VERIFIED_BOUNDED | Reachable noise/evidence routes NPC investigation; obstacle and no-route behavior are covered by AI tests. |
| STEALTH_ENCOUNTER | VERIFIED_BOUNDED | Posture, movement, light and distance alter deterministic player visibility; there is no large detection-meter or final stealth presentation claim. |
| GUARD_COMBAT_ENCOUNTER | VERIFIED_BOUNDED | Guard active-room/LOS/range/cadence damage reaches authoritative player health and death/recovery. This is a bounded health/combat proof, not complete enemy combat AI. |
| PISTOL_ROUTE | VERIFIED_BOUNDED | Existing Pistol slot fires through the real AI/event/health path and supplies the aggressive route; no new weapon type or art-gold claim. |
| STUNNER_ROUTE | VERIFIED | Existing Stunner slot drives non-lethal hit → unconscious body → search/drag/hide in the systemic route. |
| SMG | DEFER_ASTRA | Existing slot remains outside the Alpha core route; no expanded SMG content or art polish is claimed. |
| TERMINAL_GAMEPLAY | VERIFIED_BOUNDED | Credential possession gates the B1 terminal; successful access creates a session/audit and route fact, while denial stays denied. |
| CAMERA_GAMEPLAY | VERIFIED_BOUNDED | B1 camera can be disabled through focused interaction, creates a vandalism/outage fact and feeds the current narrator reaction. |
| READER_GATE | VERIFIED | Held valid badge plus clearance opens the real B1 door and crossing changes room/checkpoint state; denied route leaves it closed. |
| CHECKPOINT | VERIFIED_BOUNDED | B1 service crossing loads `room_01_calibration`, completes the bounded opening quest and records the active objective presentation before completion. |
| NARRATIVE_ALPHA | VERIFIED_BOUNDED | Current Alpha facts select authored natural-language text resources; visible action is asserted in replay. Final prose is deferred. |
| NORMAL_PLAYER_ROUTE | ENGINEERING_READY_MANUAL_PENDING | The authored route and prompts are present, but an unattended direct human 8–12 minute foreground session was not used as a substitute for engineering replay. |
| MANUAL_PLAYABILITY | PENDING_MANUAL | Requires Rain to launch the Release executable in the intended foreground Windows Terminal and play the route. |
| ART_POLISH | MANUAL_REVIEW_PENDING | Existing Character-Art direction and HUD remain unchanged in this slice; no visual-gold claim is made. |
| AUDIO_POLISH | DEFER_ASTRA | Existing procedural/placeholder cues remain; final audio design is outside Alpha-01. |

### Alpha replay receipt

`scripts/recovery_replay_gate.ps1` now checks the five Recovery cases and four
Alpha cases independently for process exit, input consumption, expected state
and replay result. The current Release gate returned `RECOVERY_REPLAY_GATE=PASS`.

* `alpha01_systemic_success` — PASS: non-lethal/systemic route, body hidden
  and discovered, Cleaner response, credentialed terminal and service gate.
* `alpha01_aggressive_success` — PASS: Pistol damage/death, runtime body,
  credentialed gate and checkpoint without the concealment route.
* `alpha01_denied` — PASS: no badge, access attempted and denied, gate closed,
  no terminal session and no checkpoint.
* `alpha01_memory_consequence` — PASS: durable Cleaner relationship/history,
  save/load, hidden body discovery and altered Cleaner response.

These are deterministic integration receipts, not a claim that a human has
completed the full slice or that the foreground visual presentation has been
accepted.

## Vertical Slice Alpha-02 — Functional Chapter One

The following is the newer bounded Chapter One receipt. It does not alter the
historical Alpha-01 or Recovery-04 claims above. Source implementation/fix
head: `79c893d9f8f109b54c9af7dd3814b9ee5ffb81d9` (implementation
`c6b3872611ed3a299cb5e05b9b0569083b88d187`; the follow-up source fix removes
an unused transition helper required by cross-platform `-Werror` CI builds).

| Capability | Status | Evidence / boundary |
|---|---|---|
| CHAPTER01_FUNCTIONAL | VERIFIED_BOUNDED | Six existing rooms form a real B1 → calibration → medical → staff/security → elevator flow with authoritative checkpoint state. The 15–25 minute human target remains unmeasured. |
| CHAPTER01_SYSTEMIC_ROUTE | VERIFIED | `chapter01_systemic` proves non-lethal/body concealment, durable Cleaner history, credentialed terminals/readers, quiet staff route, narrative action, save/load and checkpoint. |
| CHAPTER01_AGGRESSIVE_ROUTE | VERIFIED | `chapter01_aggressive` proves a distinct loud/lethal/security route with runtime body, bounded Guard damage/health proof, security checkpoint, save/load and elevator completion. |
| CHAPTER01_BRANCHING | VERIFIED_BOUNDED | Quiet/staff and aggressive/security routes diverge in durable facts and later access, then reconverge at the elevator. |
| CHAPTER01_CONSEQUENCE_PROPAGATION | VERIFIED_BOUNDED | Body/loudness/relationship/camera facts alter later route or Cleaner response; no replay-only branch flag is used. |
| CHAPTER01_OBJECTIVES | VERIFIED_BOUNDED | Objective progression continues beyond B1 through calibration, medical, branch selection and elevator. Completion does not require a stale active objective. |
| CHAPTER01_CHECKPOINT | VERIFIED_BOUNDED | Elevator completion requires route/checkpoint conditions and records the durable chapter fact. |
| CALIBRATION_GAMEPLAY | VERIFIED_BOUNDED | Authored terminal, objective and bounded links are runtime-interactive. |
| MEDICAL_GAMEPLAY | VERIFIED_BOUNDED | Intake terminal and body/route consequence surface provide the second-stage service branch. |
| SECURITY_GAMEPLAY | VERIFIED_BOUNDED | Patrol/investigate/stealth and bounded Guard combat/health proof are available in the authored security room. |
| STAFF_GAMEPLAY | VERIFIED_BOUNDED | Compact quiet-route room and service link are reachable only after the quiet condition. |
| ELEVATOR_GAMEPLAY | VERIFIED_BOUNDED | Elevator room is a real checkpoint interaction, not an unconditional coordinate trigger. |
| NPC_POPULATION | VERIFIED_BOUNDED | Five runtime instances from six authored profiles are used; no decorative fake NPC population was added. |
| PATROL | VERIFIED_BOUNDED | Three authored patrol routes use the existing motor/decision seams. |
| INVESTIGATE | VERIFIED_BOUNDED | Reachable noise/evidence and body/camera consequences feed current-room investigation. |
| STEALTH | VERIFIED_BOUNDED | Existing deterministic posture/movement/light/distance visibility affects the security approach; no large stealth framework is claimed. |
| COMBAT | VERIFIED_BOUNDED | Guard attack reaches authoritative player health/death with room/LOS/range/cadence checks. This is not complete enemy combat AI. |
| MEMORY_RELATIONSHIP | VERIFIED | Cleaner history changes a later discovery response and survives save/load. |
| CAMERA_CONSEQUENCE | VERIFIED_BOUNDED | B1 camera state feeds durable outage/surveillance consequence and current narrative response. |
| BODY_CONSEQUENCE | VERIFIED | Body state and concealment affect the quiet/aggressive branch and Cleaner response. |
| MID_CHAPTER_SAVE_LOAD | VERIFIED | Quiet and aggressive chapter replays continue to completion after a mid-route save/load. |
| BACKTRACKING | VERIFIED_BOUNDED | Calibration ↔ medical backtrack replay passes without duplicating completion or losing objective state. |
| SOFTLOCK_RED_TEAM | VERIFIED_BOUNDED | Denial, badge-only, terminal-denied, death/recovery, backtracking, branch divergence and save/load counterfactuals pass; exhaustive human ordering review remains future review. |
| NARRATIVE_FUNCTIONAL | VERIFIED_BOUNDED | Thirteen compiled storylets use authored natural-language resources for the current chapter reactions. |
| NARRATOR_FUNCTIONAL | VERIFIED_BOUNDED | Current bounded facts trigger visible chapter reactions; final narrator prose is not claimed. |
| FINAL_PROSE | DEFER_ASTRA | Functional text only. |
| FINAL_ART | DEFER_ASTRA | Existing Character-Art path is preserved; no art-gold claim. |
| FINAL_AUDIO | DEFER_ASTRA | No final audio pass. |
| MANUAL_PLAY | PENDING_MANUAL | No first-time human duration or foreground Windows Terminal acceptance was fabricated from replay frames. |
| CHAPTER01_REPLAY_RECEIPT | HISTORICAL_RECEIPT_ONLY | The earlier Alpha-02 document's 15/15 statement is retained as history; the current committed gate is reported in the Alpha-03 continuation addendum below. |
| SAVE_FAULT_MATRIX | VERIFIED | Eight final-commit fault stages roll back without live-state mutation. |
| BENCHMARK | VERIFIED_BOUNDED | Current Release proxy remains within recorded budgets; platform terminal writes and end-to-end 120 Hz are not proven. |

Alpha-02 remote receipt: normal main push completed; GitHub Actions run
`34353383751` passed all five jobs for validated code/fix head
`79c893d9f8f109b54c9af7dd3814b9ee5ffb81d9`. The documentation receipt itself
does not make a Product Gold, Visual Gold, release, or manual-play claim.

## Alpha-03 continuation current-head addendum

This addendum is the current receipt for implementation head
`2efde27325dd26baeb2fdc33b584acdfa81c6911`, from start head
`4c809a8b0340808b1db287db3f71a340bcc28ea4`. It corrects the current coverage
statements without rewriting any historical commit.

| Capability | Current status | Current evidence / boundary |
|---|---|---|
| `CHAPTER01_REPLAY_RECEIPT` | `VERIFIED_BOUNDED` | The committed Release gate executed 18 cases: 9 Recovery/Alpha cases, 6 Chapter One cases, and 3 current regression fixtures. Each receipt separates process exit, input consumption, expected state and result. |
| `CHAPTER01_SCENARIO_MATRIX` | `VERIFIED_BOUNDED` | 36 rows were classified; 18 authored fixtures executed and passed, 18 rows were explicitly `INVALID_SETUP`. Counts: 11 completable, 5 expected denial, 2 expected failure/recovery. |
| `ELEVATOR_SOFTLOCK` | `FIXED_AND_VERIFIED` | Disabled-guard security bypass records the durable security checkpoint before entering the terminal elevator room. |
| `NO_SAVE_DEATH_RECOVERY` | `FIXED_AND_VERIFIED` | A no-save death route accepts F9 and restarts the authored room at spawn with health 100 and explicit player text. |
| `B1_TERMINAL_GATE` | `FIXED_AND_VERIFIED` | B1 crossing consumes the shared transition policy; badge-only/no-terminal remains denied. |
| `INCAPACITATED_ACTOR_BODY` | `FIXED_AND_VERIFIED` | Runtime incapacitated actors create room-local BodyRecords and the body renderer consumes them outside the B1-only branch. |
| `FACILITY_ALERT_CONSUMER` | `VERIFIED_BOUNDED` | Suspicious alert state changes the current security objective; per-NPC alertness remains diagnostic and is not claimed as a full AI consumer. |
| `PACKAGE_RUNTIME_ASSETS` | `VERIFIED_BOUNDED` | Clean Windows package smoke requires authored character art and recovery text, checks executable-relative data and user-data separation. |
| `SAVE_FAULT_MATRIX` | `VERIFIED` | Fresh current-head injection passed all eight final-commit stages with rollback and `LOAD_OK=NO`. |
| `COMPLETE_ENEMY_COMBAT` | `NOT_CLAIMED` | Guard health/death is a bounded current Chapter One proof, not complete enemy combat AI. |
| `TERMINAL_NORMAL_LAUNCH` | `PENDING_MANUAL` | Current non-foreground pipe correctly reports the compatibility fallback; a foreground Windows Terminal acceptance run has not been fabricated. |

The fresh local gates for this addendum were: Debug/Release configure and
build, Debug/Release CTest, both direct 206-test executables, content check and
13/13 content tests, systemic schema check and 10/10 schema tests, invalid-seed
check, static audit COUNT=0, contract check, Debug/Release smoke, 18/18
replays, 36-row scenario matrix, 8/8 save fault matrix and Release benchmark.
Benchmark output is reported with its actual `worst_1pct_avg_ms` field; it is
not called p99 and is not end-to-end 120 Hz proof.

The supplemental Audit-02B root is not complete. It has no completion flag,
`FILE_COVERAGE_PASS=NO`, 0% PASS1/PASS2/PASS3 coverage and 376 unreviewed files.
It therefore remains `NOT_COMPLETE`, not a zero-finding result. See
`docs/audit/CHAPTER01_AUDIT02B_REMEDIATION.md`.
