# WRITEOVER-07 — Recovery-04 Capability Truth Matrix

This is the current bounded capability receipt for final receipt head
`b6dacee155b1af3492f48eb02f0030dc7cc82ebe` (source implementation head
`b7cea9cca387f30a4b4d81b9b9f3d81f186c7859`). The final receipt adds only
portable parentheses to an existing test assertion. `VERIFIED` means that the live
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

The final GitHub Actions run for this receipt is `34316270370` and completed
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
