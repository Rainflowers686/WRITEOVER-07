#pragma once
// Strong typed ID policy. Every identity is wrapped in its own struct so the
// compiler prevents mixing RoomId with NpcId etc. Bare uint64_t ids are a
// forbidden pattern (M-010 / 17.2 closure). 0 = invalid.

#include <cstdint>

namespace writeover {

#define WO_DEFINE_ID(Name)                                                    \
    struct Name {                                                             \
        uint64_t value = 0;                                                   \
        bool operator==(const Name& o) const { return value == o.value; }     \
        bool operator!=(const Name& o) const { return value != o.value; }     \
        bool operator<(const Name& o) const { return value < o.value; }       \
        uint64_t GetValue() const { return value; }                           \
        bool IsValid() const { return value != 0; }                           \
        static Name Invalid() { return Name{}; }                              \
        static Name New(uint64_t v) { Name n; n.value = v; return n; }        \
    };

WO_DEFINE_ID(EntityId)
WO_DEFINE_ID(EventId)
WO_DEFINE_ID(FactId)
WO_DEFINE_ID(ClaimId)
WO_DEFINE_ID(StoryletId)
WO_DEFINE_ID(RoomId)
WO_DEFINE_ID(ResourceId)   // content resource key (audio, text, data)
WO_DEFINE_ID(StringId)      // localization / semantic text key
WO_DEFINE_ID(NpcId)
WO_DEFINE_ID(DoorId)
WO_DEFINE_ID(SystemId)     // infrastructure system (power, door group, camera)
WO_DEFINE_ID(WeaponId)
WO_DEFINE_ID(ItemId)
WO_DEFINE_ID(EvidenceId)
WO_DEFINE_ID(PromiseId)
WO_DEFINE_ID(ContainerId)
WO_DEFINE_ID(MemoryId)
WO_DEFINE_ID(AudioId)
WO_DEFINE_ID(CheckpointId)

#undef WO_DEFINE_ID

// Serialization helpers for strong ids (little-endian u64 on the wire).
class Serializer;
class Deserializer;

template <typename IdT>
inline void WriteId(Serializer& s, IdT id);

template <typename IdT>
inline IdT ReadId(Deserializer& d);

} // namespace writeover