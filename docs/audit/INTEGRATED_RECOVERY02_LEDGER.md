# INTEGRATED RECOVERY-02 — independent truth ledger

This ledger records the first read of the live `main` worktree at
`a4c03f2a0f0b9b6a68e28aef02d3ddfedaa39871`.  It separates an existing helper,
unit seam, or document claim from a runtime capability.  `CONFIRMED` means the
live implementation was inspected and the finding is reproducible from code;
`ALREADY_FIXED` means the requested property has a real implementation and a
counter-test; `MODIFIED_FINDING` means the old wording overclaimed the current
bounded slice; `NOT_REPRODUCED` is reserved for a claim not found in the live
tree.

## Initial ledger

| Area | Status | Live evidence | Recovery consequence |
|---|---|---|---|
| Game-time pause | CONFIRMED | `PlayerModule` returns while paused, but `Engine` still ticks every module and `SimClock` | P0: pause must gate the fixed clock and all gameplay modules |
| Engine timing / presentation | CONFIRMED | `Engine::Run` renders once after a step and once after the no-step sleep iteration; render effects decrement in `RenderModule` | P0: expose distinct sim/presentation timing and prevent duplicate gameplay-duration decay |
| Projection contract | CONFIRMED | Character renderer shares focal value for walls/sprites, but interaction uses local 2-D proximity/dot tests and `IWorldQuery::LineOfSight` is not a shared target ray | P0: one bounded camera ray/target/LOS seam |
| Actor orientation | CONFIRMED | `CharacterSpriteInstance` has no actor yaw; runtime `NPCInstance::yaw` is not passed to rendering | P1: world-relative actor-facing LOD/art selection |
| Character depth / opacity | CONFIRMED | sprite drawing is per-column all-or-nothing wall occlusion and art uses spaces as the only transparency state | P1: per-cell depth and explicit transparent/glyph/opaque-empty semantics |
| Body visual truth | CONFIRMED | render skips `Dead` only; stunned runtime actors remain standing and `BodyRecord` is not the render source | P0: active actor/body disposition must select one visible representation |
| LOD boundary | CONFIRMED | `SelectCharacterLod` uses direct distance thresholds with no hysteresis | P1: prevent oscillation at thresholds |
| Scene identity / placement | CONFIRMED | B1 props and interactions are hardcoded in `composition_root.cpp`; room schema has no placed entities or portals | P0: small shared placed-entity seam before more room content |
| Non-B1 NPC population | CONFIRMED | profiles are attached only when the B1 room is loaded; non-B1 rendering adds hardcoded sprites | P0: authored population must be room-scoped and runtime-backed |
| Room transition | CONFIRMED | callbacks use coordinate thresholds and hardcoded spawn points, not authored portals | P0: bounded authored portal records/validation |
| Interaction targeting | CONFIRMED | B1 selection is 2-D proximity plus forward dot and a point LOS check; pitch and target bounds are absent | P0: target ray and bounded 3-D target volumes |
| True 3-D LOS | CONFIRMED | grid LOS accepts one `eye_z` argument and current callers reuse eye height for targets | P0: endpoint-height-aware LOS for interaction/hitscan/AI where required |
| Lean / traversal | MODIFIED_FINDING | lean state and geometry helpers exist, but render/runtime camera offset and vault/mantle execution are absent | P1: do not claim traversal; implement only if needed for the recovery slice |
| Weapon firing | CONFIRMED | `PlayerModule` consumes `action_pressed`; SMG is not held-auto; reload is not cancelled on slot switch | P0 for truthful weapon state: cadence, reload/switch policy, slot identity |
| Weapon presentation | CONFIRMED | `RenderModule` always draws the pistol and labels HUD `PISTOL`, independent of combat slot | P0: identity must match selected weapon or unavailable slots must be explicit |
| Enemy combat | MODIFIED_FINDING | guard sight-entry emits one bounded `EventPlayerDamage`; no attack cadence/tactical loop exists | Must remain documented as bounded health proof until a real bounded combat loop exists |
| AI movement | CONFIRMED | cleaner has direct cart movement; ordinary patrol/investigate states do not navigate | P0 for future NPC truth; no general planner/ECS required |
| Stealth / hearing | CONFIRMED | sight uses distance/FOV/LOS; hearing is distance-only; posture/light/cover and wall attenuation are absent | P1 for the B1 slice unless a tested counterfactual depends on it |
| Memory provenance | CONFIRMED | heard gunshot memories use player as subject/target even without visual attribution | P0: unknown heard noise must not become a direct player claim |
| Fact/event pipeline | CONFIRMED | bridge records generic systemic events, but narrative does not consume the event stream or execute storylet actions | P0: event-to-fact/action seam needed for a real slice |
| Dialogue / quest runtime | CONFIRMED | app queues a fixed diagnostic narrator line; queue output and quest transitions are not driven by gameplay facts | P0: one real B1 dialogue/objective transition, without a full quest engine |
| Narrator runtime | CONFIRMED | help key triggers visual typography only; no bounded event reaction/authority path is wired | P1 for recovery; no claim of runtime narrator intervention |
| Ending runtime | ALREADY_FIXED | ending library has deterministic four-quadrant and hidden-loop unit coverage | Foundation only; not an integrated endgame claim |
| Save transaction | CONFIRMED | app load stages some sections but switches room and commits several live sections before AI validation completes | P0: validate/stage all sections before any live mutation |
| Narrative save | CONFIRMED | app saves StoryletEngine only; dialogue/causality runtime is not part of the narrative section | P0 for resumed B1 narrative truth |
| Duplicate transient truth | CONFIRMED | `SliceRuntime` mirrors body, gate, terminal, and discovery facts beside systemic records | P0: derive milestone assertions from authoritative records/events |
| Storage bounds / duplicate IDs | ALREADY_FIXED | systemic add/deserialize paths reject duplicate IDs and bound vectors/counts; tests cover corruption | Keep as regression gate; do not replace storage with a new framework |
| Credential / body concealment | ALREADY_FIXED | systemic and runtime tests cover possession, body drag/hide/discovery response and incapacitated cleaner | Preserve; integrate only where app wiring currently bypasses it |
| Full-NPC speech | ALREADY_FIXED | sight-entry state transition plus a five-second counter-test | Preserve; no new speech architecture |
| Performance claim | MODIFIED_FINDING | benchmark exercises current systemic operations and reports `worst_1pct_avg_ms`, not strict p99 or end-to-end terminal cadence | Keep truthful labels; add only workload evidence needed for changed paths |

## Initial priority decision

The existing foundation is useful and several kernel seams are genuine, but
the integrated player-facing truth is not yet closed.  The first implementation
batch is therefore limited to: fixed-clock pause/timing; shared bounded target
and LOS geometry; body/actor visual selection; truthful weapon slot/reload
state; memory provenance; one authored room/entity/population seam; and
transactional application load.  All remaining work will be reported as
`NOT_VERIFIED` or `P1` rather than certified by helper existence or process
exit.

No new map, weapon type, renderer, ECS, release, tag, branch, PR, or Steam
operation is part of this recovery.

## Post-implementation disposition

The bounded recovery slice was implemented in the dirty worktree and checked
against the Release executable.  The statuses below describe the code that is
actually present; they do not turn the slice into a complete M1--M6 game.

| Area | Current status | Evidence / limit |
|---|---|---|
| Game-time pause and render pacing | IMPLEMENTED / VERIFIED | `RuntimeTimeGate` freezes gameplay frame while paused; `Engine` renders once after a fixed step. Covered by `engine.presents_once_per_fixed_step` and local Debug/Release CTest. |
| B1 projection / target / LOS | IMPLEMENTED for recovery slice | B1 target table uses pose, bounded facing, proximity, target centre and 3-D segment LOS. General non-B1 interaction remains older proximity wiring. |
| Actor orientation / depth / opacity / LOD hysteresis | NOT IMPLEMENTED | These remain future renderer work; no capability is claimed from the current sprite API. |
| Body visual truth | IMPLEMENTED for recovery slice | Stunned/dead runtime actors are suppressed and the exposed systemic body is the single B1 visual source; hidden bodies are not drawn. |
| Scene identity and placement | PARTIAL | A small private app table is shared by B1 interaction and rendering. It is not a general authored placed-entity/portal schema. |
| Room population | IMPLEMENTED for loaded room | Authored NPC profiles are loaded from `data/npcs/npcs.bin`, retained by spawn room, and filtered by active room. Overlapping B1 background administrator remains systemic-only for this bounded slice. |
| Room transitions | PARTIAL / VERIFIED for routes | An explicit six-entry portal table covers the current calibration/security/medical/restroom/elevator recovery routes. It is not a complete building portal model. |
| Weapon state | IMPLEMENTED / VERIFIED | SMG held-fire and reload cancellation on slot change are wired; HUD slot identity is derived from `CombatState`. Only the authored pistol viewmodel is rendered, so non-pistol art remains incomplete. |
| AI execution | PARTIAL / VERIFIED for cleaner | Cleaner movement, arrival radius, inspection delay, body discovery and response are runtime-backed. General patrol/pathfinding/tactical combat is not implemented. |
| Perception and memory | PARTIAL / VERIFIED | 3-D endpoint LOS and wall-muffled hearing are present; heard-only memory has no false player attribution; continuous stimuli refresh one semantic memory. Posture/cover/acoustic propagation remain future work. |
| Narrative / dialogue / quest | PARTIAL / VERIFIED for B1 | Storylet actions emit typed trigger events, subtitles and world commands; dialogue/causality are saved; the opening quest is transitioned through its real records. No general quest engine or event-driven narrator authority is claimed. |
| Save/load transaction | PARTIAL / VERIFIED for staged recovery load | All seven sections are parsed and preflighted before the bounded live commit; AI identity and exact room are checked. A fault-injected end-to-end rollback test for a failure during the final commit is not present. |
| Storage robustness | IMPLEMENTED / VERIFIED | Storylet, dialogue and causality loads now reject bounds, invalid IDs/enums, duplicate storylet/fired IDs, truncation and trailing compiled bytes; systemic/event tests cover their existing bounded loaders. |
| Replay assertions | IMPLEMENTED / VERIFIED | Output separates process exit, input consumption, expected state and chapter checkpoint. Success, denied, terminal-denied and badge-only Release replays require explicit state predicates. |
| Enemy combat | BOUNDED PROOF ONLY | Guard sight-entry damage and the 1F health/death probe prove an authoritative health transition. This is not complete enemy combat AI. |

### Live verification boundary

The four quick Release replays passed with explicit expected-state assertions:

* `recovery_b1_success.txt`: access, terminal session, non-lethal body,
  drag/hide, cleaner response, save/load and calibration checkpoint.
* `recovery_b1_denied.txt`: access attempted and denied; gate remains closed.
* `recovery_b1_terminal_denied.txt`: terminal attempted and denied; no session.
* `recovery_b1_badge_only.txt`: badge held by player while
  `ACCESS_ATTEMPTED=NO`.

The legacy `recovery_b1_health_death.txt` route did not encounter the current
B1 guard and therefore failed its expected health predicate when run in B1.
The same Release executable and replay, explicitly started in
`room_1f_security`, passed the bounded health/death/save/load/recovery
predicate.  This is recorded as a 1F proof, not relabeled as a B1 proof.

No foreground Windows Terminal screenshot or continuous manual video was
captured by this recovery pass.  The direct Release smoke ran under the
available non-foreground console probe (`legacy-conhost`, 80x30,
ANSI16/Compatibility), so it is process/runtime evidence only and is not
visual acceptance evidence.
