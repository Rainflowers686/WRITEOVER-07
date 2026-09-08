# WRITEOVER-07 — Integrated Recovery-02 Capability Truth Matrix

This matrix describes the live bounded recovery implementation.  `VERIFIED`
means a code path and an assertion or gate exercised it. `PARTIAL` means the
recovery slice has a real seam but the general product capability is not
implemented. `NOT_VERIFIED` is deliberately not a synonym for PASS.

| Capability | Status | What is real now | Boundary / not claimed |
|---|---|---|---|
| PlayerModule | VERIFIED / PARTIAL | Fixed-step movement, mouse look, posture, pause gate, health/death authority, weapon selection and reload policy | Lean/traversal are not a complete movement feature. |
| WorldModule | VERIFIED / PARTIAL | Loaded room query, B1 infrastructure door, facts, typed world commands and bounded room reload | General world simulation and authored portal data are outside this slice. |
| AiModule | VERIFIED / PARTIAL | Active-room profile population, perception, semantic memory refresh, cleaner approach/arrival/inspection/discovery/response | No general navigation planner, stealth model, or tactical enemy combat. |
| NarrativeModule | VERIFIED / PARTIAL | Storylet selection from a real fact, typed trigger, narrator/dialogue subtitle actions, world command sink, dialogue and causality persistence | No complete M6 quest graph or runtime narrator authority loop. |
| RenderModule | VERIFIED / PARTIAL | Existing Character-Art renderer reads active-room runtime actors/bodies, real health and dialogue sources, and respects the active weapon identity | Actor yaw/depth/opacity and LOD hysteresis remain future renderer work; only pistol art exists. |
| Save/Load | VERIFIED / PARTIAL | Seven sections are written; load validates/stages player, world, RNG, events, narrative, AI and systemic state; exact room is probed before commit | No fault injection during the final live commit has been performed. |
| Room switch | VERIFIED / PARTIAL | Explicit bounded portal records cover the current recovery routes and update world/player/AI/render room identity together | Not a building-wide room/portal schema. |
| B1 gameplay | VERIFIED | Release success route reaches calibration; denied, terminal-denied and badge-only counterfactuals have independent assertions | This is a small recovery slice, not a finished level. |
| 1F gameplay | VERIFIED / BOUNDED | Real guard population is loaded in `room_1f_security`; the Release health/death replay passes there | Health proof is not complete enemy combat. |
| Interaction | VERIFIED / PARTIAL | B1 uses a shared private scene-entity table plus target identity, facing, range and 3-D LOS | Non-B1 callbacks still contain bounded legacy proximity interactions. |
| Replay | VERIFIED | Output separates process exit, input consumed, expected state and chapter checkpoint; four B1 replay predicates pass | No claim that an arbitrary replay file is a gameplay oracle. |
| HUD / subtitle | VERIFIED / PARTIAL | Health/ammo/weapon label, compact subtitle, dialogue queue and F3-only developer overlay are wired | Foreground visual acceptance was not performed in this pass. |
| Body logic | VERIFIED / PARTIAL | Runtime non-lethal hit creates an unconscious body; search/badge transfer/drag/update/end/hide/cleaner discovery are connected | Broader body/evidence authoring is not implemented. |
| Narrator visual | VERIFIED / PARTIAL | Help path renders bounded typography; storylet narrator lines enter the dialogue queue | No event-driven authority/observability gameplay loop is claimed. |
| NPC population | VERIFIED / PARTIAL | NPC profiles are compiled and loaded by authored spawn room; active room filters runtime behavior and rendering | No full multi-floor population model. |

## Truth rules preserved

* `ReaderAcceptsItem()` validates the credential; the B1 interaction layer
  separately requires `ItemHeldBy(badge, player)`.
* Body discovery is arrival-gated and response is a separate decision. A due
  frame alone cannot discover a body.
* Heard noise has a location and semantic tag but no direct player attribution
  without sight.
* Continuous sight/noise refreshes a bounded semantic memory instead of
  creating one identical durable record per 10-Hz decision tick.
* The current guard damage path is explicitly a bounded authoritative health
  proof, not complete enemy combat.
* The current direct Release smoke reports `legacy-conhost`, 80x30,
  `ANSI16`, `COMPATIBILITY` under the non-foreground probe. It is runtime
  boot evidence, not a Windows Terminal visual acceptance result.

## Remaining implementation boundary

The matrix intentionally leaves the following for later scoped work: general
actor orientation/depth/opacity/LOD hysteresis, authored room entities and
portals, general navigation and tactical combat, full quest/narrator runtime,
and foreground visual/video evidence.  None is silently certified by the
passing unit or content gates.
