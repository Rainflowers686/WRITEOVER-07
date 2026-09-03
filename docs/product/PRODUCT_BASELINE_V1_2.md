# PRODUCT BASELINE v1.2 — WRITEOVER-07

> **SUPERSEDES PRODUCT_BASELINE_V1_1**
>
> v1.1 remains in the repository as history and is not deleted. v1.2 does not
> revoke the technical foundation (Height-Span, Save, EventBus, Terminal,
> Input, Content compiler); it expands the product contract from "runtime
> immersive FPS framework" to "Systemic Gameplay Foundation".

## 0. Scope of v1.2

This round is **not formal content production**.

Deliberately excluded from this round:

- 25 complete identity-bearing NPC implementations
- 36 fully playable floors
- final story dialogue
- all weapon animations
- final visual asset production

Included in this round:

- product/spec/docs
- core contracts
- minimal trusted kernel
- data schemas
- kernel tests and benchmark
- save persistence for systemic state

## 1. Highest Product Priorities

1. **Visual Identity**
2. **Frame pacing / 120Hz performance**
3. **Game feel**
4. **Systemic player freedom**
5. **NPC social memory**
6. **Narrative sovereignty**
7. **Content quantity**

Content quantity is deliberately last. The current `ReferenceRenderer` is
**not** final art; it is a technical reference / test raster only.

## 2. Visual Direction

Final normal-world presentation: **Half-block TrueColor Pixel Framebuffer**.

- Each terminal cell is `▀`
- foreground RGB = upper logical pixel
- background RGB = lower logical pixel
- ULTRA: 240×67 terminal cells = 240×134 logical pixels
- Normal world is a pixel FPS
- Character/text corruption is reserved for Narrator intrusion, glitch, HUD,
  and terminal contexts

See `VISUAL_BIBLE_V1_2.md`.

## 3. Building

The diegetic building is B4/B3/B2/B1 + 1F–36F + Roof.

It exists because of elevator, directory, radio, logs, windows, NPC
dialogue/routine, lockdown, and world-state references — not because every
floor is a full level.

v1.2 target scope:

- 12–14 playable regions
- 6–7 signature gameplay spaces
- ≈10 secret spaces

See `BUILDING_40_LEVEL_DIEGESIS.md`.

## 4. NPC Population

- ≈23–27 identity-bearing NPCs
- 5 Full-Human illusion NPCs
- All other identity-bearing NPCs are at least Semi-Human

No "3 real NPCs + 20 scripted puppets".

### Full-Human NPC

Persistent memory, belief, relationship vector, knowledge provenance,
utility, GOAP-lite/equivalent small planner, needs, personal goals, routine,
independent action.

### Semi-Human NPC

Identity, occupation, personality, need, fear, trust, suspicion, debt,
belief, 4–12 memories, routine, small goals, inventory. Ordinary guards are
Semi-Human.

## 5. Relationship Vector

Full NPCs support:

Trust, Fear, Respect, Suspicion, Debt, Attachment, IdeologicalAlignment.

Semi-Human NPCs at minimum support:

Trust, Fear, Suspicion, Debt.

Raw numbers are not shown in normal UI. F3 debug may show exact values.

## 6. Player / Social / Meta State

Frozen semantic axes include:

- Humanity, Violence, Reliability, Coercion
- PublicTrust, SecurityStanding, MedicalResearchStanding, MaintenanceStanding
- NarratorAlignment, NarratorDominance, Autonomy
- TruthExposure, SelfKnowledge
- FacilityAlert, InfrastructureIntegrity
- TimelineInstability, ResidualMemoryPressure
- CivilianCasualties, SecurityCasualties, PromisesBroken
- EvidenceSet

Most are hidden from normal play. F3 may show exact values.

Endings are driven by **Truth × NarratorDominance** for four Macro Endings;
other variables drive ending variation, NPC epilogue, building outcome, and
social outcome. See `STATE_ENDING_MODEL_V1_2.md`.

## 7. Systemic Kernel Ownership

Cross-module state must use the contracts in
`docs/engineering/SYSTEMIC_CONTRACTS_V1_2.md`. Duplicate state in multiple
modules is forbidden.

| Module | Owns |
|--------|------|
| M1 Core | save, timeline, persistence, global state, Protected Recovery |
| M2 Render | pixel framebuffer, sprites, viewmodels, lighting, effects, HUD, subtitle, Narrator typography, visual performance |
| M3 Player | interaction verbs, movement, combat, inventory, body dragging, weapon handling, throwables |
| M4 World | building topology, containers, doors, power, terminals, ownership, loot ecology, environment interaction |
| M5 AI | NPC identity, perception, routine, memory, relationship, witness, communication, social decision, body discovery |
| M6 Narrative | quests, promises semantic consequences, Claim, Narrator authority policy, observability policy, Storylets, ending state |

## 8. Hard Non-Goals for This Round

No full AI planner, no full shop UI, no full hacking UI, no full trading UI,
no complete 36F maps, no full weapon set, no complete NPC schedules.

## 9. v1.1 Historical Note

`PRODUCT_BASELINE_V1_1.md` is preserved as the prior frozen baseline. It is
superseded by this document and should not be used as the active product input.

## 10. Final Correction Freeze

- Runtime owns exactly one `SystemicWorld`.
- Cognition/Role separated from Faction.
- Directed relationships with canonical ranges.
- Fail-closed systemic deserialization.
- Systemic section included in runtime save/load.
- Seed JSON compiles to binary and loads at runtime.
- Full floor directory, ending model, visual production contract, and active product sources frozen.
