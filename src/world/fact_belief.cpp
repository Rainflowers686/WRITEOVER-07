#include "writeover/world/fact_belief.h"

#include <algorithm>
#include <utility>

namespace writeover {

namespace {
void SerializeFactValue(Serializer& s, const FactValue& value) {
    const size_t index = value.index();
    s.WriteU8(static_cast<uint8_t>(index));
    switch (index) {
    case 0:
        s.WriteU8(std::get<bool>(value) ? 1 : 0);
        break;
    case 1:
        s.WriteI32(std::get<int32_t>(value));
        break;
    case 2:
        WriteId(s, std::get<EntityId>(value));
        break;
    case 3:
        WriteId(s, std::get<RoomId>(value));
        break;
    case 4:
        s.WriteU8(std::get<uint8_t>(value));
        break;
    default:
        break;
    }
}

FactValue DeserializeFactValue(Deserializer& d) {
    const uint8_t index = d.ReadU8();
    switch (index) {
    case 0:
        return FactValue(d.ReadU8() != 0);
    case 1:
        return FactValue(d.ReadI32());
    case 2:
        return FactValue(ReadId<EntityId>(d));
    case 3:
        return FactValue(ReadId<RoomId>(d));
    case 4:
        return FactValue(d.ReadU8());
    default:
        return FactValue(false);
    }
}
} // namespace

void FactStore::Set(WorldFact fact) { facts_[fact.id] = std::move(fact); }

bool FactStore::Exists(FactId id) const { return facts_.count(id) > 0; }

bool FactStore::Get(FactId id, WorldFact& out) const {
    const auto it = facts_.find(id);
    if (it == facts_.end()) {
        return false;
    }
    out = it->second;
    return true;
}

std::vector<WorldFact> FactStore::Snapshot() const {
    std::vector<WorldFact> out;
    out.reserve(facts_.size());
    for (const auto& kv : facts_) {
        out.push_back(kv.second);
    }
    return out;
}

void FactStore::Save(Serializer& s) const {
    s.WriteU32(static_cast<uint32_t>(facts_.size()));
    for (const auto& kv : facts_) {
        const WorldFact& f = kv.second;
        WriteId(s, f.id);
        WriteId(s, f.subject_entity);
        s.WriteU8(static_cast<uint8_t>(f.predicate));
        SerializeFactValue(s, f.value);
    }
}

void FactStore::Load(Deserializer& d) {
    facts_.clear();
    const uint32_t count = d.ReadU32();
    for (uint32_t i = 0; i < count; ++i) {
        WorldFact f;
        f.id = ReadId<FactId>(d);
        f.subject_entity = ReadId<EntityId>(d);
        f.predicate = static_cast<PredicateType>(d.ReadU8());
        f.value = DeserializeFactValue(d);
        facts_[f.id] = std::move(f);
    }
}

void BeliefSet::Upsert(NpcId npc, FactId fact, float confidence,
                       BeliefSource source, uint32_t frame) {
    confidence = std::max(0.0f, std::min(1.0f, confidence));
    Belief& b = beliefs_[npc][fact];
    b.npc_id = npc;
    b.fact_id = fact;
    b.confidence = confidence;
    b.source = source;
    b.last_updated_frame = frame;
}

float BeliefSet::Confidence(NpcId npc, FactId fact) const {
    const auto npc_it = beliefs_.find(npc);
    if (npc_it == beliefs_.end()) {
        return 0.0f;
    }
    const auto fact_it = npc_it->second.find(fact);
    if (fact_it == npc_it->second.end()) {
        return 0.0f;
    }
    return fact_it->second.confidence;
}

std::vector<Belief> BeliefSet::BeliefsOf(NpcId npc) const {
    std::vector<Belief> out;
    const auto npc_it = beliefs_.find(npc);
    if (npc_it == beliefs_.end()) {
        return out;
    }
    for (const auto& kv : npc_it->second) {
        out.push_back(kv.second);
    }
    return out;
}

void BeliefSet::Save(Serializer& s) const {
    s.WriteU32(static_cast<uint32_t>(beliefs_.size()));
    for (const auto& npc_kv : beliefs_) {
        WriteId(s, npc_kv.first);
        s.WriteU32(static_cast<uint32_t>(npc_kv.second.size()));
        for (const auto& fact_kv : npc_kv.second) {
            const Belief& b = fact_kv.second;
            WriteId(s, b.fact_id);
            s.WriteF32(b.confidence);
            s.WriteU32(b.last_updated_frame);
            s.WriteU8(static_cast<uint8_t>(b.source));
        }
    }
}

void BeliefSet::Load(Deserializer& d) {
    beliefs_.clear();
    const uint32_t npc_count = d.ReadU32();
    for (uint32_t i = 0; i < npc_count; ++i) {
        const NpcId npc = ReadId<NpcId>(d);
        const uint32_t fact_count = d.ReadU32();
        for (uint32_t j = 0; j < fact_count; ++j) {
            Belief b;
            b.fact_id = ReadId<FactId>(d);
            b.confidence = d.ReadF32();
            b.last_updated_frame = d.ReadU32();
            b.source = static_cast<BeliefSource>(d.ReadU8());
            b.npc_id = npc;
            beliefs_[npc][b.fact_id] = b;
        }
    }
}

} // namespace writeover