#pragma once
// Fact / Belief / Claim (M-010 closure).
// FactValue is a TYPED variant that can hold a 64-bit EntityId (a uint16_t
// predicateValue cannot). WorldFact uses strong SubjectRef/EntityId only.
// Four-layer separation is preserved: WorldFact -> Belief -> Claim -> UI.

#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"

#include <cstdint>
#include <map>
#include <optional>
#include <variant>
#include <vector>

namespace writeover {

using FactValue = std::variant<bool, int32_t, EntityId, RoomId, uint8_t>;

enum class PredicateType : uint8_t {
    State = 0,     // FactValue bool or uint8_t enum
    Relation = 1,  // FactValue EntityId / RoomId (64-bit supported)
    Count = 2,     // FactValue int32_t
};

struct WorldFact {
    FactId id;
    EntityId subject_entity;   // strong ref; never bare uint64_t
    PredicateType predicate = PredicateType::State;
    FactValue value;
};

// Single writer: WorldSim/world systems mutate facts; everyone else reads.
class FactStore {
public:
    void Set(WorldFact fact);
    bool Exists(FactId id) const;
    bool Get(FactId id, WorldFact& out) const;
    std::vector<WorldFact> Snapshot() const;   // ordered by id (deterministic)

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    std::map<FactId, WorldFact> facts_;  // ordered, deterministic
};

enum class BeliefSource : uint8_t {
    Observed = 0,
    Heard = 1,
    Inferred = 2,
    Told = 3,
};

struct Belief {
    FactId fact_id;
    NpcId npc_id;
    float confidence = 0.0f;          // 0..1
    uint32_t last_updated_frame = 0;
    BeliefSource source = BeliefSource::Observed;
};

class BeliefSet {
public:
    void Upsert(NpcId npc, FactId fact, float confidence,
                BeliefSource source, uint32_t frame);
    float Confidence(NpcId npc, FactId fact) const;
    std::vector<Belief> BeliefsOf(NpcId npc) const;
    size_t Count() const { return beliefs_.size(); }

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    std::map<NpcId, std::map<FactId, Belief>> beliefs_;
};

enum class Veracity : uint8_t {
    True = 0,
    False = 1,
    Misleading = 2,
    Distorted = 3,
};

struct Claim {
    ClaimId id;
    FactId underlying_fact_id;
    NpcId speaker_id;      // or kNarratorId special value
    NpcId audience_id;     // invalid = everyone
    Veracity veracity = Veracity::True;
};

// Narrator is represented as a special NpcId so claims don't need a raw enum.
inline NpcId NarratorSpeakerId() { return NpcId::New(0xFFFFFFFFFFFFFFFEull); }

} // namespace writeover