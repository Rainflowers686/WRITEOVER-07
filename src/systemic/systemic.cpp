#include "writeover/systemic/systemic.h"
#include "writeover/common/io.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <string>
#include <utility>

namespace writeover {

namespace {

constexpr uint32_t kMaxActors = 256;
constexpr uint32_t kMaxItems = 4096;
constexpr uint32_t kMaxBodies = 1024;
constexpr uint32_t kMaxContainers = 1024;
constexpr uint32_t kMaxEvidence = 8192;
constexpr uint32_t kMaxMemories = 16384;
constexpr uint32_t kMaxRelationships = 8192;
constexpr uint32_t kMaxPromises = 4096;
constexpr uint32_t kMaxQuests = 1024;
constexpr uint32_t kMaxKnowledge = 4096;
constexpr uint32_t kMaxSearches = 4096;
constexpr uint32_t kMaxExchanges = 4096;
constexpr uint32_t kMaxTerminals = 1024;
constexpr uint32_t kMaxSessions = 4096;
constexpr uint32_t kMaxAudits = 8192;
constexpr uint32_t kMaxSources = 1024;
constexpr uint32_t kMaxEvents = 16384;
constexpr uint32_t kMaxDrags = 1024;
constexpr uint32_t kMaxVector = 4096;
constexpr uint32_t kMaxString = 4096;

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

bool ReadCount(Deserializer& d, uint32_t& out, uint32_t max) {
    if (d.Remaining() < 4) {
        d.MarkError();
        return false;
    }
    out = d.ReadU32();
    if (out > max) {
        d.MarkError();
        return false;
    }
    return !d.HasError();
}

bool WriteStringsBounded(Serializer& s, const std::vector<std::string>& values) {
    if (values.size() > kMaxVector) {
        return false;
    }
    s.WriteU32(static_cast<uint32_t>(values.size()));
    for (const auto& v : values) {
        if (v.size() > kMaxString) {
            return false;
        }
        s.WriteString(v);
    }
    return true;
}

bool ReadBoundedString(Deserializer& d, std::string& out) {
    if (d.Remaining() < 4) {
        d.MarkError();
        return false;
    }
    const uint32_t len = d.ReadU32();
    if (len > kMaxString || len > d.Remaining()) {
        d.MarkError();
        return false;
    }
    out.resize(len);
    if (len > 0) {
        d.ReadBytes(out.data(), len);
    }
    return !d.HasError();
}

bool ReadStringsBounded(Deserializer& d, std::vector<std::string>& out) {
    uint32_t n = 0;
    if (!ReadCount(d, n, kMaxVector)) {
        return false;
    }
    out.clear();
    out.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        std::string value;
        if (!ReadBoundedString(d, value)) {
            return false;
        }
        out.push_back(std::move(value));
    }
    return true;
}

template <typename Id>
bool WriteIdVector(Serializer& s, const std::vector<Id>& values) {
    if (values.size() > kMaxVector) {
        return false;
    }
    s.WriteU32(static_cast<uint32_t>(values.size()));
    for (const auto& v : values) {
        WriteId(s, v);
    }
    return true;
}

template <typename Id>
bool ReadIdVectorBounded(Deserializer& d, std::vector<Id>& out) {
    uint32_t n = 0;
    if (!ReadCount(d, n, kMaxVector)) {
        return false;
    }
    out.clear();
    out.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        out.push_back(ReadId<Id>(d));
        if (d.HasError()) {
            return false;
        }
    }
    return true;
}

bool IsFinite(float v) {
    return std::isfinite(v);
}

bool IsUnit(float v) {
    return IsFinite(v) && v >= 0.0f && v <= 1.0f;
}

bool IsAlertLevel(uint8_t v) { return v <= static_cast<uint8_t>(FacilityAlertLevel::Critical); }
bool IsBodyStatus(uint8_t v) { return v <= static_cast<uint8_t>(BodyStatus::Dead); }
bool IsBodyDisposition(uint8_t v) { return v <= static_cast<uint8_t>(BodyDisposition::HiddenInContainer); }
bool IsBodyDragStatus(uint8_t v) { return v <= static_cast<uint8_t>(BodyDragStatus::Dragging); }
bool IsBodyDiscoveryResponse(uint8_t v) { return v <= static_cast<uint8_t>(BodyDiscoveryResponse::Flee); }
bool IsContainerKind(uint8_t v) { return v <= static_cast<uint8_t>(ContainerKind::SealedRoom); }
bool IsRoutineTag(uint8_t v) { return v <= static_cast<uint8_t>(RoutineTag::RoutineUser); }
bool IsEvidenceType(uint8_t v) { return v <= static_cast<uint8_t>(EvidenceType::CctvRecord); }
bool IsFaction(uint8_t v) { return v <= static_cast<uint8_t>(Faction::Civilian); }
bool IsCognition(uint8_t v) { return v <= static_cast<uint8_t>(CognitionTier::SemiHuman); }
bool IsRole(uint8_t v) { return v <= static_cast<uint8_t>(Role::Other); }
bool IsItemType(uint8_t v) { return v <= static_cast<uint8_t>(ItemType::Other); }
bool IsItemLocation(uint8_t v) { return v <= static_cast<uint8_t>(ItemLocationKind::Container); }
bool IsMemoryKind(uint8_t v) { return v <= static_cast<uint8_t>(MemoryKind::Report); }
bool IsKnowledgeSource(uint8_t v) { return v <= static_cast<uint8_t>(KnowledgeSource::ResidualMemory); }
bool IsPromiseStatus(uint8_t v) { return v <= static_cast<uint8_t>(PromiseStatus::Cancelled); }
bool IsQuestStatus(uint8_t v) { return v <= static_cast<uint8_t>(QuestStatus::Betrayed); }
bool IsSearchTarget(uint8_t v) { return v <= static_cast<uint8_t>(SearchTargetType::Container); }
bool IsSocialExchangeType(uint8_t v) { return v <= static_cast<uint8_t>(SocialExchangeType::AskFavor); }
bool IsSocialExchangeOutcome(uint8_t v) { return v <= static_cast<uint8_t>(SocialExchangeOutcome::AcceptedThenMayReport); }
bool IsKnowledgeAssetType(uint8_t v) { return v <= static_cast<uint8_t>(KnowledgeAssetType::Other); }
bool IsTerminalAccessMethod(uint8_t v) { return v <= static_cast<uint8_t>(TerminalAccessMethod::PhysicalServicePort); }
bool IsObservationSourceType(uint8_t v) { return v <= static_cast<uint8_t>(ObservationSourceType::Other); }
bool IsSystemicEventType(uint8_t v) { return v <= static_cast<uint8_t>(SystemicEventType::SocialExchange); }
bool IsLegality(uint8_t v) { return v <= static_cast<uint8_t>(LegalityClass::JustifiedOrUnclear); }
bool IsOutcome(uint8_t v) { return v <= static_cast<uint8_t>(OutcomeType::Undetected); }
bool IsIdentityReaction(uint8_t v) { return v <= static_cast<uint8_t>(IdentityReaction::HelpCoverUp); }

bool WriteActor(Serializer& s, const ActorRecord& a) {
    WriteId(s, a.id);
    WriteId(s, a.data_key);
    s.WriteU8(static_cast<uint8_t>(a.faction));
    s.WriteU8(static_cast<uint8_t>(a.cognition));
    s.WriteU8(static_cast<uint8_t>(a.role));
    WriteId(s, a.occupation);
    s.WriteU8(a.full_human_illusion ? 1 : 0);
    return WriteIdVector(s, a.known_identities) && WriteStringsBounded(s, a.personality_tags);
}

bool ReadActor(Deserializer& d, ActorRecord& a) {
    a.id = ReadId<NpcId>(d);
    a.data_key = ReadId<ResourceId>(d);
    const uint8_t faction = d.ReadU8();
    const uint8_t cog = d.ReadU8();
    const uint8_t role = d.ReadU8();
    a.occupation = ReadId<StringId>(d);
    a.full_human_illusion = d.ReadU8() != 0;
    if (d.HasError() || !IsFaction(faction) || !IsCognition(cog) || !IsRole(role)) {
        d.MarkError();
        return false;
    }
    a.faction = static_cast<Faction>(faction);
    a.cognition = static_cast<CognitionTier>(cog);
    a.role = static_cast<Role>(role);
    return ReadIdVectorBounded(d, a.known_identities) && ReadStringsBounded(d, a.personality_tags);
}

bool WriteItem(Serializer& s, const ItemRecord& i) {
    WriteId(s, i.id);
    s.WriteU8(static_cast<uint8_t>(i.type));
    WriteId(s, i.owner);
    WriteId(s, i.issuer);
    WriteId(s, i.legal_holder);
    WriteId(s, i.current_holder);
    s.WriteU8(static_cast<uint8_t>(i.location));
    WriteVec3(s, i.ground_position);
    WriteId(s, i.room);
    WriteId(s, i.container);
    s.WriteU8(i.reported_stolen ? 1 : 0);
    s.WriteU8(i.revoked ? 1 : 0);
    s.WriteU8(i.credential_level);
    return WriteStringsBounded(s, i.provenance_tags);
}

bool ReadItem(Deserializer& d, ItemRecord& i) {
    i.id = ReadId<ItemId>(d);
    const uint8_t type = d.ReadU8();
    i.owner = ReadId<EntityId>(d);
    i.issuer = ReadId<EntityId>(d);
    i.legal_holder = ReadId<EntityId>(d);
    i.current_holder = ReadId<EntityId>(d);
    const uint8_t loc = d.ReadU8();
    i.ground_position = ReadVec3(d);
    i.room = ReadId<RoomId>(d);
    i.container = ReadId<ContainerId>(d);
    i.reported_stolen = d.ReadU8() != 0;
    i.revoked = d.ReadU8() != 0;
    i.credential_level = d.ReadU8();
    if (d.HasError() || !IsItemType(type) || !IsItemLocation(loc) || i.credential_level > 3) {
        d.MarkError();
        return false;
    }
    i.type = static_cast<ItemType>(type);
    i.location = static_cast<ItemLocationKind>(loc);
    return ReadStringsBounded(d, i.provenance_tags);
}

bool WriteBody(Serializer& s, const BodyRecord& b) {
    WriteId(s, b.id);
    WriteId(s, b.npc);
    s.WriteU8(static_cast<uint8_t>(b.status));
    s.WriteU8(static_cast<uint8_t>(b.disposition));
    s.WriteU8(static_cast<uint8_t>(b.drag_status));
    WriteVec3(s, b.position);
    WriteId(s, b.room);
    WriteId(s, b.container);
    s.WriteU8(b.searched ? 1 : 0);
    s.WriteU8(b.has_weapon ? 1 : 0);
    return true;
}

bool ReadBody(Deserializer& d, BodyRecord& b) {
    b.id = ReadId<EntityId>(d);
    b.npc = ReadId<NpcId>(d);
    const uint8_t st = d.ReadU8();
    const uint8_t disp = d.ReadU8();
    const uint8_t drag = d.ReadU8();
    b.position = ReadVec3(d);
    b.room = ReadId<RoomId>(d);
    b.container = ReadId<ContainerId>(d);
    b.searched = d.ReadU8() != 0;
    b.has_weapon = d.ReadU8() != 0;
    if (d.HasError() || !IsBodyStatus(st) || !IsBodyDisposition(disp) || !IsBodyDragStatus(drag)) {
        d.MarkError();
        return false;
    }
    b.status = static_cast<BodyStatus>(st);
    b.disposition = static_cast<BodyDisposition>(disp);
    b.drag_status = static_cast<BodyDragStatus>(drag);
    return true;
}

bool WriteContainer(Serializer& s, const HideableContainer& c) {
    WriteId(s, c.id);
    s.WriteU8(static_cast<uint8_t>(c.kind));
    WriteVec3(s, c.position);
    WriteId(s, c.room);
    s.WriteF32(c.capacity_volume);
    s.WriteU8(c.concealment);
    s.WriteU8(c.accessibility);
    if (c.routine_tags.size() > kMaxVector) return false;
    s.WriteU32(static_cast<uint32_t>(c.routine_tags.size()));
    for (const auto tag : c.routine_tags) {
        s.WriteU8(static_cast<uint8_t>(tag));
    }
    return WriteIdVector(s, c.current_occupants);
}

bool ReadContainer(Deserializer& d, HideableContainer& c) {
    c.id = ReadId<ContainerId>(d);
    const uint8_t kind = d.ReadU8();
    c.position = ReadVec3(d);
    c.room = ReadId<RoomId>(d);
    c.capacity_volume = d.ReadF32();
    c.concealment = d.ReadU8();
    c.accessibility = d.ReadU8();
    uint32_t n = 0;
    if (!ReadCount(d, n, kMaxVector)) return false;
    c.routine_tags.clear();
    for (uint32_t i = 0; i < n; ++i) {
        const uint8_t tag = d.ReadU8();
        if (!IsRoutineTag(tag)) {
            d.MarkError();
            return false;
        }
        c.routine_tags.push_back(static_cast<RoutineTag>(tag));
    }
    if (d.HasError() || !IsContainerKind(kind) || !IsFinite(c.capacity_volume) ||
        c.capacity_volume <= 0.0f || c.concealment > 100 || c.accessibility > 100) {
        d.MarkError();
        return false;
    }
    c.kind = static_cast<ContainerKind>(kind);
    return ReadIdVectorBounded(d, c.current_occupants);
}

bool WriteDrag(Serializer& s, const BodyDragRecord& r) {
    WriteId(s, r.actor);
    WriteId(s, r.body);
    WriteId(s, r.room);
    WriteVec3(s, r.position);
    s.WriteU64(r.started_frame);
    s.WriteU64(r.last_update_frame);
    s.WriteU8(r.sprint_forbidden ? 1 : 0);
    s.WriteU8(r.weapon_restricted ? 1 : 0);
    s.WriteF32(r.movement_modifier);
    s.WriteString(r.noise_profile);
    return true;
}

bool ReadDrag(Deserializer& d, BodyDragRecord& r) {
    r.actor = ReadId<EntityId>(d);
    r.body = ReadId<EntityId>(d);
    r.room = ReadId<RoomId>(d);
    r.position = ReadVec3(d);
    r.started_frame = d.ReadU64();
    r.last_update_frame = d.ReadU64();
    r.sprint_forbidden = d.ReadU8() != 0;
    r.weapon_restricted = d.ReadU8() != 0;
    r.movement_modifier = d.ReadF32();
    if (!ReadBoundedString(d, r.noise_profile)) return false;
    return !d.HasError() && IsFinite(r.movement_modifier) && r.movement_modifier >= 0.0f &&
           r.movement_modifier <= 1.0f;
}

bool WriteEvidence(Serializer& s, const EvidenceRecord& e) {
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
    return WriteIdVector(s, e.discovered_by);
}

bool ReadEvidence(Deserializer& d, EvidenceRecord& e) {
    e.id = ReadId<EvidenceId>(d);
    const uint8_t type = d.ReadU8();
    e.source_event = ReadId<EventId>(d);
    e.subject = ReadId<EntityId>(d);
    e.owner = ReadId<EntityId>(d);
    e.room = ReadId<RoomId>(d);
    e.position = ReadVec3(d);
    e.visibility = d.ReadF32();
    e.persists = d.ReadU8() != 0;
    e.frame = d.ReadU64();
    if (d.HasError() || !IsEvidenceType(type) || !IsUnit(e.visibility)) {
        d.MarkError();
        return false;
    }
    e.type = static_cast<EvidenceType>(type);
    return ReadIdVectorBounded(d, e.discovered_by);
}

bool WriteMemory(Serializer& s, const MemoryRecord& m) {
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
    return WriteStringsBounded(s, m.tags);
}

bool ReadMemory(Deserializer& d, MemoryRecord& m) {
    m.id = ReadId<MemoryId>(d);
    m.npc = ReadId<EntityId>(d);
    const uint8_t kind = d.ReadU8();
    m.subject = ReadId<EntityId>(d);
    m.target = ReadId<EntityId>(d);
    m.room = ReadId<RoomId>(d);
    m.frame = d.ReadU64();
    m.salience = d.ReadF32();
    m.confidence = d.ReadF32();
    const uint8_t src = d.ReadU8();
    m.text_key = ReadId<ResourceId>(d);
    if (d.HasError() || !IsMemoryKind(kind) || !IsUnit(m.salience) || !IsUnit(m.confidence) ||
        !IsKnowledgeSource(src)) {
        d.MarkError();
        return false;
    }
    m.kind = static_cast<MemoryKind>(kind);
    m.source = static_cast<KnowledgeSource>(src);
    return ReadStringsBounded(d, m.tags);
}

bool WriteRelationship(Serializer& s, const RelationshipRecord& r) {
    WriteId(s, r.a);
    WriteId(s, r.b);
    s.WriteF32(r.trust);
    s.WriteF32(r.fear);
    s.WriteF32(r.respect);
    s.WriteF32(r.suspicion);
    s.WriteF32(r.debt);
    s.WriteF32(r.attachment);
    s.WriteF32(r.ideological_alignment);
    return true;
}

bool ReadRelationship(Deserializer& d, RelationshipRecord& r) {
    r.a = ReadId<EntityId>(d);
    r.b = ReadId<EntityId>(d);
    r.trust = d.ReadF32();
    r.fear = d.ReadF32();
    r.respect = d.ReadF32();
    r.suspicion = d.ReadF32();
    r.debt = d.ReadF32();
    r.attachment = d.ReadF32();
    r.ideological_alignment = d.ReadF32();
    if (d.HasError() || !IsUnit(r.trust) || !IsUnit(r.fear) || !IsUnit(r.respect) ||
        !IsUnit(r.suspicion) || !IsUnit(r.debt) || !IsUnit(r.attachment) ||
        !IsFinite(r.ideological_alignment) || r.ideological_alignment < -1.0f ||
        r.ideological_alignment > 1.0f) {
        d.MarkError();
        return false;
    }
    return true;
}

bool WritePromise(Serializer& s, const PromiseRecord& p) {
    WriteId(s, p.id);
    WriteId(s, p.giver);
    WriteId(s, p.receiver);
    s.WriteString(p.subject);
    s.WriteU64(p.accepted_frame);
    s.WriteU64(p.deadline_frame);
    s.WriteU64(p.transition_frame);
    s.WriteString(p.reason);
    s.WriteU8(static_cast<uint8_t>(p.status));
    s.WriteU8(p.storylet_eligible ? 1 : 0);
    return true;
}

bool ReadPromise(Deserializer& d, PromiseRecord& p) {
    p.id = ReadId<PromiseId>(d);
    p.giver = ReadId<EntityId>(d);
    p.receiver = ReadId<EntityId>(d);
    if (!ReadBoundedString(d, p.subject)) return false;
    p.accepted_frame = d.ReadU64();
    p.deadline_frame = d.ReadU64();
    p.transition_frame = d.ReadU64();
    if (!ReadBoundedString(d, p.reason)) return false;
    const uint8_t st = d.ReadU8();
    p.storylet_eligible = d.ReadU8() != 0;
    if (d.HasError() || !IsPromiseStatus(st) || p.subject.size() > kMaxString ||
        p.reason.size() > kMaxString) {
        d.MarkError();
        return false;
    }
    p.status = static_cast<PromiseStatus>(st);
    return true;
}

bool WriteQuest(Serializer& s, const QuestRecord& q) {
    WriteId(s, q.id);
    s.WriteString(q.title);
    s.WriteString(q.presentation_objective);
    s.WriteU8(static_cast<uint8_t>(q.status));
    s.WriteU64(q.transition_frame);
    s.WriteString(q.reason);
    return true;
}

bool ReadQuest(Deserializer& d, QuestRecord& q) {
    q.id = ReadId<QuestId>(d);
    if (!ReadBoundedString(d, q.title)) return false;
    if (!ReadBoundedString(d, q.presentation_objective)) return false;
    const uint8_t st = d.ReadU8();
    q.transition_frame = d.ReadU64();
    if (!ReadBoundedString(d, q.reason)) return false;
    if (d.HasError() || !IsQuestStatus(st) || q.title.size() > kMaxString ||
        q.presentation_objective.size() > kMaxString || q.reason.size() > kMaxString) {
        d.MarkError();
        return false;
    }
    q.status = static_cast<QuestStatus>(st);
    return true;
}

bool WriteKnowledge(Serializer& s, const KnowledgeAssetRecord& k) {
    WriteId(s, k.id);
    s.WriteU8(static_cast<uint8_t>(k.type));
    WriteId(s, k.source);
    s.WriteF32(k.confidence);
    return WriteIdVector(s, k.known_by);
}

bool ReadKnowledge(Deserializer& d, KnowledgeAssetRecord& k) {
    k.id = ReadId<KnowledgeAssetId>(d);
    const uint8_t type = d.ReadU8();
    k.source = ReadId<ResourceId>(d);
    k.confidence = d.ReadF32();
    if (d.HasError() || !IsKnowledgeAssetType(type) || !IsUnit(k.confidence)) {
        d.MarkError();
        return false;
    }
    k.type = static_cast<KnowledgeAssetType>(type);
    return ReadIdVectorBounded(d, k.known_by);
}

bool WriteSearch(Serializer& s, const SearchAction& a) {
    WriteId(s, a.actor);
    WriteId(s, a.target);
    s.WriteU8(static_cast<uint8_t>(a.target_type));
    s.WriteU8(a.consent ? 1 : 0);
    WriteId(s, a.room);
    s.WriteU64(a.frame);
    return true;
}

bool ReadSearch(Deserializer& d, SearchAction& a) {
    a.actor = ReadId<EntityId>(d);
    a.target = ReadId<EntityId>(d);
    const uint8_t tt = d.ReadU8();
    a.consent = d.ReadU8() != 0;
    a.room = ReadId<RoomId>(d);
    a.frame = d.ReadU64();
    if (d.HasError() || !IsSearchTarget(tt)) {
        d.MarkError();
        return false;
    }
    a.target_type = static_cast<SearchTargetType>(tt);
    return true;
}

bool WriteExchange(Serializer& s, const SocialExchangeRecord& e) {
    WriteId(s, e.id);
    s.WriteU8(static_cast<uint8_t>(e.type));
    WriteId(s, e.actor);
    WriteId(s, e.target);
    if (!WriteIdVector(s, e.items) || !WriteIdVector(s, e.information)) return false;
    s.WriteU32(e.cash);
    if (!WriteIdVector(s, e.witnesses)) return false;
    s.WriteString(e.risk_context);
    s.WriteU8(static_cast<uint8_t>(e.outcome));
    s.WriteU64(e.frame);
    return true;
}

bool ReadExchange(Deserializer& d, SocialExchangeRecord& e) {
    e.id = ReadId<SocialExchangeId>(d);
    const uint8_t type = d.ReadU8();
    e.actor = ReadId<EntityId>(d);
    e.target = ReadId<EntityId>(d);
    if (!ReadIdVectorBounded(d, e.items) || !ReadIdVectorBounded(d, e.information)) return false;
    e.cash = d.ReadU32();
    if (!ReadIdVectorBounded(d, e.witnesses)) return false;
    if (!ReadBoundedString(d, e.risk_context)) return false;
    const uint8_t out = d.ReadU8();
    e.frame = d.ReadU64();
    if (d.HasError() || !IsSocialExchangeType(type) || !IsSocialExchangeOutcome(out) ||
        e.risk_context.size() > kMaxString) {
        d.MarkError();
        return false;
    }
    e.type = static_cast<SocialExchangeType>(type);
    e.outcome = static_cast<SocialExchangeOutcome>(out);
    return true;
}

bool WriteTerminal(Serializer& s, const TerminalRecord& t) {
    WriteId(s, t.id);
    WriteId(s, t.room);
    s.WriteU8(t.powered ? 1 : 0);
    s.WriteU8(t.credential_requirement);
    return WriteStringsBounded(s, t.access_scope);
}

bool ReadTerminal(Deserializer& d, TerminalRecord& t) {
    t.id = ReadId<TerminalId>(d);
    t.room = ReadId<RoomId>(d);
    t.powered = d.ReadU8() != 0;
    t.credential_requirement = d.ReadU8();
    if (d.HasError() || t.credential_requirement > 3) {
        d.MarkError();
        return false;
    }
    return ReadStringsBounded(d, t.access_scope);
}

bool WriteSession(Serializer& s, const TerminalSession& t) {
    WriteId(s, t.terminal);
    WriteId(s, t.user);
    s.WriteU8(static_cast<uint8_t>(t.method));
    s.WriteU64(t.started_frame);
    s.WriteU8(t.active ? 1 : 0);
    return true;
}

bool ReadSession(Deserializer& d, TerminalSession& t) {
    t.terminal = ReadId<TerminalId>(d);
    t.user = ReadId<EntityId>(d);
    const uint8_t m = d.ReadU8();
    t.started_frame = d.ReadU64();
    t.active = d.ReadU8() != 0;
    if (d.HasError() || !IsTerminalAccessMethod(m)) {
        d.MarkError();
        return false;
    }
    t.method = static_cast<TerminalAccessMethod>(m);
    return true;
}

bool WriteAudit(Serializer& s, const TerminalAuditLog& a) {
    WriteId(s, a.terminal);
    WriteId(s, a.user);
    s.WriteU8(static_cast<uint8_t>(a.method));
    s.WriteU64(a.frame);
    s.WriteString(a.action);
    s.WriteU8(a.unauthorized ? 1 : 0);
    return true;
}

bool ReadAudit(Deserializer& d, TerminalAuditLog& a) {
    a.terminal = ReadId<TerminalId>(d);
    a.user = ReadId<EntityId>(d);
    const uint8_t m = d.ReadU8();
    a.frame = d.ReadU64();
    if (!ReadBoundedString(d, a.action)) return false;
    a.unauthorized = d.ReadU8() != 0;
    if (d.HasError() || !IsTerminalAccessMethod(m) || a.action.size() > kMaxString) {
        d.MarkError();
        return false;
    }
    a.method = static_cast<TerminalAccessMethod>(m);
    return true;
}

bool WriteSource(Serializer& s, const ObservationSource& o) {
    WriteId(s, o.id);
    s.WriteU8(static_cast<uint8_t>(o.type));
    WriteId(s, o.room);
    s.WriteU8(o.online ? 1 : 0);
    s.WriteString(o.network_segment);
    s.WriteString(o.provenance);
    return true;
}

bool ReadSource(Deserializer& d, ObservationSource& o) {
    o.id = ReadId<ObservationSourceId>(d);
    const uint8_t type = d.ReadU8();
    o.room = ReadId<RoomId>(d);
    o.online = d.ReadU8() != 0;
    if (!ReadBoundedString(d, o.network_segment)) return false;
    if (!ReadBoundedString(d, o.provenance)) return false;
    if (d.HasError() || !IsObservationSourceType(type) ||
        o.network_segment.size() > kMaxString || o.provenance.size() > kMaxString) {
        d.MarkError();
        return false;
    }
    o.type = static_cast<ObservationSourceType>(type);
    return true;
}

bool WriteSystemicEvent(Serializer& s, const SystemicEvent& e) {
    WriteId(s, e.id);
    WriteId(s, e.source_world_event_id);
    s.WriteU8(static_cast<uint8_t>(e.type));
    WriteId(s, e.actor);
    WriteId(s, e.target);
    WriteId(s, e.location);
    s.WriteU64(e.frame);
    if (!WriteIdVector(s, e.witnesses)) return false;
    s.WriteU8(e.severity);
    s.WriteU8(static_cast<uint8_t>(e.legality));
    WriteId(s, e.owner);
    s.WriteString(e.method);
    s.WriteU8(static_cast<uint8_t>(e.outcome));
    if (!WriteIdVector(s, e.evidence) || !WriteStringsBounded(s, e.tags)) return false;
    return true;
}

bool ReadSystemicEvent(Deserializer& d, SystemicEvent& e) {
    e.id = ReadId<EventId>(d);
    e.source_world_event_id = ReadId<EventId>(d);
    const uint8_t type = d.ReadU8();
    e.actor = ReadId<EntityId>(d);
    e.target = ReadId<EntityId>(d);
    e.location = ReadId<RoomId>(d);
    e.frame = d.ReadU64();
    if (!ReadIdVectorBounded(d, e.witnesses)) return false;
    e.severity = d.ReadU8();
    const uint8_t leg = d.ReadU8();
    e.owner = ReadId<EntityId>(d);
    if (!ReadBoundedString(d, e.method)) return false;
    const uint8_t out = d.ReadU8();
    if (!ReadIdVectorBounded(d, e.evidence) || !ReadStringsBounded(d, e.tags)) return false;
    if (d.HasError() || !IsSystemicEventType(type) || !IsLegality(leg) || !IsOutcome(out) ||
        e.severity > 100 || e.method.size() > kMaxString) {
        d.MarkError();
        return false;
    }
    e.type = static_cast<SystemicEventType>(type);
    e.legality = static_cast<LegalityClass>(leg);
    e.outcome = static_cast<OutcomeType>(out);
    return true;
}

bool HasDuplicateIds(const std::vector<NpcId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<ItemId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<EntityId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<ContainerId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<EvidenceId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<MemoryId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<PromiseId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<QuestId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<KnowledgeAssetId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<SocialExchangeId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<TerminalId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<ObservationSourceId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}
bool HasDuplicateIds(const std::vector<EventId>& ids) {
    std::set<uint64_t> seen;
    for (const auto& id : ids) {
        if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
    }
    return false;
}

template <typename T>
bool WriteRecordVector(Serializer& s, const std::vector<T>& values,
                       bool (*write_one)(Serializer&, const T&)) {
    if (values.size() > kMaxVector) return false;
    s.WriteU32(static_cast<uint32_t>(values.size()));
    for (const auto& v : values) {
        if (!write_one(s, v)) return false;
    }
    return true;
}

template <typename T>
bool ReadRecordVector(Deserializer& d, std::vector<T>& out, uint32_t max,
                      bool (*read_one)(Deserializer&, T&)) {
    uint32_t n = 0;
    if (!ReadCount(d, n, max)) return false;
    out.clear();
    out.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        T value{};
        if (!read_one(d, value)) return false;
        out.push_back(std::move(value));
    }
    return !d.HasError();
}

bool ValidateWorld(const SystemicWorld& w, std::string& error) {
    auto has_dup = [](const auto& values, const auto& idFn) {
        std::set<uint64_t> seen;
        for (const auto& v : values) {
            const auto id = idFn(v);
            if (id.GetValue() == 0 || !seen.insert(id.GetValue()).second) return true;
        }
        return false;
    };
    if (has_dup(w.Actors(), [](const ActorRecord& a){ return a.id; })) { error = "duplicate actor id"; return false; }
    if (has_dup(w.Items(), [](const ItemRecord& i){ return i.id; })) { error = "duplicate item id"; return false; }
    if (has_dup(w.Bodies(), [](const BodyRecord& b){ return b.id; })) { error = "duplicate body id"; return false; }
    if (has_dup(w.Containers(), [](const HideableContainer& c){ return c.id; })) { error = "duplicate container id"; return false; }
    if (has_dup(w.Evidence(), [](const EvidenceRecord& e){ return e.id; })) { error = "duplicate evidence id"; return false; }
    if (has_dup(w.Memories(), [](const MemoryRecord& m){ return m.id; })) { error = "duplicate memory id"; return false; }
    if (has_dup(w.Promises(), [](const PromiseRecord& p){ return p.id; })) { error = "duplicate promise id"; return false; }
    if (has_dup(w.Quests(), [](const QuestRecord& q){ return q.id; })) { error = "duplicate quest id"; return false; }
    if (has_dup(w.Knowledge(), [](const KnowledgeAssetRecord& k){ return k.id; })) { error = "duplicate knowledge id"; return false; }
    if (has_dup(w.Terminals(), [](const TerminalRecord& t){ return t.id; })) { error = "duplicate terminal id"; return false; }
    if (has_dup(w.SystemEvents(), [](const SystemicEvent& e){ return e.id; })) { error = "duplicate systemic event id"; return false; }
    return true;
}

} // namespace

SystemicEventBridge::SystemicEventBridge(SystemicWorld* world) : world_(world) {}

void SystemicEventBridge::Register(EventBus& bus) {
    consumer_id_ = bus.Register([this](const WorldEvent& event) { OnWorldEvent(event); });
}

void SystemicEventBridge::OnWorldEvent(const WorldEvent& event) {
    if (!world_) return;
    if (world_->BridgeWorldEventOnce(event)) {
        ++bridged_count_;
    }
}


// ---------------------------------------------------------------------------
// SystemicWorld implementation
// ---------------------------------------------------------------------------

bool SystemicWorld::AddActor(const ActorRecord& actor) {
    if (!actor.id.IsValid()) return false;
    if (GetActor(actor.id) != nullptr) return false;
    if (actor.role == Role::Other && actor.occupation.GetValue() == 0) return false;
    actors_.push_back(actor);
    return true;
}

const ActorRecord* SystemicWorld::GetActor(NpcId id) const {
    for (const auto& a : actors_) if (a.id == id) return &a;
    return nullptr;
}

bool SystemicWorld::AddItem(const ItemRecord& item) {
    if (!item.id.IsValid()) return false;
    if (GetItem(item.id) != nullptr) return false;
    items_.push_back(item);
    return true;
}

const ItemRecord* SystemicWorld::GetItem(ItemId id) const {
    for (const auto& i : items_) if (i.id == id) return &i;
    return nullptr;
}

bool SystemicWorld::TransferItem(ItemId id, EntityId to) {
    ItemRecord* item = const_cast<ItemRecord*>(GetItem(id));
    if (!item || !to.IsValid()) return false;
    // Physical transfer only: current_holder changes, legal_holder does not.
    item->current_holder = to;
    item->location = ItemLocationKind::Holder;
    item->container = ContainerId::Invalid();
    return true;
}

bool SystemicWorld::LoanItem(ItemId id, EntityId to) {
    if (!TransferItem(id, to)) return false;
    // Loan keeps the original legal holder.
    return true;
}

bool SystemicWorld::AuthorizedTransferItem(ItemId id, EntityId to) {
    ItemRecord* item = const_cast<ItemRecord*>(GetItem(id));
    if (!item || !to.IsValid()) return false;
    // Authorized transfer updates both current and legal holder.
    item->current_holder = to;
    item->legal_holder = to;
    item->location = ItemLocationKind::Holder;
    item->container = ContainerId::Invalid();
    return true;
}

bool SystemicWorld::TheftItem(ItemId id, EntityId to, uint64_t frame) {
    if (!TransferItem(id, to)) return false;
    // Theft is not automatically reported; discovery is a separate transition.
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.type = SystemicEventType::Theft;
    e.actor = to;
    e.target = GetItem(id) ? GetItem(id)->owner : EntityId::Invalid();
    e.frame = frame;
    e.legality = LegalityClass::Illegal;
    e.tags.push_back("item_theft");
    e.tags.push_back("not_reported_automatically");
    events_.push_back(e);
    return true;
}

bool SystemicWorld::ReportItemStolen(ItemId id, uint64_t frame) {
    ItemRecord* item = const_cast<ItemRecord*>(GetItem(id));
    if (!item) return false;
    item->reported_stolen = true;
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.type = SystemicEventType::Generic;
    e.actor = item->owner;
    e.target = item->current_holder;
    e.frame = frame;
    e.method = "reported_stolen";
    e.legality = LegalityClass::Legal;
    e.tags.push_back("reported_stolen");
    events_.push_back(e);
    return true;
}

bool SystemicWorld::ReturnItem(ItemId id, uint64_t frame) {
    const ItemRecord* item = GetItem(id);
    if (!item) return false;
    ItemRecord* it = const_cast<ItemRecord*>(item);
    // Return goes to the current legal holder, not necessarily the original owner.
    it->current_holder = it->legal_holder;
    it->location = ItemLocationKind::Holder;
    it->container = ContainerId::Invalid();
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.type = SystemicEventType::ItemReturn;
    e.frame = frame;
    e.tags.push_back("item_return");
    events_.push_back(e);
    return true;
}

bool SystemicWorld::DropItem(ItemId id, const Vec3& pos, RoomId room, uint64_t frame) {
    (void)frame;
    ItemRecord* item = const_cast<ItemRecord*>(GetItem(id));
    if (!item) return false;
    item->location = ItemLocationKind::Ground;
    item->ground_position = pos;
    item->room = room;
    item->container = ContainerId::Invalid();
    return true;
}

bool SystemicWorld::PlaceItemInContainer(ItemId id, ContainerId container, uint64_t frame) {
    (void)frame;
    const HideableContainer* c = GetContainer(container);
    if (!c) return false;
    ItemRecord* item = const_cast<ItemRecord*>(GetItem(id));
    if (!item) return false;
    item->location = ItemLocationKind::Container;
    item->container = container;
    return true;
}

bool SystemicWorld::RevokeCredential(ItemId id, uint64_t frame) {
    ItemRecord* item = const_cast<ItemRecord*>(GetItem(id));
    if (!item) return false;
    item->revoked = true;
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.type = SystemicEventType::RevokeCredential;
    e.frame = frame;
    e.tags.push_back("credential_revoked");
    events_.push_back(e);
    return true;
}

bool SystemicWorld::RestoreAuthorization(ItemId id, uint64_t frame) {
    ItemRecord* item = const_cast<ItemRecord*>(GetItem(id));
    if (!item) return false;
    item->revoked = false;
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.type = SystemicEventType::RestoreAuthorization;
    e.frame = frame;
    e.tags.push_back("credential_restored");
    events_.push_back(e);
    return true;
}

bool SystemicWorld::AddBody(const BodyRecord& body) {
    if (!body.id.IsValid()) return false;
    for (const auto& b : bodies_) if (b.id == body.id) return false;
    bodies_.push_back(body);
    return true;
}

const BodyRecord* SystemicWorld::GetBody(EntityId id) const {
    for (const auto& b : bodies_) if (b.id == id) return &b;
    return nullptr;
}

bool SystemicWorld::AddContainer(const HideableContainer& container) {
    if (!container.id.IsValid()) return false;
    if (GetContainer(container.id) != nullptr) return false;
    if (container.capacity_volume <= 0.0f) return false;
    containers_.push_back(container);
    return true;
}

const HideableContainer* SystemicWorld::GetContainer(ContainerId id) const {
    for (const auto& c : containers_) if (c.id == id) return &c;
    return nullptr;
}

bool SystemicWorld::BeginDrag(EntityId actor, EntityId body_id, uint64_t frame) {
    const BodyRecord* body = GetBody(body_id);
    if (!body || body->status == BodyStatus::Alive) return false;
    for (const auto& d : drags_) if (d.body == body_id) return false;
    BodyDragRecord d;
    d.actor = actor;
    d.body = body_id;
    d.room = body->room;
    d.position = body->position;
    d.started_frame = frame;
    d.last_update_frame = frame;
    drags_.push_back(d);
    BodyRecord* b = const_cast<BodyRecord*>(body);
    b->drag_status = BodyDragStatus::Dragging;
    return true;
}

bool SystemicWorld::UpdateDrag(EntityId body_id, const Vec3& pos, RoomId room, uint64_t frame) {
    for (auto& d : drags_) {
        if (d.body == body_id) {
            d.position = pos;
            d.room = room;
            d.last_update_frame = frame;
            BodyRecord* b = const_cast<BodyRecord*>(GetBody(body_id));
            if (b) { b->position = pos; b->room = room; }
            return true;
        }
    }
    return false;
}

bool SystemicWorld::EndDrag(EntityId body_id, uint64_t frame) {
    (void)frame;
    for (auto it = drags_.begin(); it != drags_.end(); ++it) {
        if (it->body == body_id) {
            drags_.erase(it);
            BodyRecord* b = const_cast<BodyRecord*>(GetBody(body_id));
            if (b) b->drag_status = BodyDragStatus::None;
            return true;
        }
    }
    return false;
}

const BodyDragRecord* SystemicWorld::GetDrag(EntityId body_id) const {
    for (const auto& d : drags_) if (d.body == body_id) return &d;
    return nullptr;
}

bool SystemicWorld::HideBody(EntityId body_id, ContainerId container_id) {
    BodyRecord* body = const_cast<BodyRecord*>(GetBody(body_id));
    HideableContainer* container = const_cast<HideableContainer*>(GetContainer(container_id));
    if (!body || !container) return false;
    if (body->status == BodyStatus::Alive) return false;
    if (body->disposition == BodyDisposition::HiddenInContainer) return false;
    if (body->drag_status != BodyDragStatus::None) return false;
    for (const auto& occ : container->current_occupants) {
        if (occ == body_id) return false;
    }
    constexpr float kBodyVolume = 0.40f;
    const float used = static_cast<float>(container->current_occupants.size()) * kBodyVolume;
    if (used + kBodyVolume > container->capacity_volume + 0.001f) return false;
    body->disposition = BodyDisposition::HiddenInContainer;
    body->container = container_id;
    body->room = container->room;
    body->position = container->position;
    container->current_occupants.push_back(body_id);
    return true;
}

bool SystemicWorld::CanDirectlyObserveBody(EntityId, EntityId body_id) const {
    const BodyRecord* body = GetBody(body_id);
    return body && body->disposition == BodyDisposition::Exposed;
}

bool SystemicWorld::DiscoverBody(NpcId discoverer, ContainerId container_id, uint64_t frame) {
    HideableContainer* container = const_cast<HideableContainer*>(GetContainer(container_id));
    if (!container || container->current_occupants.empty()) return false;
    const EntityId body_id = container->current_occupants.front();
    BodyRecord* body = const_cast<BodyRecord*>(GetBody(body_id));
    if (!body) return false;
    body->disposition = BodyDisposition::Exposed;
    body->container = ContainerId::Invalid();
    container->current_occupants.erase(
        std::remove(container->current_occupants.begin(), container->current_occupants.end(), body_id),
        container->current_occupants.end());

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
    memories_.push_back(mem);

    SystemicEvent evt;
    evt.id = EventId::New(events_.size() + 1);
    evt.type = SystemicEventType::BodyDiscovered;
    evt.actor = EntityId::New(discoverer.GetValue());
    evt.target = EntityId::New(body->npc.GetValue());
    evt.location = body->room;
    evt.frame = frame;
    evt.witnesses.push_back(discoverer);
    evt.severity = 50;
    evt.legality = LegalityClass::Unauthorized;
    evt.method = "container_open";
    evt.outcome = OutcomeType::Discovered;
    evt.evidence.push_back(ev.id);
    evt.tags.push_back("body_discovery");
    evt.tags.push_back("observation_only");
    events_.push_back(evt);
    return true;
}

bool SystemicWorld::ApplyDiscoveryResponse(EntityId actor, EventId discovery_event_id,
                                           BodyDiscoveryResponse response, uint64_t frame) {
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.actor = actor;
    e.frame = frame;
    e.source_world_event_id = discovery_event_id;
    bool found = false;
    for (const auto& ev : events_) {
        if (ev.id == discovery_event_id) { found = true; e.location = ev.location; break; }
    }
    if (!found) return false;
    switch (response) {
    case BodyDiscoveryResponse::ReportSecurity:
        e.type = SystemicEventType::Report;
        e.tags.push_back("report_security");
        if (alert_.level < FacilityAlertLevel::Suspicious) {
            SetAlert(FacilityAlertLevel::Suspicious, {e.location}, frame);
        }
        break;
    case BodyDiscoveryResponse::CallMedical:
        e.type = SystemicEventType::MedicalCall;
        e.tags.push_back("call_medical");
        break;
    case BodyDiscoveryResponse::HelpCoverUp:
        e.type = SystemicEventType::HelpCoverUp;
        e.tags.push_back("help_coverup");
        break;
    case BodyDiscoveryResponse::Ignore:
        e.type = SystemicEventType::Generic;
        e.tags.push_back("ignore");
        break;
    case BodyDiscoveryResponse::Investigate:
        e.type = SystemicEventType::WitnessObservation;
        e.tags.push_back("investigate");
        break;
    case BodyDiscoveryResponse::Flee:
        e.type = SystemicEventType::Generic;
        e.tags.push_back("flee");
        break;
    }
    events_.push_back(e);
    return true;
}

bool SystemicWorld::AddEvidence(const EvidenceRecord& evidence) {
    if (!evidence.id.IsValid() || GetEvidence(evidence.id) != nullptr) return false;
    evidence_.push_back(evidence);
    return true;
}

const EvidenceRecord* SystemicWorld::GetEvidence(EvidenceId id) const {
    for (const auto& e : evidence_) if (e.id == id) return &e;
    return nullptr;
}

bool SystemicWorld::AddMemory(const MemoryRecord& memory) {
    if (!memory.id.IsValid() || GetMemory(memory.id) != nullptr) return false;
    memories_.push_back(memory);
    return true;
}

const MemoryRecord* SystemicWorld::GetMemory(MemoryId id) const {
    for (const auto& m : memories_) if (m.id == id) return &m;
    return nullptr;
}

std::vector<MemoryRecord> SystemicWorld::MemoriesOf(EntityId actor) const {
    std::vector<MemoryRecord> out;
    for (const auto& m : memories_) if (m.npc == actor) out.push_back(m);
    return out;
}

bool SystemicWorld::SetRelationship(const RelationshipRecord& rel) {
    if (!rel.a.IsValid() || !rel.b.IsValid() || rel.a == rel.b) return false;
    if (!IsUnit(rel.trust) || !IsUnit(rel.fear) || !IsUnit(rel.respect) ||
        !IsUnit(rel.suspicion) || !IsUnit(rel.debt) || !IsUnit(rel.attachment) ||
        !IsFinite(rel.ideological_alignment) || rel.ideological_alignment < -1.0f ||
        rel.ideological_alignment > 1.0f) return false;
    for (auto& r : relationships_) {
        if (r.a == rel.a && r.b == rel.b) {
            r = rel;
            return true;
        }
    }
    relationships_.push_back(rel);
    return true;
}

const RelationshipRecord* SystemicWorld::GetRelationship(EntityId a, EntityId b) const {
    for (const auto& r : relationships_) {
        if (r.a == a && r.b == b) return &r;
    }
    return nullptr;
}

bool SystemicWorld::AddPromise(const PromiseRecord& promise) {
    if (!promise.id.IsValid() || GetPromise(promise.id) != nullptr) return false;
    if (promise.status != PromiseStatus::Offered && promise.status != PromiseStatus::Accepted) return false;
    promises_.push_back(promise);
    return true;
}

const PromiseRecord* SystemicWorld::GetPromise(PromiseId id) const {
    for (const auto& p : promises_) if (p.id == id) return &p;
    return nullptr;
}

bool SystemicWorld::TransitionPromise(PromiseId id, PromiseStatus next, uint64_t frame,
                                     const std::string& reason) {
    PromiseRecord* p = const_cast<PromiseRecord*>(GetPromise(id));
    if (!p) return false;
    const PromiseStatus cur = p->status;
    bool legal = false;
    SystemicEventType ev_type = SystemicEventType::PromiseOffered;
    if (cur == PromiseStatus::Offered && next == PromiseStatus::Accepted) {
        legal = true; ev_type = SystemicEventType::PromiseAccepted;
    } else if (cur == PromiseStatus::Offered && next == PromiseStatus::Cancelled) {
        legal = true; ev_type = SystemicEventType::PromiseOffered; // cancelled initial offer
    } else if (cur == PromiseStatus::Accepted && next == PromiseStatus::Fulfilled) {
        legal = true; ev_type = SystemicEventType::PromiseFulfilled;
    } else if (cur == PromiseStatus::Accepted && next == PromiseStatus::Broken) {
        legal = true; ev_type = SystemicEventType::PromiseBroken;
    } else if (cur == PromiseStatus::Accepted && next == PromiseStatus::Expired) {
    } else if (cur == PromiseStatus::Accepted && next == PromiseStatus::Expired) {
        legal = true; ev_type = SystemicEventType::Generic; // distinct expired, not broken
    } else if (cur == PromiseStatus::Accepted && next == PromiseStatus::Cancelled) {
        legal = true; ev_type = SystemicEventType::Generic;
    }
    if (!legal) return false;
    p->status = next;
    p->transition_frame = frame;
    p->reason = reason;
    p->storylet_eligible = true;
    if (next == PromiseStatus::Broken) {
        ++player_state_.promises_broken;
        player_state_.reliability = std::max(0.0f, player_state_.reliability - 0.08f);
        for (auto& r : relationships_) {
            if (r.a == p->giver && r.b == p->receiver) {
                r.trust = std::max(0.0f, r.trust - 0.10f);
                r.debt = std::min(1.0f, r.debt + 0.10f);
            }
        }
    } else if (next == PromiseStatus::Fulfilled) {
        player_state_.reliability = std::min(1.0f, player_state_.reliability + 0.05f);
        for (auto& r : relationships_) {
            if (r.a == p->giver && r.b == p->receiver) {
                r.trust = std::min(1.0f, r.trust + 0.10f);
                r.debt = std::max(0.0f, r.debt - 0.10f);
            }
        }
    }
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.type = ev_type;
    e.actor = p->giver;
    e.target = p->receiver;
    e.frame = frame;
    e.method = p->subject;
    e.tags.push_back("promise");
    if (next == PromiseStatus::Expired) e.tags.push_back("promise_expired");
    e.tags.push_back(p->reason);
    events_.push_back(e);
    return true;
}

bool SystemicWorld::AddQuest(const QuestRecord& quest) {
    if (!quest.id.IsValid() || GetQuest(quest.id) != nullptr) return false;
    if (quest.status != QuestStatus::Offered) return false;
    quests_.push_back(quest);
    return true;
}

const QuestRecord* SystemicWorld::GetQuest(QuestId id) const {
    for (const auto& q : quests_) if (q.id == id) return &q;
    return nullptr;
}

bool SystemicWorld::TransitionQuest(QuestId id, QuestStatus next, uint64_t frame, const std::string& reason) {
    QuestRecord* q = const_cast<QuestRecord*>(GetQuest(id));
    if (!q) return false;
    const QuestStatus cur = q->status;
    bool legal = false;
    if (cur == QuestStatus::Offered && next == QuestStatus::Accepted) legal = true;
    else if (cur == QuestStatus::Accepted && next == QuestStatus::Active) legal = true;
    else if (cur == QuestStatus::Active && (next == QuestStatus::Completed || next == QuestStatus::Failed ||
             next == QuestStatus::Expired || next == QuestStatus::Abandoned || next == QuestStatus::Betrayed)) legal = true;
    else if (cur == QuestStatus::Accepted && next == QuestStatus::Abandoned) legal = true;
    if (!legal) return false;
    q->status = next;
    q->transition_frame = frame;
    q->reason = reason;
    return true;
}

bool SystemicWorld::AddKnowledgeAsset(const KnowledgeAssetRecord& asset) {
    if (!asset.id.IsValid() || GetKnowledgeAsset(asset.id) != nullptr) return false;
    knowledge_.push_back(asset);
    return true;
}

const KnowledgeAssetRecord* SystemicWorld::GetKnowledgeAsset(KnowledgeAssetId id) const {
    for (const auto& k : knowledge_) if (k.id == id) return &k;
    return nullptr;
}

SearchOutcome SystemicWorld::PerformSearch(const SearchAction& action) {
    SearchOutcome out;
    searches_.push_back(action);
    if (action.target_type == SearchTargetType::Body) {
        BodyRecord* b = const_cast<BodyRecord*>(GetBody(action.target));
        if (b) {
            b->searched = true;
            out.success = true;
            out.legality = action.consent ? LegalityClass::Legal : LegalityClass::Unauthorized;
        }
    } else {
        out.success = true;
        out.legality = action.consent ? LegalityClass::Legal : LegalityClass::Unauthorized;
    }
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.type = SystemicEventType::SearchPerformed;
    e.actor = action.actor;
    e.target = action.target;
    e.location = action.room;
    e.frame = action.frame;
    e.legality = out.legality;
    events_.push_back(e);
    return out;
}

bool SystemicWorld::AddSocialExchange(const SocialExchangeRecord& exchange) {
    if (!exchange.id.IsValid() || GetSocialExchange(exchange.id) != nullptr) return false;
    exchanges_.push_back(exchange);
    return true;
}

const SocialExchangeRecord* SystemicWorld::GetSocialExchange(SocialExchangeId id) const {
    for (const auto& e : exchanges_) if (e.id == id) return &e;
    return nullptr;
}

bool SystemicWorld::AddTerminal(const TerminalRecord& terminal) {
    if (!terminal.id.IsValid() || GetTerminal(terminal.id) != nullptr) return false;
    terminals_.push_back(terminal);
    return true;
}

const TerminalRecord* SystemicWorld::GetTerminal(TerminalId id) const {
    for (const auto& t : terminals_) if (t.id == id) return &t;
    return nullptr;
}

bool SystemicWorld::AddTerminalSession(const TerminalSession& session) {
    if (session.terminal.GetValue() == 0) return false;
    sessions_.push_back(session);
    return true;
}

bool SystemicWorld::AddTerminalAudit(const TerminalAuditLog& audit) {
    if (audit.terminal.GetValue() == 0) return false;
    audits_.push_back(audit);
    return true;
}

bool SystemicWorld::AddObservationSource(const ObservationSource& source) {
    if (!source.id.IsValid() || GetObservationSource(source.id) != nullptr) return false;
    sources_.push_back(source);
    return true;
}

const ObservationSource* SystemicWorld::GetObservationSource(ObservationSourceId id) const {
    for (const auto& s : sources_) if (s.id == id) return &s;
    return nullptr;
}

bool SystemicWorld::NarratorObserves(ObservationSourceId id) const {
    const ObservationSource* s = GetObservationSource(id);
    return s && s->online;
}

bool SystemicWorld::NarratorCanObserveRoom(RoomId room, ObservationSourceType type) const {
    for (const auto& s : sources_) {
        if (s.room == room && s.type == type && s.online) return true;
    }
    return false;
}

void SystemicWorld::SetAlert(FacilityAlertLevel level, const std::vector<RoomId>& scope,
                             uint64_t frame) {
    alert_.level = level;
    alert_.scope = scope;
    alert_.last_change_frame = frame;
}

float SystemicWorld::DerivedAlertScalar() const {
    return static_cast<float>(alert_.level) / static_cast<float>(FacilityAlertLevel::Critical);
}

bool SystemicWorld::AddSystemicEvent(const SystemicEvent& event) {
    if (!event.id.IsValid()) return false;
    for (const auto& e : events_) if (e.id == event.id) return false;
    events_.push_back(event);
    return true;
}

bool SystemicWorld::BridgeWorldEventOnce(const WorldEvent& we) {
    for (const auto& e : events_) {
        if (e.source_world_event_id == we.id) return false;
    }
    SystemicEvent e;
    e.id = EventId::New(events_.size() + 1);
    e.source_world_event_id = we.id;
    e.actor = we.source_entity;
    e.target = we.target_entity;
    e.frame = we.sim_frame;
    e.type = SystemicEventType::Generic;
    if (std::holds_alternative<EventWeaponFire>(we.payload)) {
        e.type = SystemicEventType::IllegalWeapon;
    } else if (std::holds_alternative<EventDoorChange>(we.payload) ||
               std::holds_alternative<EventPowerToggle>(we.payload)) {
        e.type = SystemicEventType::InfrastructureChange;
    } else if (std::holds_alternative<EventNpcStateChange>(we.payload)) {
        e.type = SystemicEventType::BodyIncapacitated;
    } else if (std::holds_alternative<EventFactLearned>(we.payload)) {
        e.type = SystemicEventType::MemoryFormed;
    } else if (std::holds_alternative<EventPlayerDamage>(we.payload)) {
        e.type = SystemicEventType::Assault;
    } else if (std::holds_alternative<EventNpcSpeak>(we.payload)) {
        e.type = SystemicEventType::Communication;
    }
    e.tags.push_back("world_event_bridge");
    events_.push_back(e);
    return true;
}

bool SystemicWorld::ReaderAcceptsItem(ItemId id, uint8_t required_clearance) const {
    const ItemRecord* item = GetItem(id);
    if (!item || item->revoked) return false;
    return item->credential_level >= required_clearance;
}

bool SystemicWorld::NpcAcceptsPresentedIdentity(NpcId observer, EntityId expected_owner,
                                                EntityId presenter) const {
    const ActorRecord* actor = GetActor(observer);
    if (!actor) return true;
    const bool knows = std::find(actor->known_identities.begin(), actor->known_identities.end(),
                                 expected_owner) != actor->known_identities.end();
    if (!knows) return true;
    return presenter == expected_owner;
}

IdentityReaction SystemicWorld::ReactionToIdentityMismatch(NpcId observer,
                                                           EntityId original_owner,
                                                           EntityId presenter) const {
    const ActorRecord* actor = GetActor(observer);
    if (!actor) return IdentityReaction::Accept;
    const bool knows = std::find(actor->known_identities.begin(), actor->known_identities.end(),
                                 original_owner) != actor->known_identities.end();
    if (!knows) return IdentityReaction::Accept;
    const EntityId observer_entity = EntityId::New(observer.GetValue());
    const RelationshipRecord* rel_presenter = GetRelationship(observer_entity, presenter);
    const RelationshipRecord* rel_owner = GetRelationship(observer_entity, original_owner);
    const float trust_in_presenter = rel_presenter ? rel_presenter->trust : 0.0f;
    const float fear_of_presenter = rel_presenter ? rel_presenter->fear : 0.0f;
    const float debt_to_presenter = rel_presenter ? rel_presenter->debt : 0.0f;
    const float trust_in_owner = rel_owner ? rel_owner->trust : 0.0f;
    if (trust_in_presenter > 0.6f && debt_to_presenter > 0.3f && fear_of_presenter < 0.4f)
        return IdentityReaction::HelpCoverUp;
    if (trust_in_owner > 0.6f && fear_of_presenter > 0.5f) return IdentityReaction::Report;
    if (fear_of_presenter < 0.25f && trust_in_presenter < 0.3f) return IdentityReaction::Confront;
    return IdentityReaction::Report;
}

void SystemicWorld::Save(Serializer& s) const {
    WriteRecordVector(s, actors_, &WriteActor);
    WriteRecordVector(s, items_, &WriteItem);
    WriteRecordVector(s, bodies_, &WriteBody);
    WriteRecordVector(s, containers_, &WriteContainer);
    WriteRecordVector(s, drags_, &WriteDrag);
    WriteRecordVector(s, evidence_, &WriteEvidence);
    WriteRecordVector(s, memories_, &WriteMemory);
    WriteRecordVector(s, relationships_, &WriteRelationship);
    WriteRecordVector(s, promises_, &WritePromise);
    WriteRecordVector(s, quests_, &WriteQuest);
    WriteRecordVector(s, knowledge_, &WriteKnowledge);
    WriteRecordVector(s, searches_, &WriteSearch);
    WriteRecordVector(s, exchanges_, &WriteExchange);
    WriteRecordVector(s, terminals_, &WriteTerminal);
    WriteRecordVector(s, sessions_, &WriteSession);
    WriteRecordVector(s, audits_, &WriteAudit);
    WriteRecordVector(s, sources_, &WriteSource);
    WriteRecordVector(s, events_, &WriteSystemicEvent);

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
    s.WriteF32(player_state_.infrastructure_integrity);
    s.WriteF32(player_state_.timeline_instability);
    s.WriteF32(player_state_.residual_memory_pressure);
    s.WriteU32(player_state_.civilian_casualties);
    s.WriteU32(player_state_.security_casualties);
    s.WriteU32(player_state_.promises_broken);
    s.WriteU32(player_state_.previous_cycle_evidence_count);
    s.WriteU8(player_state_.operator_room_found ? 1 : 0);

    s.WriteU8(authority_.authority_stage);
    s.WriteF32(authority_.intervention_cost);
    s.WriteU64(authority_.intervention_cooldown_frame);
    s.WriteU8(observability_.meta_observability ? 1 : 0);
    WriteRecordVector(s, observability_.sources, &WriteSource);

    s.WriteU8(static_cast<uint8_t>(alert_.level));
    s.WriteU64(alert_.last_change_frame);
    WriteIdVector(s, alert_.scope);
}

std::vector<uint8_t> SystemicWorld::Serialize() const {
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    Save(s);
    return bytes;
}

Result<SystemicWorld> SystemicWorld::Deserialize(const uint8_t* data, size_t size) {
    SystemicWorld w;
    Deserializer d(data, size);

    if (!ReadRecordVector(d, w.actors_, kMaxActors, &ReadActor) ||
        !ReadRecordVector(d, w.items_, kMaxItems, &ReadItem) ||
        !ReadRecordVector(d, w.bodies_, kMaxBodies, &ReadBody) ||
        !ReadRecordVector(d, w.containers_, kMaxContainers, &ReadContainer) ||
        !ReadRecordVector(d, w.drags_, kMaxDrags, &ReadDrag) ||
        !ReadRecordVector(d, w.evidence_, kMaxEvidence, &ReadEvidence) ||
        !ReadRecordVector(d, w.memories_, kMaxMemories, &ReadMemory) ||
        !ReadRecordVector(d, w.relationships_, kMaxRelationships, &ReadRelationship) ||
        !ReadRecordVector(d, w.promises_, kMaxPromises, &ReadPromise) ||
        !ReadRecordVector(d, w.quests_, kMaxQuests, &ReadQuest) ||
        !ReadRecordVector(d, w.knowledge_, kMaxKnowledge, &ReadKnowledge) ||
        !ReadRecordVector(d, w.searches_, kMaxSearches, &ReadSearch) ||
        !ReadRecordVector(d, w.exchanges_, kMaxExchanges, &ReadExchange) ||
        !ReadRecordVector(d, w.terminals_, kMaxTerminals, &ReadTerminal) ||
        !ReadRecordVector(d, w.sessions_, kMaxSessions, &ReadSession) ||
        !ReadRecordVector(d, w.audits_, kMaxAudits, &ReadAudit) ||
        !ReadRecordVector(d, w.sources_, kMaxSources, &ReadSource) ||
        !ReadRecordVector(d, w.events_, kMaxEvents, &ReadSystemicEvent)) {
        return Result<SystemicWorld>::Err(1000, "systemic deserialize failed: bounded read error");
    }

    w.player_state_.humanity = d.ReadF32();
    w.player_state_.violence = d.ReadF32();
    w.player_state_.reliability = d.ReadF32();
    w.player_state_.coercion = d.ReadF32();
    w.player_state_.public_trust = d.ReadF32();
    w.player_state_.security_standing = d.ReadF32();
    w.player_state_.medical_research_standing = d.ReadF32();
    w.player_state_.maintenance_standing = d.ReadF32();
    w.player_state_.narrator_alignment = d.ReadF32();
    w.player_state_.narrator_dominance = d.ReadF32();
    w.player_state_.autonomy = d.ReadF32();
    w.player_state_.truth_exposure = d.ReadF32();
    w.player_state_.self_knowledge = d.ReadF32();
    w.player_state_.infrastructure_integrity = d.ReadF32();
    w.player_state_.timeline_instability = d.ReadF32();
    w.player_state_.residual_memory_pressure = d.ReadF32();
    w.player_state_.civilian_casualties = d.ReadU32();
    w.player_state_.security_casualties = d.ReadU32();
    w.player_state_.promises_broken = d.ReadU32();
    w.player_state_.previous_cycle_evidence_count = d.ReadU32();
    w.player_state_.operator_room_found = d.ReadU8() != 0;

    w.authority_.authority_stage = d.ReadU8();
    w.authority_.intervention_cost = d.ReadF32();
    w.authority_.intervention_cooldown_frame = d.ReadU64();
    w.observability_.meta_observability = d.ReadU8() != 0;
    if (!ReadRecordVector(d, w.observability_.sources, kMaxSources, &ReadSource)) {
        return Result<SystemicWorld>::Err(1001, "systemic deserialize failed: observability read");
    }

    const uint8_t alert = d.ReadU8();
    if (!IsAlertLevel(alert)) return Result<SystemicWorld>::Err(1002, "invalid alert enum");
    w.alert_.level = static_cast<FacilityAlertLevel>(alert);
    w.alert_.last_change_frame = d.ReadU64();
    if (!ReadIdVectorBounded(d, w.alert_.scope)) {
        return Result<SystemicWorld>::Err(1003, "systemic deserialize failed: alert scope");
    }
    if (d.HasError() || !d.AtEnd()) {
        return Result<SystemicWorld>::Err(1004, "systemic deserialize failed: trailing/invalid data");
    }
    if (!IsUnit(w.player_state_.humanity) || !IsUnit(w.player_state_.violence) ||
        !IsUnit(w.player_state_.reliability) || !IsUnit(w.player_state_.coercion) ||
        !IsUnit(w.player_state_.public_trust) || !IsUnit(w.player_state_.security_standing) ||
        !IsUnit(w.player_state_.medical_research_standing) || !IsUnit(w.player_state_.maintenance_standing) ||
        !IsFinite(w.player_state_.narrator_alignment) || !IsFinite(w.player_state_.narrator_dominance) ||
        !IsUnit(w.player_state_.autonomy) || !IsUnit(w.player_state_.truth_exposure) ||
        !IsUnit(w.player_state_.self_knowledge) || !IsFinite(w.player_state_.infrastructure_integrity) ||
        !IsUnit(w.player_state_.timeline_instability) || !IsUnit(w.player_state_.residual_memory_pressure) ||
        !IsFinite(w.authority_.intervention_cost) || w.authority_.authority_stage > 3) {
        return Result<SystemicWorld>::Err(1005, "systemic deserialize failed: invalid state range");
    }

    std::string error;
    if (!ValidateWorld(w, error)) {
        return Result<SystemicWorld>::Err(1006, "systemic deserialize failed: " + error);
    }
    return Result<SystemicWorld>::Ok(std::move(w));
}

Result<void> SystemicWorld::LoadSeedBytes(const uint8_t* data, size_t size) {
    Deserializer d(data, size);
    if (d.Remaining() < 8) return Result<void>::Err(2000, "seed too small");
    const uint32_t magic = d.ReadU32();
    const uint32_t version = d.ReadU32();
    if (magic != 0x574F5344u || version != 1) {
        return Result<void>::Err(2001, "bad seed magic/version");
    }

    // Seed binary currently carries actors, items, containers, evidence, and
    // promises. Additional sections can be appended in later versions.
    std::vector<ActorRecord> actors;
    std::vector<ItemRecord> items;
    std::vector<HideableContainer> containers;
    std::vector<EvidenceRecord> evidence;
    std::vector<PromiseRecord> promises;
    if (!ReadRecordVector(d, actors, kMaxActors, &ReadActor) ||
        !ReadRecordVector(d, items, kMaxItems, &ReadItem) ||
        !ReadRecordVector(d, containers, kMaxContainers, &ReadContainer) ||
        !ReadRecordVector(d, evidence, kMaxEvidence, &ReadEvidence) ||
        !ReadRecordVector(d, promises, kMaxPromises, &ReadPromise) ||
        !d.AtEnd()) {
        return Result<void>::Err(2002, "invalid systemic seed");
    }
    for (auto& a : actors) if (!AddActor(a)) return Result<void>::Err(2003, "seed actor rejected");
    for (auto& i : items) if (!AddItem(i)) return Result<void>::Err(2004, "seed item rejected");
    for (auto& c : containers) if (!AddContainer(c)) return Result<void>::Err(2005, "seed container rejected");
    for (auto& e : evidence) if (!AddEvidence(e)) return Result<void>::Err(2006, "seed evidence rejected");
    for (auto& p : promises) if (!AddPromise(p)) return Result<void>::Err(2007, "seed promise rejected");
    return Result<void>::Ok();
}

Result<void> SystemicWorld::LoadSeedBinary(const std::string& path) {
    auto data = ReadFileBinary(path);
    if (data.IsError()) return Result<void>::Err(data.Error().code, data.Error().message);
    return LoadSeedBytes(data.Value().data(), data.Value().size());
}

} // namespace writeover
