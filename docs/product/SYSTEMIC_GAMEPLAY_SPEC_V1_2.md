# SYSTEMIC GAMEPLAY SPEC v1.2 — WRITEOVER-07

This document freezes product-facing systemic semantics. Implementation
contracts are in `docs/engineering/SYSTEMIC_CONTRACTS_V1_2.md`.

## 1. Event Grammar

Every meaningful systemic action is expressed through one shared event
vocabulary. The event record carries:

- EventType
- Actor
- Target
- Location
- Timeline/frame
- Witnesses
- Severity
- Legality
- Ownership
- Method
- Outcome
- Evidence
- Tags

Events are small POD/value types, not a large inheritance hierarchy.

## 2. Crime / Law Semantics

Supported concepts:

Trespass, Theft, IllegalWeapon, Impersonation, UnauthorizedAccess, Hacking,
Bribery, Vandalism, Assault, NonLethalTakedown, Killing, EvidenceTampering.

An event is **never** converted directly into a moral "evil points" value.
Consequence is determined by context, witnesses, faction, victim, evidence,
and justification.

## 3. Witness / Observation

NPCs acquire knowledge only through:

- Direct Witness
- Heard Sound
- Hearsay
- System Feed
- Narrator Claim
- Environmental Inference
- Residual Memory

Camera, microphone, and access logs are observation sources. **Authority is
not Observability**: the Narrator may control a door without knowing where the
player is standing.

## 4. Evidence

Evidence is minimal, not forensic science. Fields:

- type
- source event
- location
- owner/subject
- visibility
- persistence
- discovered_by
- timestamp/timeline

Examples: visible body, unconscious body, blood, shell casing, broken glass,
broken lock, camera outage, terminal login, door access log, stolen
credential, weapon provenance, CCTV record.

## 5. Body System

States: Alive, Injured, Unconscious, Dead.

Player actions: Search, Drag, Hide, CheckCondition.

Loot from a body is inventory-driven: weapon, ammo, cash, badge, medical,
key item.

## 6. Body Drag

Minimum kernel: player interaction target = body.

Drag rules:

- reduced movement speed
- sprint disabled
- weapon use restricted
- noise generated

No ragdoll physics. Body moves via entity/grid occupancy.

## 7. Conceal Body

`HideableContainer` semantic includes:

- capacity
- concealment
- accessibility
- routine_tags
- current_occupants

Allowed hideable categories: industrial refuse bin, cleaning cart, laundry
cart, large locker, vehicle trunk, maintenance compartment, restroom stall,
sealed room.

Large-body size checks are mandatory. A normal small trash bin cannot contain
an adult body.

## 8. Delayed Discovery

Hiding a body **never deletes the entity**. The container remains part of
WorldState. When a cleaner, maintenance worker, security officer, owner, or
routine user accesses the container, `DiscoverBody` may trigger.

After discovery:

- WorldEvent
- Memory
- Fear
- Claim
- Alert

Propagation is natural, not instant global omniscience.

The automatic regression test `SYSTEMIC_BODY_CONCEALMENT_CHAIN` covers:
incapacitate guard → hide in cleaning cart → patrol does not see body →
cleaner opens cart later → discovery → memory → report/claim → local alert →
stolen badge remains in inventory → save/load consistency.

## 9. Item Ownership / Provenance

Important items support:

- ItemId
- ItemType
- OwnerId
- Issuer/Faction
- LegalHolder
- ReportedStolen
- CredentialLevel
- ProvenanceTags

Generic items (ammo, generic bandage) need not track individual ownership.
Badges, guns, and key items must.

Typical rule: a stolen badge still opens a compatible reader, but an NPC who
personally knows the original owner may identify the mismatch.

## 10. Search / Containers

Unified interaction verbs for search person/body/desk/locker/bag/vehicle/
cabinet/container.

Search has duration, visibility/noise, and ownership consequences. No bespoke
gameplay code per container type.

## 11. Trading / Giving / Bribery

Unified Social Exchange kernel supports:

Give, Request, Trade, Bribe, AskFavor.

Acceptance depends on personality, need, risk, amount/value, relationship,
fear, debt, faction, and witness presence.

No per-NPC shop UI. NPCs may accept money and still betray the player.

## 12. Information Economy

`KnowledgeAsset` is a first-class non-item asset:

- password
- username
- route
- shift schedule
- camera blind spot
- employee name
- safe combination
- access procedure
- secret

It can be known, told, lied about, traded, and remembered.

## 13. Promise / Favor

Promise state:

- PromiseId
- giver
- receiver
- subject
- accepted_time
- optional deadline
- status

Statuses: Offered, Accepted, Fulfilled, Broken, Expired, Cancelled.

Promise outcome affects Reliability, Trust, Debt, Storylet eligibility, and
ending state.

Regression test `SYSTEMIC_PROMISE_CHAIN` verifies persistence through
save/load and correct social/memory changes on failure or fulfilment.

## 14. Factions

Core factions:

- GeneralStaff
- Security
- Medical
- Research
- Maintenance

Executive/Detained may be added as content factions later.

Personal relationships are separate from faction relations. A Security member
may personally trust the player.

## 15. NPC Routine

Routine semantics: Work, Patrol, Break, Restroom, Eat, Talk, Repair, Clean,
Medical, Travel, Hide, EmergencyAssignment.

No complex Sims scheduler. Low-frequency routine scheduler. Alert/Storylet can
override routine.

## 16. Communication

Channels: DirectSpeech, Radio, Phone, PA, TerminalMessage, SystemLog, Rumor.

Every communication has source, recipient, latency, provenance.

One person seeing a crime must not cause the whole building to know in the
same tick.

## 17. Alert Escalation

Formal levels:

0 NORMAL
1 SUSPICIOUS
2 LOCAL_ALERT
3 SEARCH
4 LOCKDOWN
5 CRITICAL

Alert has regional scope. Global Alert does not mean every NPC already knows
every fact.

## 18. Sound

SoundEvent includes type, position, loudness, source, tags.

Types: footstep, sprint, gunshot, suppressed_shot, glass, explosion, voice,
door, decoy.

Propagation uses distance, door/wall attenuation, and room connectivity. No
real acoustical ray tracing.

## 19. Power / Infrastructure

Circuit graph supports Lighting, Camera, Door, Elevator, Ventilation, Medical,
Server, Alarm.

States: On, Off, Damaged, Backup.

Controls: manual breaker, terminal control, physical repair. No real
electrical simulation.

## 20. Repair

Broken cameras, lights, fuses, door controllers, and terminals can be
repaired by repair-capable NPCs if they later reach them.

World persistence is not world stagnation.

## 21. Fire / Smoke

Only small hazard zones are allowed. Explosion/short circuit may create
smoke, sparks, temporary visibility reduction, and alarm.

No large continuous cellular fire simulation unless performance proves it is
extremely cheap.

## 22. Injury

Player and NPC support HP, Armor, Bleeding, Stun, Unconscious.

Medical: Bandage, Hemostatic, FirstAidKit, MedicalStation.

Abstract Energy is cancelled. Devices use Battery Pack / Charge and similar
realistic resources.

## 23. Weapons

Archetypes: Service Pistol, Compact SMG, Security Carbine, Pump Shotgun,
Marksman Rifle, Taser, Baton.

Throwables: Sound Decoy, Flashbang, Smoke, Frag (rare).

Carry limit: 1 sidearm, 2 long guns, 1 device, throwables/items. No infinite
weapon wheel.

## 24. Loot Ecology

Authoring spawn pools are per zone/actor class.

- Security: weapon, ammo, vest, cash, bandage
- Medical: medical
- Office: cash, badge, logs, personal item
- Maintenance: tools, battery, fuse

No random cross-theme loot. No natural respawn.

## 25. Requisition Terminal

Diegetic Requisition Terminal, not a traditional RPG shop.

Sells: ammo, medical, weapon attachment, battery, decoy, flash/smoke, limited
supplies.

Weapons and armor come mainly from lockers, armory, NPCs, rooms, or story
rewards.

## 26. Cash

Cash is for private trade and bribery. No complex economy. Only amount, value
judgement, and NPC willingness.

## 27. Terminal / Hacking

Access requires known credentials, stolen credentials, unlocked session,
password clue, maintenance account, NPC login, or physical service terminal.

System functions: door, camera, logs, alarm, elevator, user account,
clearance.

Unauthorized use can leave an AuditLog. There is no "Press H to hack".

## 28. Quest Lifecycle

States: Offered, Accepted, Active, Completed, Failed, Expired, Abandoned,
Betrayed.

Presentation Objective is separate from True Quest State. The Narrator can
change display, but cannot erase failed facts.

## 29. Facility Scheduling

Incident phases: INITIAL_CHAOS, LOCAL_RESPONSE, SECURITY_SEARCH,
FACILITY_LOCKDOWN, SYSTEMIC_CRISIS.

Player events can accelerate, delay, skip, or modify phases. Not a pure
wall-clock switch.

## 30. Exploration

Building is B4–B1, 1F–36F, Roof.

Topology includes elevator, stairs, maintenance shaft, external maintenance
path.

Playable target: 12–14 regions, not 41 full levels.

## 31. Secret Tiers

- EASTER_EGG
- CACHE_SECRET
- DEEP_SECRET

Deep Secrets may affect Truth, SelfKnowledge, and Meta Ending. Ordinary
easter eggs must not all affect endings.

## 32. Narrator Observability

NarratorAuthority and NarratorObservability are separate.

Observability sources: camera, microphone, access reader, terminal, radio,
NPC report, timeline anomaly.

If the player cuts camera/network, the Narrator may genuinely not see.
Meta-level observability appears only later.

## 33. Narrator Intervention Rules

Narrator has high authority, but with:

- AuthorityStage
- InterventionCost
- InterventionCooldown / narrative constraint

Major abilities: Kill, ForcedRespawn, Rewind, SaveManipulation.

These cannot be spammed. In-fiction reasons: experiment integrity, system
safety, authority escalation, limited intervention window.

## 34. Save Fiction

VisibleNarrativeSave and ProtectedRecovery remain fully separate.

The Narrator can control the former. It can never irreversibly delete the last
recoverable progress.

## 35. Failure Recovery

Every main objective has at least 2 independent solution chains.

No single NPC death, lost badge, or destroyed circuit should make the game
impossible. A specific route may be permanently closed.

## 36. World Persistence

Within a timeline, the following persist:

- broken glass
- body / unconscious NPC
- dropped weapon
- searched container
- stolen item
- power state
- door state
- blood/evidence
- NPC location
- relationship
- memory

Only world actions (cleanup, repair, NPC movement, timeline reset) change
them.

## 37. Difficulty

Difficulty changes AI reaction, hearing, search quality, coordination,
resource scarcity, aim, and medical/ammo availability.

It does **not** mean enemy HP × 5.

## 38. Tutorialization

Room/B1/1F naturally teaches move, look, shoot, interact, search, credential,
stealth, body, terminal, and NPC exchange.

Later systems are introduced one at a time when needed. No tutorial dump.

## 39. Visual Feedback

Each systemic mechanic must document:

- What player sees
- What player hears
- What HUD shows
- What later consequence confirms

Example Hide Body:

- Now: body disappears from LOS but remains container state
- Later: cleaner animation, container opens, shout/radio, local alert change

No "STEALTH +10" feedback.

## 40. Implementation Scope This Round

Only the cross-module minimum kernel is implemented:

A. SystemicEvent schema
B. Actor/Identity core metadata
C. Item/Ownership/Provenance core metadata
D. BodyState + HideableContainer semantic kernel
E. Evidence record
F. SocialState / GlobalPlayerState
G. Relationship minimal storage
H. Promise minimal state
I. Alert level
J. Narrator Authority vs Observability state
K. World persistence save serialization
L. Data authoring schema / validators

## 41. Final Correction Freeze

- Actor identity separates `CognitionTier`, `Faction`, and `Role`.
- Relationships are directed and debt direction is `A feels indebted to B`.
- Promise and Quest use explicit FSM transitions.
- Alert canonical owner is `AlertState`; evidence canonical owner is the evidence collection.
- DiscoverBody creates observation only; response is typed and explicit.
- Narrator authority and observability are separate; observability is per ObservationSource.
- Search, item lifecycle, social exchange, knowledge assets, quests, and terminal access are typed contract seams, not full gameplay systems.
- Systemic save is fail-closed.
- Seed JSON compiles to binary and is loaded by the runtime.
