#pragma once
// Typed WorldEvent fabric (M-003 closure).
// A WorldEvent is a FACT that already happened. There is no Command/Query
// category: commands are WorldCommand requests, queries are read-only
// interfaces. Remaining kinds only describe what happened:
//   Mutation     -> world state changed (single writer: world systems)
//   Notification -> relevant information, no state change
// Presentation output (subtitle/HUD) is a separate queue, not a WorldEvent.

#include "writeover/common/ids.h"
#include "writeover/common/weapon_types.h"
#include "writeover/common/types.h"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace writeover {

class Serializer;
class Deserializer;

// --- Typed event payloads (no magic bytes; M-012 closure) ---
struct EventWeaponFire {
    EntityId shooter;
    WeaponSlot slot = WeaponSlot::Pistol;
    Vec3 origin;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float loudness = 0.8f;  // sound propagation input for AI
};
struct EventDamage {
    EntityId target;
    EntityId source;
    uint16_t amount = 0;
    uint8_t damage_type = 0;  // 0=kinetic, 1=electrical
    bool headshot = false;
};
struct EventDoorChange {
    DoorId door;
    bool open = false;
};
struct EventPowerToggle {
    SystemId system;
    bool powered = false;
};
struct EventNpcStateChange {
    NpcId npc;
    uint8_t new_state = 0;
};
struct EventStoryletTrigger {
    StoryletId storylet;
    EntityId trigger;
};
struct EventFactLearned {
    FactId fact;
    NpcId learner;
};
struct EventPlayerDamage {
    uint16_t amount = 0;
    uint8_t damage_type = 0;
    EntityId source;
};
struct EventGameOver {
    uint8_t ending_index = 0;
};
struct EventNpcSpeak {
    NpcId npc;
    StringId line;
};

using EventPayload = std::variant<
    EventWeaponFire, EventDamage, EventDoorChange, EventPowerToggle,
    EventNpcStateChange, EventStoryletTrigger, EventFactLearned,
    EventPlayerDamage, EventGameOver, EventNpcSpeak>;

enum class EventKind : uint8_t {
    Mutation = 0,     // a world fact changed
    Notification = 1, // an event that changed nothing
};

struct WorldEvent {
    EventId id;
    uint64_t sim_frame = 0;
    EventKind kind = EventKind::Notification;
    EventId parent_event_id;   // 0/invalid = root of a causality chain
    EntityId source_entity;    // who originated (0 = world/none)
    EntityId target_entity;    // who is affected (0 = none)
    EventPayload payload;
};

void SerializePayload(Serializer& s, const EventPayload& p);
EventPayload DeserializePayload(Deserializer& d);
void SerializeWorldEvent(Serializer& s, const WorldEvent& e);
WorldEvent DeserializeWorldEvent(Deserializer& d);

// --- EventBus: deterministic fan-out journal (M-004 closure) ---
// Dispatch occurs once per sim tick, at the end of the tick. Every consumer
// sees every event (insertion order). Events posted during dispatch go to the
// NEXT tick; there is no mid-dispatch mutation and no event stealing.
// No kMaxCascadeDepth pseudo-safety valve: reaction events are naturally
// deferred to the next tick by the phased loop; runaway reactors are caught
// by tests, not by a fake counter.
class EventBus {
public:
    using ConsumerId = uint32_t;
    using Consumer = std::function<void(const WorldEvent&)>;

    EventId Post(EventPayload payload, EventKind kind,
                 EntityId source, EntityId target, EventId parent,
                 uint64_t sim_frame);

    ConsumerId Register(Consumer consumer);
    void Unregister(ConsumerId id);

    // Dispatches the pending queue to all consumers; newly posted events are
    // moved to the next tick's queue. Appends to the internal bounded journal.
    void Dispatch();

    size_t PendingCount() const { return pending_.size(); }
    size_t JournalCount() const { return journal_.size(); }
    // Journal read-only access for tests (read-only; never consumes).
    const std::vector<WorldEvent>& JournalSnapshot() const { return journal_; }

    uint64_t NextEventId() const { return next_event_id_; }

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    std::vector<WorldEvent> pending_;
    std::vector<WorldEvent> next_pending_;
    std::vector<std::pair<ConsumerId, Consumer>> consumers_;
    std::vector<WorldEvent> journal_;   // bounded to kJournalCapacity
    ConsumerId next_consumer_id_ = 1;
    uint64_t next_event_id_ = 1;
    static constexpr size_t kJournalCapacity = 500;
};

} // namespace writeover