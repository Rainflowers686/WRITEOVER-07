#include "writeover/systemic/systemic.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace writeover {

namespace {

void WriteVec3(Serializer& s, const Vec3& v) {
    s.WriteF32(v.x);
    s.WriteF32(v.y);
    s.WriteF32(v.z);
}

Vec3 ReadVec3(Deserializer& d) {
    Vec3 v;
    v.x = d.ReadF32();
    v.y = d.ReadF32();
    v.z = d.ReadF32();
    return v;
}

template <typename T>
void WriteStrings(Serializer& s, const std::vector<T>& values) {
    s.WriteU32(static_cast<uint32_t>(values.size()));
    for (const auto& v : values) {
        s.WriteString(v);
    }
}

std::vector<std::string> ReadStrings(Deserializer& d) {
    const uint32_t n = d.ReadU32();
    std::vector<std::string> out;
    out.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        out.push_back(d.ReadString());
    }
    return out;
}

template <typename Id>
void WriteIdVector(Serializer& s, const std::vector<Id>& values) {
    s.WriteU32(static_cast<uint32_t>(values.size()));
    for (const auto& v : values) {
        WriteId(s, v);
    }
}

template <typename Id>
std::vector<Id> ReadIdVector(Deserializer& d) {
    const uint32_t n = d.ReadU32();
    std::vector<Id> out;
    out.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        out.push_back(ReadId<Id>(d));
    }
    return out;
}

void WriteActor(Serializer& s, const ActorRecord& a) {
    WriteId(s, a.id);
    WriteId(s, a.data_key);
    s.WriteU8(static_cast<uint8_t>(a.faction));
    s.WriteU8(static_cast<uint8_t>(a.actor_class));
    WriteId(s, a.occupation);
    s.WriteU8(a.full_human_illusion ? 1 : 0);
    WriteIdVector(s, a.known_identities);
    WriteStrings(s, a.personality_tags);
}

ActorRecord ReadActor(Deserializer& d) {
    ActorRecord a;
    a.id = ReadId<NpcId>(d);
    a.data_key = ReadId<ResourceId>(d);
    a.faction = static_cast<Faction>(d.ReadU8());
    a.actor_class = static_cast<ActorClass>(d.ReadU8());
    a.occupation = ReadId<StringId>(d);
    a.full_human_illusion = d.ReadU8() != 0;
    a.known_identities = ReadIdVector<EntityId>(d);
    a.personality_tags = ReadStrings(d);
    return a;
}

void WriteItem(Serializer& s, const ItemRecord& i) {
    WriteId(s, i.id);
    s.WriteU8(static_cast<uint8_t>(i.type));
    WriteId(s, i.owner);
    WriteId(s, i.issuer);
    WriteId(s, i.legal_holder);
    WriteId(s, i.current_holder);
    s.WriteU8(i.reported_stolen ? 1 : 0);
    s.WriteU8(i.credential_level);
    WriteStrings(s, i.provenance_tags);
}

ItemRecord ReadItem(Deserializer& d) {
    ItemRecord i;
    i.id = ReadId<ItemId>(d);
    i.type = static_cast<ItemType>(d.ReadU8());
    i.owner = ReadId<EntityId>(d);
    i.issuer = ReadId<EntityId>(d);
    i.legal_holder = ReadId<EntityId>(d);
    i.current_holder = ReadId<EntityId>(d);
    i.reported_stolen = d.ReadU8() != 0;
    i.credential_level = d.ReadU8();
    i.provenance_tags = ReadStrings(d);
    return i;
}

void WriteBody(Serializer& s, const BodyRecord& b) {
    WriteId(s, b.id);
    WriteId(s, b.npc);
    s.WriteU8(static_cast<uint8_t>(b.status));
    s.WriteU8(static_cast<uint8_t>(b.disposition));
    WriteVec3(s, b.position);
    WriteId(s, b.room);
    WriteId(s, b.container);
    s.WriteU8(b.searched ? 1 : 0);
    s.WriteU8(b.has_weapon ? 1 : 0);
}

BodyRecord ReadBody(Deserializer& d) {
    BodyRecord b;
    b.id = ReadId<EntityId>(d);
    b.npc = ReadId<NpcId>(d);
    b.status = static_cast<BodyStatus>(d.ReadU8());
    b.disposition = static_cast<BodyDisposition>(d.ReadU8());
    b.position = ReadVec3(d);
    b.room = ReadId<RoomId>(d);
    b.container = ReadId<ContainerId>(d);
    b.searched = d.ReadU8() != 0;
    b.has_weapon = d.ReadU8() != 0;
    return b;
}

void WriteContainer(Serializer& s, const HideableContainer& c) {
    WriteId(s, c.id);
    s.WriteU8(static_cast<uint8_t>(c.kind));
    WriteVec3(s, c.position);
    WriteId(s, c.room);
    s.WriteF32(c.capacity_volume);
    s.WriteU8(c.concealment);
    s.WriteU8(c.accessibility);
    s.WriteU32(static_cast<uint32_t>(c.routine_tags.size()));
    for (const auto tag : c.routine_tags) {
        s.WriteU8(static_cast<uint8_t>(tag));
    }
    WriteIdVector(s, c.current_occupants);
}

HideableContainer ReadContainer(Deserializer& d) {
    HideableContainer c;
    c.id = ReadId<ContainerId>(d);
    c.kind = static_cast<ContainerKind>(d.ReadU8());
    c.position = ReadVec3(d);
    c.room = ReadId<RoomId>(d);
    c.capacity_volume = d.ReadF32();
    c.concealment = d.ReadU8();
    c.accessibility = d.ReadU8();
    const uint32_t n = d.ReadU32();
    for (uint32_t i = 0; i < n; ++i) {
        c.routine_tags.push_back(static_cast<RoutineTag>(d.ReadU8()));
    }
    c.current_occupants = ReadIdVector<EntityId>(d);
    return c;
}

void WriteEvidence(Serializer& s, const EvidenceRecord& e) {
    WriteId(s, e.id);
    s.WriteU8(static_cast<uint8_t>(e.type));
    WriteId(s, e.source_event);
    WriteId(s, e.subject);
    WriteId(s, e.owner);
    WriteId(s, e.room);
    WriteVec3(s, e.position);
    s.WriteF32(e.visibility);
    s.WriteU8(e.persists ? 1 : 0);
    s.WriteU64(e.frame);
    WriteIdVector(s, e.discovered_by);
}

EvidenceRecord ReadEvidence(Deserializer& d) {
    EvidenceRecord e;
    e.id = ReadId<EvidenceId>(d);
    e.type = static_cast<EvidenceType>(d.ReadU8());
    e.source_event = ReadId<EventId>(d);
    e.subject = ReadId<EntityId>(d);
    e.owner = ReadId<EntityId>(d);
    e.room = ReadId<RoomId>(d);
    e.position = ReadVec3(d);
    e.visibility = d.ReadF32();
    e.persists = d.ReadU8() != 0;
    e.frame = d.ReadU64();
    e.discovered_by = ReadIdVector<NpcId>(d);
    return e;
}

void WriteMemory(Serializer& s, const MemoryRecord& m) {
    WriteId(s, m.id);
    WriteId(s, m.npc);
    s.WriteU8(static_cast<uint8_t>(m.kind));
    WriteId(s, m.subject);
    WriteId(s, m.target);
    WriteId(s, m.room);
    s.WriteU64(m.frame);
    s.WriteF32(m.salience);
    s.WriteF32(m.confidence);
    s.WriteU8(static_cast<uint8_t>(m.source));
    WriteId(s, m.text_key);
    WriteStrings(s, m.tags);
}

MemoryRecord ReadMemory(Deserializer& d) {
    MemoryRecord m;
    m.id = ReadId<MemoryId>(d);
    m.npc = ReadId<EntityId>(d);
    m.kind = static_cast<MemoryKind>(d.ReadU8());
    m.subject = ReadId<EntityId>(d);
    m.target = ReadId<EntityId>(d);
    m.room = ReadId<RoomId>(d);
    m.frame = d.ReadU64();
    m.salience = d.ReadF32();
    m.confidence = d.ReadF32();
    m.source = static_cast<KnowledgeSource>(d.ReadU8());
    m.text_key = ReadId<ResourceId>(d);
    m.tags = ReadStrings(d);
    return m;
}

void WriteRelationship(Serializer& s, const RelationshipRecord& r) {
    WriteId(s, r.a);
    WriteId(s, r.b);
    s.WriteF32(r.trust);
    s.WriteF32(r.fear);
    s.WriteF32(r.respect);
    s.WriteF32(r.suspicion);
    s.WriteF32(r.debt);
    s.WriteF32(r.attachment);
    s.WriteF32(r.ideological_alignment);
}

RelationshipRecord ReadRelationship(Deserializer& d) {
    RelationshipRecord r;
    r.a = ReadId<EntityId>(d);
    r.b = ReadId<EntityId>(d);
    r.trust = d.ReadF32();
    r.fear = d.ReadF32();
    r.respect = d.ReadF32();
    r.suspicion = d.ReadF32();
    r.debt = d.ReadF32();
    r.attachment = d.ReadF32();
    r.ideological_alignment = d.ReadF32();
    return r;
}

void WritePromise(Serializer& s, const PromiseRecord& p) {
    WriteId(s, p.id);
    WriteId(s, p.giver);
    WriteId(s, p.receiver);
    s.WriteString(p.subject);
    s.WriteU64(p.accepted_frame);
    s.WriteU64(p.deadline_frame);
    s.WriteU8(static_cast<uint8_t>(p.status));
    s.WriteU8(p.storylet_eligible ? 1 : 0);
}

PromiseRecord ReadPromise(Deserializer& d) {
    PromiseRecord p;
    p.id = ReadId<PromiseId>(d);
    p.giver = ReadId<EntityId>(d);
    p.receiver = ReadId<EntityId>(d);
    p.subject = d.ReadString();
    p.accepted_frame = d.ReadU64();
    p.deadline_frame = d.ReadU64();
    p.status = static_cast<PromiseStatus>(d.ReadU8());
    p.storylet_eligible = d.ReadU8() != 0;
    return p;
}

void WriteEvent(Serializer& s, const SystemicEvent& e) {
    WriteId(s, e.id);
    s.WriteU8(static_cast<uint8_t>(e.type));
    WriteId(s, e.actor);
    WriteId(s, e.target);
    WriteId(s, e.location);
    s.WriteU64(e.frame);
    WriteIdVector(s, e.witnesses);
    s.WriteU8(e.severity);
    s.WriteU8(static_cast<uint8_t>(e.legality));
    WriteId(s, e.owner);
    s.WriteString(e.method);
    s.WriteU8(static_cast<uint8_t>(e.outcome));
    WriteIdVector(s, e.evidence);
    WriteStrings(s, e.tags);
}

SystemicEvent ReadEvent(Deserializer& d) {
    SystemicEvent e;
    e.id = ReadId<EventId>(d);
    e.type = static_cast<SystemicEventType>(d.ReadU8());
    e.actor = ReadId<EntityId>(d);
    e.target = ReadId<EntityId>(d);
    e.location = ReadId<RoomId>(d);
    e.frame = d.ReadU64();
    e.witnesses = ReadIdVector<NpcId>(d);
    e.severity = d.ReadU8();
    e.legality = static_cast<LegalityClass>(d.ReadU8());
    e.owner = ReadId<EntityId>(d);
    e.method = d.ReadString();
    e.outcome = static_cast<OutcomeType>(d.ReadU8());
    e.evidence = ReadIdVector<EvidenceId>(d);
    e.tags = ReadStrings(d);
    return e;
}

template <typename T>
void WriteRecordVector(Serializer& s, const std::vector<T>& values,
                       void (*write_one)(Serializer&, const T&)) {
    s.WriteU32(static_cast<uint32_t>(values.size()));
    for (const auto& v : values) {
        write_one(s, v);
    }
}

template <typename T>
std::vector<T> ReadRecordVector(Deserializer& d, T (*read_one)(Deserializer&)) {
    const uint32_t n = d.ReadU32();
    std::vector<T> out;
    out.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        out.push_back(read_one(d));
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// SystemicWorld implementation
// ---------------------------------------------------------------------------

void SystemicWorld::AddActor(const ActorRecord& actor) {
    actors_.push_back(actor);
}

const ActorRecord* SystemicWorld::GetActor(NpcId id) const {
    for (const auto& a : actors_) {
        if (a.id == id) {
            return &a;
        }
    }
    return nullptr;
}

ItemId SystemicWorld::AddItem(const ItemRecord& item) {
    items_.push_back(item);
    return item.id;
}

const ItemRecord* SystemicWorld::GetItem(ItemId id) const {
    for (const auto& i : items_) {
        if (i.id == id) {
            return &i;
        }
    }
    return nullptr;
}

void SystemicWorld::SetItemHolder(ItemId id, EntityId holder) {
    for (auto& i : items_) {
        if (i.id == id) {
            i.current_holder = holder;
            return;
        }
    }
}

void SystemicWorld::MarkReportedStolen(ItemId id) {
    for (auto& i : items_) {
        if (i.id == id) {
            i.reported_stolen = true;
            return;
        }
    }
}

ContainerId SystemicWorld::AddContainer(const HideableContainer& container) {
    containers_.push_back(container);
    return container.id;
}

const HideableContainer* SystemicWorld::GetContainer(ContainerId id) const {
    for (const auto& c : containers_) {
        if (c.id == id) {
            return &c;
        }
    }
    return nullptr;
}

EntityId SystemicWorld::AddBody(const BodyRecord& body) {
    bodies_.push_back(body);
    return body.id;
}

const BodyRecord* SystemicWorld::GetBody(EntityId id) const {
    for (const auto& b : bodies_) {
        if (b.id == id) {
            return &b;
        }
    }
    return nullptr;
}

bool SystemicWorld::HideBody(EntityId body_id, ContainerId container_id) {
    BodyRecord* body = nullptr;
    for (auto& b : bodies_) {
        if (b.id == body_id) {
            body = &b;
            break;
        }
    }
    HideableContainer* container = nullptr;
    for (auto& c : containers_) {
        if (c.id == container_id) {
            container = &c;
            break;
        }
    }
    if (!body || !container) {
        return false;
    }

    // Large-body size check: a cleaning cart / refuse bin / laundry cart can
    // hold a body only when capacity is large enough. This is a kernel check,
    // not a real physics sim.
    constexpr float kBodyVolume = 0.40f;
    if (container->capacity_volume < kBodyVolume) {
        return false;
    }

    body->disposition = BodyDisposition::HiddenInContainer;
    body->container = container_id;
    container->current_occupants.push_back(body_id);
    return true;
}

bool SystemicWorld::CanDirectlyObserveBody(EntityId, EntityId body_id) const {
    const BodyRecord* body = GetBody(body_id);
    if (!body) {
        return false;
    }
    return body->disposition == BodyDisposition::Exposed;
}

bool SystemicWorld::DiscoverBody(NpcId discoverer, ContainerId container_id,
                                 uint64_t frame) {
    HideableContainer* container = nullptr;
    for (auto& c : containers_) {
        if (c.id == container_id) {
            container = &c;
            break;
        }
    }
    if (!container || container->current_occupants.empty()) {
        return false;
    }

    const EntityId body_id = container->current_occupants.front();
    BodyRecord* body = nullptr;
    for (auto& b : bodies_) {
        if (b.id == body_id) {
            body = &b;
            break;
        }
    }
    if (!body) {
        return false;
    }

    // Keep the body in WorldState: it moves from hidden-in-container back to
    // exposed at the container/room, never deleted.
    body->disposition = BodyDisposition::Exposed;
    body->container = ContainerId::Invalid();
    container->current_occupants.erase(
        std::remove(container->current_occupants.begin(),
                    container->current_occupants.end(), body_id),
        container->current_occupants.end());

    // Evidence record.
    EvidenceRecord ev;
    ev.id = EvidenceId::New(evidence_.size() + 1);
    ev.type = EvidenceType::VisibleBody;
    ev.subject = body_id;
    ev.owner = EntityId::New(body->npc.GetValue());
    ev.room = body->room;
    ev.position = body->position;
    ev.visibility = 1.0f;
    ev.persists = true;
    ev.frame = frame;
    ev.discovered_by.push_back(discoverer);
    evidence_.push_back(ev);

    // Memory for the discoverer.
    MemoryRecord mem;
    mem.id = MemoryId::New(memories_.size() + 1);
    mem.npc = EntityId::New(discoverer.GetValue());
    mem.kind = MemoryKind::BodyDiscovery;
    mem.subject = body_id;
    mem.target = EntityId::New(body->npc.GetValue());
    mem.room = body->room;
    mem.frame = frame;
    mem.salience = 0.9f;
    mem.confidence = 1.0f;
    mem.source = KnowledgeSource::DirectWitness;
    mem.text_key = ResourceId::New(0);
    memories_.push_back(mem);

    // Systemic event.
    SystemicEvent evt;
    evt.id = EventId::New(events_.size() + 1);
    evt.type = SystemicEventType::BodyDiscovered;
    evt.actor = EntityId::New(discoverer.GetValue());
    evt.target = EntityId::New(body->npc.GetValue());
    evt.location = body->room;
    evt.frame = frame;
    evt.witnesses.push_back(discoverer);
    evt.severity = 70;
    evt.legality = LegalityClass::Unauthorized;
    evt.owner = EntityId::New(body->npc.GetValue());
    evt.method = "container_open";
    evt.outcome = OutcomeType::Discovered;
    evt.evidence.push_back(ev.id);
    evt.tags.push_back("body_discovery");
    evt.tags.push_back("delayed_discovery");
    events_.push_back(evt);

    // Natural local alert escalation.
    if (alert_.level < FacilityAlertLevel::LocalAlert) {
        SetAlert(FacilityAlertLevel::LocalAlert, {body->room}, frame);
    }

    return true;
}

EvidenceId SystemicWorld::AddEvidence(const EvidenceRecord& evidence) {
    evidence_.push_back(evidence);
    return evidence.id;
}

const EvidenceRecord* SystemicWorld::GetEvidence(EvidenceId id) const {
    for (const auto& e : evidence_) {
        if (e.id == id) {
            return &e;
        }
    }
    return nullptr;
}

MemoryId SystemicWorld::AddMemory(const MemoryRecord& memory) {
    memories_.push_back(memory);
    return memory.id;
}

const MemoryRecord* SystemicWorld::GetMemory(MemoryId id) const {
    for (const auto& m : memories_) {
        if (m.id == id) {
            return &m;
        }
    }
    return nullptr;
}

std::vector<MemoryRecord> SystemicWorld::MemoriesOf(EntityId actor) const {
    std::vector<MemoryRecord> out;
    for (const auto& m : memories_) {
        if (m.npc == actor) {
            out.push_back(m);
        }
    }
    return out;
}

void SystemicWorld::SetRelationship(const RelationshipRecord& rel) {
    for (auto& r : relationships_) {
        const bool same_a = (r.a == rel.a && r.b == rel.b);
        const bool same_b = (r.a == rel.b && r.b == rel.a);
        if (same_a || same_b) {
            r = rel;
            return;
        }
    }
    relationships_.push_back(rel);
}

const RelationshipRecord* SystemicWorld::GetRelationship(EntityId a,
                                                         EntityId b) const {
    for (const auto& r : relationships_) {
        if ((r.a == a && r.b == b) || (r.a == b && r.b == a)) {
            return &r;
        }
    }
    return nullptr;
}

PromiseId SystemicWorld::AddPromise(const PromiseRecord& promise) {
    promises_.push_back(promise);
    return promise.id;
}

const PromiseRecord* SystemicWorld::GetPromise(PromiseId id) const {
    for (const auto& p : promises_) {
        if (p.id == id) {
            return &p;
        }
    }
    return nullptr;
}

bool SystemicWorld::SettlePromise(PromiseId id, PromiseStatus status,
                                  uint64_t frame) {
    PromiseRecord* promise = nullptr;
    for (auto& p : promises_) {
        if (p.id == id) {
            promise = &p;
            break;
        }
    }
    if (!promise) {
        return false;
    }
    if (promise->status != PromiseStatus::Accepted &&
        promise->status != PromiseStatus::Offered) {
        return false;
    }

    promise->status = status;
    promise->storylet_eligible = true;

    if (status == PromiseStatus::Fulfilled) {
        player_state_.reliability = std::min(1.0f, player_state_.reliability + 0.05f);
        RelationshipRecord* rel = nullptr;
        for (auto& r : relationships_) {
            if ((r.a == promise->giver && r.b == promise->receiver) ||
                (r.a == promise->receiver && r.b == promise->giver)) {
                rel = &r;
                break;
            }
        }
        if (rel) {
            rel->trust = std::min(1.0f, rel->trust + 0.10f);
            rel->debt = std::max(0.0f, rel->debt - 0.10f);
        }
    } else if (status == PromiseStatus::Broken || status == PromiseStatus::Expired) {
        player_state_.reliability = std::max(0.0f, player_state_.reliability - 0.08f);
        ++player_state_.promises_broken;
        RelationshipRecord* rel = nullptr;
        for (auto& r : relationships_) {
            if ((r.a == promise->giver && r.b == promise->receiver) ||
                (r.a == promise->receiver && r.b == promise->giver)) {
                rel = &r;
                break;
            }
        }
        if (rel) {
            rel->trust = std::max(0.0f, rel->trust - 0.10f);
            rel->debt = std::min(1.0f, rel->debt + 0.10f);
        }
    }

    MemoryRecord mem;
    mem.id = MemoryId::New(memories_.size() + 1);
    mem.npc = promise->giver;
    mem.kind = MemoryKind::Promise;
    mem.subject = promise->receiver;
    mem.target = promise->giver;
    mem.frame = frame;
    mem.salience = 0.8f;
    mem.confidence = 1.0f;
    mem.source = KnowledgeSource::DirectWitness;
    mem.text_key = ResourceId::New(0);
    mem.tags.push_back(promise->subject);
    memories_.push_back(mem);

    SystemicEvent evt;
    evt.id = EventId::New(events_.size() + 1);
    evt.type = status == PromiseStatus::Fulfilled
                   ? SystemicEventType::PromiseFulfilled
                   : SystemicEventType::PromiseBroken;
    evt.actor = promise->giver;
    evt.target = promise->receiver;
    evt.frame = frame;
    evt.owner = promise->giver;
    evt.method = promise->subject;
    evt.outcome = status == PromiseStatus::Fulfilled
                      ? OutcomeType::Success
                      : OutcomeType::Failure;
    evt.tags.push_back("promise");
    events_.push_back(evt);

    return true;
}

void SystemicWorld::SetAlert(FacilityAlertLevel level,
                             const std::vector<RoomId>& scope,
                             uint64_t frame) {
    alert_.level = level;
    alert_.scope = scope;
    alert_.last_change_frame = frame;
    player_state_.facility_alert = static_cast<float>(level);
}

EventId SystemicWorld::AddSystemicEvent(const SystemicEvent& event) {
    events_.push_back(event);
    return event.id;
}

bool SystemicWorld::ReaderAcceptsItem(ItemId id, uint8_t required_clearance) const {
    const ItemRecord* item = GetItem(id);
    if (!item) {
        return false;
    }
    // A stolen badge is still a functioning credential until it is reported
    // or revoked. Reader checks are physical/credential checks, not social
    // knowledge checks.
    return item->credential_level >= required_clearance;
}

bool SystemicWorld::NpcAcceptsPresentedIdentity(NpcId observer,
                                                EntityId expected_owner,
                                                EntityId presenter) const {
    const ActorRecord* actor = GetActor(observer);
    if (!actor) {
        return true;
    }
    const bool knows_original =
        std::find(actor->known_identities.begin(),
                  actor->known_identities.end(),
                  expected_owner) != actor->known_identities.end();
    if (!knows_original) {
        return true;
    }
    return presenter == expected_owner;
}

IdentityReaction SystemicWorld::ReactionToIdentityMismatch(
    NpcId observer, EntityId original_owner, EntityId presenter) const {
    const ActorRecord* actor = GetActor(observer);
    if (!actor) {
        return IdentityReaction::Accept;
    }
    const bool knows_original =
        std::find(actor->known_identities.begin(),
                  actor->known_identities.end(),
                  original_owner) != actor->known_identities.end();
    if (!knows_original) {
        return IdentityReaction::Accept;
    }
    const EntityId observer_entity = EntityId::New(observer.GetValue());
    const RelationshipRecord* rel_presenter = GetRelationship(observer_entity, presenter);
    const RelationshipRecord* rel_owner = GetRelationship(observer_entity, original_owner);

    const float trust_in_presenter = rel_presenter ? rel_presenter->trust : 0.0f;
    const float fear_of_presenter = rel_presenter ? rel_presenter->fear : 0.0f;
    const float debt_to_presenter = rel_presenter ? rel_presenter->debt : 0.0f;
    const float trust_in_owner = rel_owner ? rel_owner->trust : 0.0f;

    // Reaction is a small policy expression, not a morality system. It is
    // driven by the relationship vector so content can tune it later.
    if (trust_in_presenter > 0.6f && debt_to_presenter > 0.3f &&
        fear_of_presenter < 0.4f) {
        return IdentityReaction::HelpCoverUp;
    }
    if (trust_in_owner > 0.6f && fear_of_presenter > 0.5f) {
        return IdentityReaction::Report;
    }
    if (fear_of_presenter < 0.25f && trust_in_presenter < 0.3f) {
        return IdentityReaction::Confront;
    }
    return IdentityReaction::Report;
}

void SystemicWorld::Save(Serializer& s) const {
    WriteRecordVector(s, actors_, &WriteActor);
    WriteRecordVector(s, items_, &WriteItem);
    WriteRecordVector(s, bodies_, &WriteBody);
    WriteRecordVector(s, containers_, &WriteContainer);
    WriteRecordVector(s, evidence_, &WriteEvidence);
    WriteRecordVector(s, memories_, &WriteMemory);
    WriteRecordVector(s, relationships_, &WriteRelationship);
    WriteRecordVector(s, promises_, &WritePromise);
    WriteRecordVector(s, events_, &WriteEvent);

    s.WriteF32(player_state_.humanity);
    s.WriteF32(player_state_.violence);
    s.WriteF32(player_state_.reliability);
    s.WriteF32(player_state_.coercion);
    s.WriteF32(player_state_.public_trust);
    s.WriteF32(player_state_.security_standing);
    s.WriteF32(player_state_.medical_research_standing);
    s.WriteF32(player_state_.maintenance_standing);
    s.WriteF32(player_state_.narrator_alignment);
    s.WriteF32(player_state_.narrator_dominance);
    s.WriteF32(player_state_.autonomy);
    s.WriteF32(player_state_.truth_exposure);
    s.WriteF32(player_state_.self_knowledge);
    s.WriteF32(player_state_.facility_alert);
    s.WriteF32(player_state_.infrastructure_integrity);
    s.WriteF32(player_state_.timeline_instability);
    s.WriteF32(player_state_.residual_memory_pressure);
    s.WriteU32(player_state_.civilian_casualties);
    s.WriteU32(player_state_.security_casualties);
    s.WriteU32(player_state_.promises_broken);
    s.WriteU32(player_state_.evidence_count);

    s.WriteU8(narrator_.authority_stage);
    s.WriteF32(narrator_.intervention_cost);
    s.WriteU64(narrator_.intervention_cooldown_frame);
    s.WriteU8(narrator_.observes_cameras ? 1 : 0);
    s.WriteU8(narrator_.observes_microphones ? 1 : 0);
    s.WriteU8(narrator_.observes_access_readers ? 1 : 0);
    s.WriteU8(narrator_.observes_terminals ? 1 : 0);
    s.WriteU8(narrator_.observes_radio ? 1 : 0);
    s.WriteU8(narrator_.observes_npc_reports ? 1 : 0);
    s.WriteU8(narrator_.observes_timeline_anomalies ? 1 : 0);
    s.WriteU8(narrator_.meta_observability ? 1 : 0);

    s.WriteU8(static_cast<uint8_t>(alert_.level));
    s.WriteU64(alert_.last_change_frame);
    WriteIdVector(s, alert_.scope);
}

void SystemicWorld::Load(Deserializer& d) {
    actors_ = ReadRecordVector<ActorRecord>(d, &ReadActor);
    items_ = ReadRecordVector<ItemRecord>(d, &ReadItem);
    bodies_ = ReadRecordVector<BodyRecord>(d, &ReadBody);
    containers_ = ReadRecordVector<HideableContainer>(d, &ReadContainer);
    evidence_ = ReadRecordVector<EvidenceRecord>(d, &ReadEvidence);
    memories_ = ReadRecordVector<MemoryRecord>(d, &ReadMemory);
    relationships_ = ReadRecordVector<RelationshipRecord>(d, &ReadRelationship);
    promises_ = ReadRecordVector<PromiseRecord>(d, &ReadPromise);
    events_ = ReadRecordVector<SystemicEvent>(d, &ReadEvent);

    player_state_.humanity = d.ReadF32();
    player_state_.violence = d.ReadF32();
    player_state_.reliability = d.ReadF32();
    player_state_.coercion = d.ReadF32();
    player_state_.public_trust = d.ReadF32();
    player_state_.security_standing = d.ReadF32();
    player_state_.medical_research_standing = d.ReadF32();
    player_state_.maintenance_standing = d.ReadF32();
    player_state_.narrator_alignment = d.ReadF32();
    player_state_.narrator_dominance = d.ReadF32();
    player_state_.autonomy = d.ReadF32();
    player_state_.truth_exposure = d.ReadF32();
    player_state_.self_knowledge = d.ReadF32();
    player_state_.facility_alert = d.ReadF32();
    player_state_.infrastructure_integrity = d.ReadF32();
    player_state_.timeline_instability = d.ReadF32();
    player_state_.residual_memory_pressure = d.ReadF32();
    player_state_.civilian_casualties = d.ReadU32();
    player_state_.security_casualties = d.ReadU32();
    player_state_.promises_broken = d.ReadU32();
    player_state_.evidence_count = d.ReadU32();

    narrator_.authority_stage = d.ReadU8();
    narrator_.intervention_cost = d.ReadF32();
    narrator_.intervention_cooldown_frame = d.ReadU64();
    narrator_.observes_cameras = d.ReadU8() != 0;
    narrator_.observes_microphones = d.ReadU8() != 0;
    narrator_.observes_access_readers = d.ReadU8() != 0;
    narrator_.observes_terminals = d.ReadU8() != 0;
    narrator_.observes_radio = d.ReadU8() != 0;
    narrator_.observes_npc_reports = d.ReadU8() != 0;
    narrator_.observes_timeline_anomalies = d.ReadU8() != 0;
    narrator_.meta_observability = d.ReadU8() != 0;

    alert_.level = static_cast<FacilityAlertLevel>(d.ReadU8());
    alert_.last_change_frame = d.ReadU64();
    alert_.scope = ReadIdVector<RoomId>(d);
}

std::vector<uint8_t> SystemicWorld::Serialize() const {
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    Save(s);
    return bytes;
}

SystemicWorld SystemicWorld::Deserialize(const uint8_t* data, size_t size) {
    SystemicWorld world;
    Deserializer d(data, size);
    world.Load(d);
    return world;
}

} // namespace writeover
