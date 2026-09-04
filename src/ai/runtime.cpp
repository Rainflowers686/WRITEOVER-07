#include "writeover/ai/runtime.h"

#include "writeover/common/math.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace writeover {

namespace {
constexpr size_t kMaxRuntimeNpcs = 64;
constexpr size_t kMaxReceipts = 1024;
constexpr size_t kMaxNoises = 64;
constexpr uint64_t kDecisionPeriodFrames = 12; // 10 Hz at 120 Hz sim
constexpr uint64_t kMemoryIdBase = 0xA100000000000000ull;
constexpr float kNpcRadius = 0.42f;

bool IsNpcState(uint8_t value) {
    return value < static_cast<uint8_t>(NPCState::Count);
}

uint64_t NextMemoryId(const SystemicWorld& world) {
    uint64_t candidate = kMemoryIdBase + static_cast<uint64_t>(world.MemoryCount()) + 1;
    while (world.GetMemory(MemoryId::New(candidate)) != nullptr) {
        ++candidate;
    }
    return candidate;
}

bool RayHitsNpc(const FireRequest& request, const RuntimeNpc& runtime,
                float yaw, float pitch, float& out_distance,
                bool& out_headshot) {
    const float dx = std::cos(yaw);
    const float dy = std::sin(yaw);
    const float rel_x = runtime.instance.position.x - request.origin.x;
    const float rel_y = runtime.instance.position.y - request.origin.y;
    const float along = rel_x * dx + rel_y * dy;
    if (along <= 0.0f) return false;
    const float side = rel_x * (-dy) + rel_y * dx;
    if (std::fabs(side) > kNpcRadius) return false;
    const float z_at_target = request.origin.z + std::tan(pitch) * along;
    const float bottom = runtime.instance.position.z;
    const float top = bottom + 1.8f;
    if (z_at_target < bottom || z_at_target > top) return false;
    out_distance = along;
    out_headshot = z_at_target > bottom + 1.35f;
    return true;
}

} // namespace

void AutonomousNpcSystem::Attach(SystemicWorld* systemic, EventBus* events,
                                  DeterministicRNG* sim_rng) {
    systemic_ = systemic;
    events_ = events;
    sim_rng_ = sim_rng;
    if (events_ != nullptr && event_consumer_id_ == 0) {
        event_consumer_id_ = events_->Register(
            [this](const WorldEvent& event) { OnWorldEvent(event); });
    }
}

bool AutonomousNpcSystem::AddNpc(const NPCInstance& npc, RoomId room) {
    if (!npc.id.IsValid() || !room.IsValid() || npcs_.size() >= kMaxRuntimeNpcs) {
        return false;
    }
    if (FindRuntimeNpc(npc.id) != nullptr) return false;
    RuntimeNpc runtime;
    runtime.instance = npc;
    runtime.room = room;
    npcs_.push_back(runtime);
    std::sort(npcs_.begin(), npcs_.end(), [](const RuntimeNpc& a, const RuntimeNpc& b) {
        return a.instance.id < b.instance.id;
    });
    return true;
}

bool AutonomousNpcSystem::ConfigureBodyDiscovery(NpcId cleaner, EntityId body,
                                                 ContainerId container,
                                                 uint64_t due_frame) {
    if (!cleaner.IsValid() || !body.IsValid() || !container.IsValid() ||
        FindRuntimeNpc(cleaner) == nullptr) {
        return false;
    }
    cleaner_npc_ = cleaner;
    discovery_body_ = body;
    discovery_container_ = container;
    discovery_due_frame_ = due_frame;
    discovery_complete_ = false;
    return true;
}

void AutonomousNpcSystem::OnWorldEvent(const WorldEvent& event) {
    if (const auto* fire = std::get_if<EventWeaponFire>(&event.payload)) {
        NoiseSource noise;
        noise.position = fire->origin;
        noise.loudness = std::clamp(fire->loudness, 0.0f, 1.0f);
        noise.sim_frame = static_cast<uint32_t>(std::min<uint64_t>(
            event.sim_frame, std::numeric_limits<uint32_t>::max()));
        if (noises_.size() >= kMaxNoises) noises_.erase(noises_.begin());
        noises_.push_back(noise);
    }
}

void AutonomousNpcSystem::Receipt(NpcId npc, AutonomousPhase phase,
                                  NPCState state, uint64_t frame, bool success) {
    if (receipts_.size() >= kMaxReceipts) {
        receipts_.erase(receipts_.begin(), receipts_.begin() + 5);
    }
    receipts_.push_back(AutonomousReceipt{npc, phase, state, frame, success});
}

bool AutonomousNpcSystem::AddObservationMemory(const RuntimeNpc& runtime,
                                                const PerceptionResult& perception,
                                                uint64_t frame) {
    if (systemic_ == nullptr || !runtime.instance.id.IsValid()) return false;
    MemoryRecord memory;
    memory.id = MemoryId::New(NextMemoryId(*systemic_));
    memory.npc = EntityId::New(runtime.instance.id.GetValue());
    memory.kind = perception.sees_player ? MemoryKind::Observation
                                         : MemoryKind::EventRecall;
    memory.subject = EntityId::New(1);
    memory.target = EntityId::New(1);
    memory.room = runtime.room;
    memory.frame = frame;
    memory.salience = perception.sees_player ? 0.85f : 0.55f;
    memory.confidence = perception.sees_player ? perception.sight_confidence :
                        std::clamp(perception.noise_loudness, 0.0f, 1.0f);
    memory.source = perception.sees_player ? KnowledgeSource::DirectWitness
                                           : KnowledgeSource::HeardSound;
    memory.text_key = perception.sees_player ? ResourceId::New(0xB1001)
                                             : ResourceId::New(0xB1002);
    memory.tags.push_back(perception.sees_player ? "player_observed" : "gunshot_heard");
    return systemic_->AddMemory(memory);
}

void AutonomousNpcSystem::Tick(uint64_t frame) {
    if (frame % kDecisionPeriodFrames != 0) return;

    noises_.erase(std::remove_if(noises_.begin(), noises_.end(),
                                 [frame](const NoiseSource& noise) {
        return frame > noise.sim_frame && frame - noise.sim_frame > 120;
    }), noises_.end());

    if (world_query_ != nullptr) {
        PerceptionSystem perception_system;
        for (auto& runtime : npcs_) {
            if (runtime.room != active_room_ ||
                runtime.instance.state == NPCState::Dead ||
                runtime.instance.state == NPCState::Stunned) {
                continue;
            }
            const PerceptionResult perception = perception_system.Update(
                runtime.instance, *world_query_, player_position_, player_eye_z_,
                noises_, static_cast<uint32_t>(std::min<uint64_t>(
                    frame, std::numeric_limits<uint32_t>::max())));
            const bool stimulus = perception.sees_player || perception.hears_noise;
            Receipt(runtime.instance.id, AutonomousPhase::Observe,
                    runtime.instance.state, frame, stimulus);
            if (stimulus) {
                const bool remembered = AddObservationMemory(runtime, perception, frame);
                runtime.player_observed = runtime.player_observed || perception.sees_player;
                Receipt(runtime.instance.id, AutonomousPhase::Remember,
                        runtime.instance.state, frame, remembered);
            } else {
                Receipt(runtime.instance.id, AutonomousPhase::Remember,
                        runtime.instance.state, frame, true);
            }

            Receipt(runtime.instance.id, AutonomousPhase::Evaluate,
                    runtime.instance.state, frame, true);
            const NPCState chosen = perception.sees_player ? NPCState::Alert :
                                    (perception.hears_noise ? NPCState::Investigate
                                                             : NPCState::Patrol);
            Receipt(runtime.instance.id, AutonomousPhase::Choose,
                    chosen, frame, true);
            const bool changed = runtime.instance.state != chosen;
            runtime.instance.state = chosen;
            runtime.instance.alertness = perception.sees_player ? 100 :
                                         (perception.hears_noise ? 65 : 20);
            runtime.instance.state_timer_frames = 120;
            if (changed && events_ != nullptr) {
                events_->Post(EventNpcStateChange{runtime.instance.id,
                                                   static_cast<uint8_t>(chosen)},
                               EventKind::Mutation,
                               EntityId::New(runtime.instance.id.GetValue()),
                               EntityId::New(1), EventId::Invalid(), frame);
            }
            if (runtime.instance.cognition == CognitionTier::Full &&
                perception.sees_player && events_ != nullptr) {
                events_->Post(EventNpcSpeak{runtime.instance.id, StringId::New(0xB1003)},
                               EventKind::Notification,
                               EntityId::New(runtime.instance.id.GetValue()),
                               EntityId::New(1), EventId::Invalid(), frame);
            }
            Receipt(runtime.instance.id, AutonomousPhase::Act,
                    runtime.instance.state, frame, true);
            if (stimulus) ++autonomous_loop_count_;
        }
    }
    TryBodyDiscovery(frame);
}

bool AutonomousNpcSystem::TryBodyDiscovery(uint64_t frame) {
    if (discovery_complete_ || systemic_ == nullptr || !cleaner_npc_.IsValid() ||
        frame < discovery_due_frame_) return false;
    const RuntimeNpc* cleaner = FindRuntimeNpc(cleaner_npc_);
    if (cleaner == nullptr || cleaner->room != active_room_) return false;
    const BodyRecord* body = systemic_->GetBody(discovery_body_);
    if (body == nullptr || body->disposition != BodyDisposition::HiddenInContainer) {
        return false;
    }
    if (!systemic_->DiscoverBody(cleaner_npc_, discovery_container_, frame)) return false;

    EventId discovery_event;
    const auto& events = systemic_->SystemEvents();
    for (auto it = events.rbegin(); it != events.rend(); ++it) {
        if (it->type == SystemicEventType::BodyDiscovered &&
            it->target == EntityId::New(body->npc.GetValue())) {
            discovery_event = it->id;
            break;
        }
    }
    if (!discovery_event.IsValid()) return false;
    const EntityId cleaner_entity = EntityId::New(cleaner_npc_.GetValue());
    const RelationshipRecord* rel = systemic_->GetRelationship(
        cleaner_entity, EntityId::New(1));
    BodyDiscoveryResponse response = BodyDiscoveryResponse::ReportSecurity;
    if (rel != nullptr && rel->debt > 0.35f && rel->trust > 0.55f) {
        response = BodyDiscoveryResponse::HelpCoverUp;
    } else if (body->status == BodyStatus::Unconscious) {
        response = BodyDiscoveryResponse::CallMedical;
    }
    const bool applied = systemic_->ApplyDiscoveryResponse(
        cleaner_entity, discovery_event, response, frame);
    discovery_complete_ = applied;
    if (applied) {
        ++discovery_response_count_;
        Receipt(cleaner_npc_, AutonomousPhase::Act, cleaner->instance.state,
                frame, true);
    }
    return applied;
}

ShotFeedback AutonomousNpcSystem::HandlePlayerShot(const FireRequest& request,
                                                   const WeaponDef& weapon,
                                                   uint64_t frame) {
    ShotFeedback feedback;
    feedback.hit.hit = false;
    if (world_query_ == nullptr || sim_rng_ == nullptr) {
        if (shot_feedback_callback_) shot_feedback_callback_(feedback);
        return feedback;
    }
    HitscanResult hit = ResolveHitscan(request, weapon, *world_query_, *sim_rng_);
    float nearest = hit.hit ? hit.distance : weapon.range_meters;
    NpcId nearest_npc;
    bool headshot = false;
    for (const auto& runtime : npcs_) {
        if (runtime.room != active_room_ || runtime.instance.state == NPCState::Dead ||
            runtime.instance.state == NPCState::Stunned) continue;
        float distance = 0.0f;
        bool candidate_headshot = false;
        if (!RayHitsNpc(request, runtime, hit.resolved_yaw, hit.resolved_pitch,
                        distance, candidate_headshot) || distance > weapon.range_meters ||
            distance >= nearest) continue;
        nearest = distance;
        nearest_npc = runtime.instance.id;
        headshot = candidate_headshot;
    }
    if (nearest_npc.IsValid()) {
        RuntimeNpc* target = FindRuntimeNpc(nearest_npc);
        hit.hit = true;
        hit.target_id = EntityId::New(nearest_npc.GetValue());
        hit.distance = nearest;
        hit.headshot = headshot;
        hit.damage = weapon.non_lethal ? 0 : weapon.damage;
        hit.hit_point = Vec3{request.origin.x + std::cos(hit.resolved_yaw) * nearest,
                             request.origin.y + std::sin(hit.resolved_yaw) * nearest,
                             request.origin.z + std::tan(hit.resolved_pitch) * nearest};
        feedback.target_was_npc = true;
        feedback.npc = nearest_npc;
        if (target != nullptr) {
            if (weapon.non_lethal) {
                target->instance.state = NPCState::Stunned;
                target->instance.health = 1;
                feedback.target_stunned = true;
            } else if (target->instance.health <= hit.damage) {
                target->instance.health = 0;
                target->instance.state = NPCState::Dead;
                feedback.target_died = true;
            } else {
                target->instance.health = static_cast<uint16_t>(
                    target->instance.health - hit.damage);
                target->instance.state = NPCState::Alert;
            }
            if (events_ != nullptr) {
                events_->Post(EventDamage{hit.target_id, EntityId::New(1), hit.damage,
                                          weapon.damage_type, hit.headshot},
                                EventKind::Mutation, EntityId::New(1),
                                hit.target_id, EventId::Invalid(), frame);
            }
            if (systemic_ != nullptr) {
                SystemicEvent event;
                event.id = EventId::New(0xA200000000000000ull +
                                        static_cast<uint64_t>(systemic_->EventCount()) + 1);
                event.type = weapon.non_lethal ? SystemicEventType::NonLethalTakedown :
                             (feedback.target_died ? SystemicEventType::Killing
                                                    : SystemicEventType::Assault);
                event.actor = EntityId::New(1);
                event.target = hit.target_id;
                event.location = active_room_;
                event.frame = frame;
                event.legality = LegalityClass::Illegal;
                event.outcome = OutcomeType::Success;
                event.method = weapon.non_lethal ? "stunner" : "firearm";
                event.tags.push_back(weapon.non_lethal ? "non_lethal" : "gunfire");
                systemic_->AddSystemicEvent(event);
            }
        }
    }
    feedback.hit = hit;
    if (shot_feedback_callback_) shot_feedback_callback_(feedback);
    return feedback;
}

NpcId AutonomousNpcSystem::FindNpcByEntity(EntityId entity) const {
    return NpcId::New(entity.GetValue());
}

RuntimeNpc* AutonomousNpcSystem::FindRuntimeNpc(NpcId id) {
    for (auto& runtime : npcs_) if (runtime.instance.id == id) return &runtime;
    return nullptr;
}

const RuntimeNpc* AutonomousNpcSystem::FindRuntimeNpc(NpcId id) const {
    for (const auto& runtime : npcs_) if (runtime.instance.id == id) return &runtime;
    return nullptr;
}

void AutonomousNpcSystem::Save(Serializer& serializer) const {
    serializer.WriteU32(1);
    serializer.WriteU32(static_cast<uint32_t>(npcs_.size()));
    for (const auto& runtime : npcs_) {
        WriteId(serializer, runtime.instance.id);
        WriteId(serializer, runtime.room);
        serializer.WriteF32(runtime.instance.position.x);
        serializer.WriteF32(runtime.instance.position.y);
        serializer.WriteF32(runtime.instance.position.z);
        serializer.WriteF32(runtime.instance.yaw);
        serializer.WriteU16(runtime.instance.health);
        serializer.WriteU8(runtime.instance.alertness);
        serializer.WriteU8(static_cast<uint8_t>(runtime.instance.state));
        serializer.WriteU32(runtime.instance.state_timer_frames);
        serializer.WriteU32(runtime.instance.plan_step);
        serializer.WriteU8(runtime.player_observed ? 1 : 0);
    }
    WriteId(serializer, cleaner_npc_);
    WriteId(serializer, discovery_body_);
    WriteId(serializer, discovery_container_);
    serializer.WriteU64(discovery_due_frame_);
    serializer.WriteU8(discovery_complete_ ? 1 : 0);
}

bool AutonomousNpcSystem::Load(Deserializer& deserializer) {
    const uint32_t version = deserializer.ReadU32();
    const uint32_t count = deserializer.ReadU32();
    if (deserializer.HasError() || version != 1 || count > kMaxRuntimeNpcs) {
        deserializer.MarkError();
        return false;
    }
    struct SavedNpc {
        NpcId id;
        RoomId room;
        Vec3 position;
        float yaw;
        uint16_t health;
        uint8_t alertness;
        uint8_t state;
        uint32_t timer;
        uint32_t plan;
        bool observed;
    };
    std::vector<SavedNpc> saved;
    saved.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        SavedNpc value;
        value.id = ReadId<NpcId>(deserializer);
        value.room = ReadId<RoomId>(deserializer);
        value.position.x = deserializer.ReadF32();
        value.position.y = deserializer.ReadF32();
        value.position.z = deserializer.ReadF32();
        value.yaw = deserializer.ReadF32();
        value.health = deserializer.ReadU16();
        value.alertness = deserializer.ReadU8();
        value.state = deserializer.ReadU8();
        value.timer = deserializer.ReadU32();
        value.plan = deserializer.ReadU32();
        value.observed = deserializer.ReadU8() != 0;
        if (deserializer.HasError() || !value.id.IsValid() || !value.room.IsValid() ||
            !std::isfinite(value.position.x) || !std::isfinite(value.position.y) ||
            !std::isfinite(value.position.z) || !std::isfinite(value.yaw) ||
            value.alertness > 100 || !IsNpcState(value.state)) {
            deserializer.MarkError();
            return false;
        }
        for (const auto& prior : saved) {
            if (prior.id == value.id) {
                deserializer.MarkError();
                return false;
            }
        }
        saved.push_back(value);
    }
    const NpcId cleaner = ReadId<NpcId>(deserializer);
    const EntityId body = ReadId<EntityId>(deserializer);
    const ContainerId container = ReadId<ContainerId>(deserializer);
    const uint64_t due = deserializer.ReadU64();
    const bool complete = deserializer.ReadU8() != 0;
    if (deserializer.HasError() || !deserializer.AtEnd()) {
        deserializer.MarkError();
        return false;
    }
    for (const auto& value : saved) {
        RuntimeNpc* runtime = FindRuntimeNpc(value.id);
        if (runtime == nullptr) {
            deserializer.MarkError();
            return false;
        }
        runtime->room = value.room;
        runtime->instance.position = value.position;
        runtime->instance.yaw = value.yaw;
        runtime->instance.health = value.health;
        runtime->instance.alertness = value.alertness;
        runtime->instance.state = static_cast<NPCState>(value.state);
        runtime->instance.state_timer_frames = value.timer;
        runtime->instance.plan_step = value.plan;
        runtime->player_observed = value.observed;
    }
    cleaner_npc_ = cleaner;
    discovery_body_ = body;
    discovery_container_ = container;
    discovery_due_frame_ = due;
    discovery_complete_ = complete;
    return true;
}

} // namespace writeover
