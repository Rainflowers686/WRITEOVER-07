# SYSTEMIC CONTRACTS v1.2 — WRITEOVER-07

## 1. Scope

This is the engineering contract for the minimum systemic kernel added in
v1.2. It does not replace existing Height-Span, Save, EventBus, Terminal,
Input, or Content compiler contracts.

## 2. Public Header

Primary header: `include/writeover/systemic/systemic.h`

Library: `writeover_systemic`

Dependency rule: `writeover_systemic` depends only on `writeover_common`
(IDs, serialization, basic value types). It must not create cycles with AI,
narrative, world, player, or render.

## 3. Core Types

### SystemicEvent

POD event record with:

- EventId
- SystemicEventType
- Actor / Target
- RoomId / frame
- Witnesses
- Severity
- LegalityClass
- Owner
- Method (string)
- Outcome
- Evidence IDs
- Tags

### ActorRecord

Identity core:

- NpcId
- ResourceId data_key
- Faction
- ActorClass (Full / SemiHuman / Guard)
- occupation
- full_human_illusion
- known_identities (for identity mismatch detection)
- personality_tags

### ItemRecord

Ownership/provenance:

- ItemId
- ItemType
- Owner / Issuer / LegalHolder / CurrentHolder
- reported_stolen
- credential_level
- provenance_tags

Stolen badges still pass physical readers; identity detection is a social
layer, not a reader layer.

### BodyRecord / HideableContainer

Body: status, disposition, position, room, container, searched.

HideableContainer: kind, capacity_volume, concealment, accessibility,
routine_tags, current_occupants.

Body is never deleted by hiding. Discovery moves disposition back to Exposed
and generates memory/evidence/event/alert.

### EvidenceRecord

Minimal evidence: type, source_event, subject, owner, room, position,
visibility, persists, frame, discovered_by.

### MemoryRecord / RelationshipRecord / PromiseRecord

Memory: actor, kind, subject, target, room, frame, salience, confidence,
source, text_key, tags.

Relationship: EntityId pair + trust/fear/respect/suspicion/debt/attachment/
ideological_alignment.

Promise: giver, receiver, subject, accepted_frame, deadline_frame, status,
storylet_eligible.

### GlobalPlayerState / AlertState / NarratorObservabilityState

GlobalPlayerState holds the frozen social/meta axes.

AlertState holds FacilityAlertLevel + scoped rooms + last_change_frame.

NarratorObservabilityState separates AuthorityStage/InterventionCost/Cooldown
from observability sources and late-game meta observability.

## 4. Save Integration

`SaveSectionId::Systemic = 7` was added as a minimal forward-compatible
extension.

`SystemicWorld::Serialize()` / `Deserialize()` produce the section payload.
The section can be embedded in a standard `.wo07` save file alongside
existing sections. Existing sections remain unchanged.

## 5. Kernel Tests

Registered as `systemic.*`:

- `systemic.body_concealment_chain`
- `systemic.stolen_identity_chain`
- `systemic.promise_chain`
- `systemic.save_section_round_trip`

These tests exercise the real kernel and real serialization, not mocks.

## 6. Performance Contract

`SYSTEMIC_BENCH` in `writeover_bench` uses the real kernel with 25 NPCs,
500 evidence, 500 memories, 100 events, 30 containers, and 100 items at 120Hz
cadence. Budget: Simulation p99 <= 2.0 ms.

## 7. Module Boundaries

| Module | Must use |
|--------|----------|
| M1 Core | owns save section wrapping, Protected Recovery, timeline |
| M3 Player | reads BodyRecord/ItemRecord, writes via interaction verbs |
| M4 World | owns containers, infrastructure, loot ecology; reads systemic IDs |
| M5 AI | owns NPC memory/relationships/witness logic; writes social records |
| M6 Narrative | owns promises, Claims, storylets, Narrator policy; reads evidence/state |

No module may create a second body state, second alert level, or second
ownership system.
