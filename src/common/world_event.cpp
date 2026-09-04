#include "writeover/common/world_event.h"

#include "writeover/common/serialize.h"

#include <cmath>
#include <cstdint>
#include <set>
#include <type_traits>
#include <utility>

namespace writeover {

namespace {

struct PayloadSerializer {
    Serializer& s;
    void operator()(const EventWeaponFire& v) {
        WriteId(s, v.shooter);
        s.WriteU8(static_cast<uint8_t>(v.slot));
        s.WriteF32(v.origin.x);
        s.WriteF32(v.origin.y);
        s.WriteF32(v.origin.z);
        s.WriteF32(v.yaw);
        s.WriteF32(v.pitch);
        s.WriteF32(v.loudness);
    }
    void operator()(const EventDamage& v) {
        WriteId(s, v.target);
        WriteId(s, v.source);
        s.WriteU16(v.amount);
        s.WriteU8(v.damage_type);
        s.WriteU8(v.headshot ? 1 : 0);
    }
    void operator()(const EventDoorChange& v) {
        WriteId(s, v.door);
        s.WriteU8(v.open ? 1 : 0);
    }
    void operator()(const EventPowerToggle& v) {
        WriteId(s, v.system);
        s.WriteU8(v.powered ? 1 : 0);
    }
    void operator()(const EventNpcStateChange& v) {
        WriteId(s, v.npc);
        s.WriteU8(v.new_state);
    }
    void operator()(const EventStoryletTrigger& v) {
        WriteId(s, v.storylet);
        WriteId(s, v.trigger);
    }
    void operator()(const EventFactLearned& v) {
        WriteId(s, v.fact);
        WriteId(s, v.learner);
    }
    void operator()(const EventPlayerDamage& v) {
        s.WriteU16(v.amount);
        s.WriteU8(v.damage_type);
        WriteId(s, v.source);
    }
    void operator()(const EventGameOver& v) { s.WriteU8(v.ending_index); }
    void operator()(const EventNpcSpeak& v) {
        WriteId(s, v.npc);
        WriteId(s, v.line);
    }
};

struct PayloadDeserializer {
    Deserializer& d;
    EventPayload operator()(const EventWeaponFire&) {
        EventWeaponFire v;
        v.shooter = ReadId<EntityId>(d);
        v.slot = static_cast<WeaponSlot>(d.ReadU8());
        v.origin.x = d.ReadF32();
        v.origin.y = d.ReadF32();
        v.origin.z = d.ReadF32();
        v.yaw = d.ReadF32();
        v.pitch = d.ReadF32();
        v.loudness = d.ReadF32();
        return v;
    }
    EventPayload operator()(const EventDamage&) {
        EventDamage v;
        v.target = ReadId<EntityId>(d);
        v.source = ReadId<EntityId>(d);
        v.amount = d.ReadU16();
        v.damage_type = d.ReadU8();
        const uint8_t headshot = d.ReadU8();
        if (headshot > 1) d.MarkError();
        v.headshot = headshot != 0;
        return v;
    }
    EventPayload operator()(const EventDoorChange&) {
        EventDoorChange v;
        v.door = ReadId<DoorId>(d);
        const uint8_t open = d.ReadU8();
        if (open > 1) d.MarkError();
        v.open = open != 0;
        return v;
    }
    EventPayload operator()(const EventPowerToggle&) {
        EventPowerToggle v;
        v.system = ReadId<SystemId>(d);
        const uint8_t powered = d.ReadU8();
        if (powered > 1) d.MarkError();
        v.powered = powered != 0;
        return v;
    }
    EventPayload operator()(const EventNpcStateChange&) {
        EventNpcStateChange v;
        v.npc = ReadId<NpcId>(d);
        v.new_state = d.ReadU8();
        return v;
    }
    EventPayload operator()(const EventStoryletTrigger&) {
        EventStoryletTrigger v;
        v.storylet = ReadId<StoryletId>(d);
        v.trigger = ReadId<EntityId>(d);
        return v;
    }
    EventPayload operator()(const EventFactLearned&) {
        EventFactLearned v;
        v.fact = ReadId<FactId>(d);
        v.learner = ReadId<NpcId>(d);
        return v;
    }
    EventPayload operator()(const EventPlayerDamage&) {
        EventPlayerDamage v;
        v.amount = d.ReadU16();
        v.damage_type = d.ReadU8();
        v.source = ReadId<EntityId>(d);
        return v;
    }
    EventPayload operator()(const EventGameOver&) {
        EventGameOver v;
        v.ending_index = d.ReadU8();
        if (v.ending_index > 3) d.MarkError();
        return v;
    }
    EventPayload operator()(const EventNpcSpeak&) {
        EventNpcSpeak v;
        v.npc = ReadId<NpcId>(d);
        v.line = ReadId<StringId>(d);
        return v;
    }
};

} // namespace

void SerializePayload(Serializer& s, const EventPayload& p) {
    const size_t index = p.index();
    s.WriteU16(static_cast<uint16_t>(index));
    std::visit(PayloadSerializer{s}, p);
}

EventPayload DeserializePayload(Deserializer& d) {
    const uint16_t index = d.ReadU16();
    switch (index) {
    case 0: return PayloadDeserializer{d}(EventWeaponFire{});
    case 1: return PayloadDeserializer{d}(EventDamage{});
    case 2: return PayloadDeserializer{d}(EventDoorChange{});
    case 3: return PayloadDeserializer{d}(EventPowerToggle{});
    case 4: return PayloadDeserializer{d}(EventNpcStateChange{});
    case 5: return PayloadDeserializer{d}(EventStoryletTrigger{});
    case 6: return PayloadDeserializer{d}(EventFactLearned{});
    case 7: return PayloadDeserializer{d}(EventPlayerDamage{});
    case 8: return PayloadDeserializer{d}(EventGameOver{});
    case 9: return PayloadDeserializer{d}(EventNpcSpeak{});
    default:
        // Schema drift: the containing save section must be rejected.
        d.MarkError();
        return EventPayload(EventNpcSpeak{});
    }
}

namespace {
bool Finite(float value) { return std::isfinite(value); }

bool ValidPayload(const EventPayload& payload) {
    return std::visit([](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, EventWeaponFire>) {
            return value.shooter.IsValid() &&
                   static_cast<uint8_t>(value.slot) < kWeaponSlotCount &&
                   Finite(value.origin.x) && Finite(value.origin.y) &&
                   Finite(value.origin.z) && Finite(value.yaw) &&
                   Finite(value.pitch) && Finite(value.loudness) &&
                   value.loudness >= 0.0f && value.loudness <= 1.0f;
        } else if constexpr (std::is_same_v<T, EventDamage>) {
            return value.target.IsValid() && value.source.IsValid() &&
                   value.damage_type <= 1;
        } else if constexpr (std::is_same_v<T, EventDoorChange>) {
            return value.door.IsValid();
        } else if constexpr (std::is_same_v<T, EventPowerToggle>) {
            return value.system.IsValid();
        } else if constexpr (std::is_same_v<T, EventNpcStateChange>) {
            return value.npc.IsValid() && value.new_state <= 10;
        } else if constexpr (std::is_same_v<T, EventStoryletTrigger>) {
            return value.storylet.IsValid() && value.trigger.IsValid();
        } else if constexpr (std::is_same_v<T, EventFactLearned>) {
            return value.fact.IsValid() && value.learner.IsValid();
        } else if constexpr (std::is_same_v<T, EventPlayerDamage>) {
            return value.source.IsValid() && value.damage_type <= 1;
        } else if constexpr (std::is_same_v<T, EventGameOver>) {
            return value.ending_index <= 3;
        } else if constexpr (std::is_same_v<T, EventNpcSpeak>) {
            return value.npc.IsValid() && value.line.IsValid();
        } else {
            return false;
        }
    }, payload);
}
} // namespace

void SerializeWorldEvent(Serializer& s, const WorldEvent& e) {
    WriteId(s, e.id);
    s.WriteU64(e.sim_frame);
    s.WriteU8(static_cast<uint8_t>(e.kind));
    WriteId(s, e.parent_event_id);
    WriteId(s, e.source_entity);
    WriteId(s, e.target_entity);
    SerializePayload(s, e.payload);
}

WorldEvent DeserializeWorldEvent(Deserializer& d) {
    WorldEvent e;
    e.id = ReadId<EventId>(d);
    e.sim_frame = d.ReadU64();
    const uint8_t kind = d.ReadU8();
    e.kind = static_cast<EventKind>(kind);
    e.parent_event_id = ReadId<EventId>(d);
    e.source_entity = ReadId<EntityId>(d);
    e.target_entity = ReadId<EntityId>(d);
    e.payload = DeserializePayload(d);
    if (!e.id.IsValid() || kind > static_cast<uint8_t>(EventKind::Notification) ||
        !ValidPayload(e.payload)) {
        d.MarkError();
    }
    return e;
}

EventId EventBus::Post(EventPayload payload, EventKind kind,
                       EntityId source, EntityId target, EventId parent,
                       uint64_t sim_frame) {
    WorldEvent evt;
    evt.id = EventId::New(next_event_id_++);
    evt.sim_frame = sim_frame;
    evt.kind = kind;
    evt.parent_event_id = parent;
    evt.source_entity = source;
    evt.target_entity = target;
    evt.payload = std::move(payload);
    next_pending_.push_back(std::move(evt));
    return evt.id;
}

void EventBus::Dispatch() {
    for (auto& evt : pending_) {
        for (const auto& consumer : consumers_) {
            consumer.second(evt);
        }
        journal_.push_back(evt);
        if (journal_.size() > kJournalCapacity) {
            journal_.erase(journal_.begin());
        }
    }
    pending_.clear();
    pending_.swap(next_pending_);
}

EventBus::ConsumerId EventBus::Register(Consumer consumer) {
    const ConsumerId id = next_consumer_id_++;
    consumers_.emplace_back(id, std::move(consumer));
    return id;
}

void EventBus::Unregister(ConsumerId id) {
    for (auto it = consumers_.begin(); it != consumers_.end(); ++it) {
        if (it->first == id) {
            consumers_.erase(it);
            return;
        }
    }
}

void EventBus::Save(Serializer& s) const {
    s.WriteU64(next_event_id_);
    s.WriteU64(pending_.size());
    for (const auto& evt : pending_) {
        SerializeWorldEvent(s, evt);
    }
    s.WriteU64(next_pending_.size());
    for (const auto& evt : next_pending_) {
        SerializeWorldEvent(s, evt);
    }
    s.WriteU64(journal_.size());
    for (const auto& evt : journal_) {
        SerializeWorldEvent(s, evt);
    }
}

void EventBus::Load(Deserializer& d) {
    constexpr uint64_t kMaxQueue = 500;
    const uint64_t next_event_id = d.ReadU64();
    const uint64_t pending_count = d.ReadU64();
    if (d.HasError() || next_event_id == 0 || pending_count > kMaxQueue) {
        d.MarkError();
        return;
    }
    std::vector<WorldEvent> pending;
    std::vector<WorldEvent> next_pending;
    std::vector<WorldEvent> journal;
    pending.reserve(static_cast<size_t>(pending_count));
    for (uint64_t i = 0; i < pending_count; ++i) {
        pending.push_back(DeserializeWorldEvent(d));
    }
    const uint64_t next_count = d.ReadU64();
    if (d.HasError() || next_count > kMaxQueue) {
        d.MarkError();
        return;
    }
    next_pending.reserve(static_cast<size_t>(next_count));
    for (uint64_t i = 0; i < next_count; ++i) {
        next_pending.push_back(DeserializeWorldEvent(d));
    }
    const uint64_t journal_count = d.ReadU64();
    if (d.HasError() || journal_count > kJournalCapacity) {
        d.MarkError();
        return;
    }
    journal.reserve(static_cast<size_t>(journal_count));
    for (uint64_t i = 0; i < journal_count; ++i) {
        journal.push_back(DeserializeWorldEvent(d));
    }
    if (d.HasError()) return;
    std::set<uint64_t> ids;
    const auto check_ids = [&ids](const std::vector<WorldEvent>& events) {
        for (const auto& event : events) {
            if (!ids.insert(event.id.GetValue()).second) return false;
        }
        return true;
    };
    if (!check_ids(pending) || !check_ids(next_pending) || !check_ids(journal)) {
        d.MarkError();
        return;
    }
    next_event_id_ = next_event_id;
    pending_ = std::move(pending);
    next_pending_ = std::move(next_pending);
    journal_ = std::move(journal);
}

} // namespace writeover
