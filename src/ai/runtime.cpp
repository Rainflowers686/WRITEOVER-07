#include "writeover/ai/runtime.h"

#include "writeover/common/math.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace writeover {

namespace {
constexpr size_t kMaxRuntimeNpcs = 64;
constexpr size_t kMaxReceipts = 1024;
constexpr size_t kMaxNoises = 64;
constexpr uint64_t kDecisionPeriodFrames = 12; // 10 Hz at 120 Hz sim
constexpr uint64_t kMemoryIdBase = 0xA100000000000000ull;
constexpr uint64_t kMemoryRefreshWindowFrames = 600; // five seconds at 120 Hz
constexpr uint64_t kDiscoveryInspectionFrames = 60; // 0.5 seconds at 120 Hz
constexpr float kNpcRadius = 0.42f;
constexpr float kNpcHeight = 1.80f;
constexpr float kNpcMotorSpeed = 1.20f;
constexpr float kNavigationArrivalRadius = 0.80f;
// Guard range follows the authored medium sight envelope.  A shorter
// independent combat range made a correctly perceived player at the edge of
// the guard's documented 12 m envelope look invisible until the guard happened
// to path all the way in.
constexpr float kGuardCombatRange = PerceptionSystem::kMediumSightRange;
constexpr uint64_t kGuardAttackPeriodFrames = 60;

enum NavigationTask : uint8_t {
    kNavigationNone = 0,
    kNavigationPatrol = 1,
    kNavigationInvestigate = 2,
    kNavigationBodyDiscovery = 3,
    kNavigationCombat = 4,
};

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

bool Finite(const Vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

bool OverlapsPlayerPersonalSpace(const Vec3& actor_feet,
                                 const Vec3& player_feet,
                                 float actor_radius,
                                 float actor_height) {
    if (!Finite(actor_feet) || !Finite(player_feet) ||
        !std::isfinite(actor_radius) || !std::isfinite(actor_height) ||
        actor_radius <= 0.0f || actor_height <= 0.0f) {
        return false;
    }
    constexpr float kMotorPlayerRadius = 0.35f;
    constexpr float kMotorPlayerHeight = 1.80f;
    if (player_feet.z >= actor_feet.z + actor_height ||
        player_feet.z + kMotorPlayerHeight <= actor_feet.z) {
        return false;
    }
    const float dx = actor_feet.x - player_feet.x;
    const float dy = actor_feet.y - player_feet.y;
    const float combined_radius = actor_radius + kMotorPlayerRadius;
    return dx * dx + dy * dy < combined_radius * combined_radius;
}

bool PassesPlayerVisibility(float sight_confidence, float player_visibility) {
    // Perception already includes the single distance falloff.  Applying the
    // visibility factor to that value as a second distance-like attenuation
    // made guards become blind at the far end of their authored range.  Keep
    // normal visibility on the authored confidence threshold and reserve a
    // bounded extra threshold only for the deliberately very-low visibility
    // counterfactual (crouched/still/dark).
    const float visibility = std::clamp(player_visibility, 0.0f, 1.0f);
    constexpr float kBaseThreshold = 0.15f;
    constexpr float kLowVisibilityKnee = 0.20f;
    constexpr float kLowVisibilityPenalty = 8.0f;
    const float threshold = kBaseThreshold +
        std::max(0.0f, kLowVisibilityKnee - visibility) *
            kLowVisibilityPenalty;
    return sight_confidence >= threshold;
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

bool AutonomousNpcSystem::SetPatrolRoute(NpcId npc,
                                          const std::vector<Vec3>& points) {
    if (!npc.IsValid() || points.empty() || points.size() > 32) return false;
    RuntimeNpc* runtime = FindRuntimeNpc(npc);
    if (runtime == nullptr) return false;
    if (!std::all_of(points.begin(), points.end(), Finite)) return false;
    runtime->patrol_points = points;
    runtime->patrol_index = 0;
    runtime->navigation_path.clear();
    runtime->navigation_cursor = 0;
    runtime->has_navigation_goal = false;
    runtime->navigation_task = kNavigationNone;
    runtime->navigation_hold_until_frame = 0;
    runtime->route_failed = false;
    return true;
}

void AutonomousNpcSystem::SetNavigationGoal(RuntimeNpc& runtime,
                                             const Vec3& target,
                                             uint8_t task) {
    runtime.navigation_goal = target;
    runtime.navigation_task = task;
    runtime.has_navigation_goal = true;
    runtime.route_failed = !PlanRoute(runtime, target);
}

bool AutonomousNpcSystem::PlanRoute(RuntimeNpc& runtime, const Vec3& target) {
    runtime.navigation_path.clear();
    runtime.navigation_cursor = 0;
    if (world_query_ == nullptr || runtime.room != active_room_ || !Finite(target)) {
        return false;
    }
    const int width = world_query_->Width();
    const int height = world_query_->Height();
    if (width <= 0 || height <= 0 || width > 128 || height > 128) return false;
    const auto cell_for = [](float value, int limit) {
        return std::clamp(static_cast<int>(std::floor(value)), 0, limit - 1);
    };
    const GridCoord start{cell_for(runtime.instance.position.x, width),
                          cell_for(runtime.instance.position.y, height)};
    const GridCoord goal{cell_for(target.x, width), cell_for(target.y, height)};
    const auto index_for = [width](GridCoord cell) {
        return cell.row * width + cell.col;
    };
    const auto walkable = [&](GridCoord cell) {
        if (cell.col < 0 || cell.row < 0 || cell.col >= width || cell.row >= height ||
            world_query_->IsSolidAt(cell.col, cell.row)) {
            return false;
        }
        const GridCell data = world_query_->GetCell(cell.col, cell.row);
        if (data.Clearance() < kNpcHeight) return false;
        const Vec3 center{static_cast<float>(cell.col) + 0.5f,
                          static_cast<float>(cell.row) + 0.5f,
                          data.floor_height};
        // The player is a bounded dynamic obstacle for ordinary NPC work and
        // patrol routes.  Combat may deliberately approach the player, but
        // non-combat motor tasks must not route through the player's personal
        // space and leave the player permanently wedged by the NPC collider.
        if (runtime.navigation_task != kNavigationCombat && !(cell == start) &&
            OverlapsPlayerPersonalSpace(center, player_position_,
                                        kNpcRadius, kNpcHeight)) {
            return false;
        }
        const AABB body{{center.x - kNpcRadius, center.y - kNpcRadius, center.z},
                        {center.x + kNpcRadius, center.y + kNpcRadius,
                         center.z + kNpcHeight}};
        return !world_query_->AabbBlocked(body);
    };
    if (!walkable(start) || !walkable(goal)) return false;

    const int cell_count = width * height;
    std::vector<int> parent(static_cast<size_t>(cell_count), -2);
    std::queue<GridCoord> open;
    parent[static_cast<size_t>(index_for(start))] = -1;
    open.push(start);
    constexpr int kDirections[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!open.empty()) {
        const GridCoord current = open.front();
        open.pop();
        if (current == goal) break;
        for (const auto& direction : kDirections) {
            const GridCoord next{current.col + direction[0],
                                 current.row + direction[1]};
            if (next.col < 0 || next.row < 0 || next.col >= width || next.row >= height ||
                !walkable(next)) continue;
            const int next_index = index_for(next);
            if (parent[static_cast<size_t>(next_index)] != -2) continue;
            parent[static_cast<size_t>(next_index)] = index_for(current);
            open.push(next);
        }
    }
    if (parent[static_cast<size_t>(index_for(goal))] == -2) return false;

    std::vector<GridCoord> cells;
    for (GridCoord current = goal; !(current == start);) {
        cells.push_back(current);
        const int parent_index = parent[static_cast<size_t>(index_for(current))];
        current = GridCoord{parent_index % width, parent_index / width};
    }
    std::reverse(cells.begin(), cells.end());
    for (const GridCoord cell : cells) {
        const GridCell data = world_query_->GetCell(cell.col, cell.row);
        runtime.navigation_path.push_back(
            Vec3{static_cast<float>(cell.col) + 0.5f,
                 static_cast<float>(cell.row) + 0.5f, data.floor_height});
    }
    if (runtime.navigation_path.empty() &&
        ((runtime.instance.position.x - target.x) * (runtime.instance.position.x - target.x) +
         (runtime.instance.position.y - target.y) * (runtime.instance.position.y - target.y) >
         kNavigationArrivalRadius * kNavigationArrivalRadius)) {
        runtime.navigation_path.push_back(target);
    }
    runtime.route_failed = false;
    return true;
}

bool AutonomousNpcSystem::MoveAlongRoute(RuntimeNpc& runtime,
                                          float delta_seconds) {
    if (!runtime.has_navigation_goal) return true;
    if (runtime.navigation_cursor >= runtime.navigation_path.size()) {
        const float dx = runtime.navigation_goal.x - runtime.instance.position.x;
        const float dy = runtime.navigation_goal.y - runtime.instance.position.y;
        if (std::sqrt(dx * dx + dy * dy) <= kNavigationArrivalRadius) return true;
        if (!PlanRoute(runtime, runtime.navigation_goal)) {
            runtime.route_failed = true;
            return false;
        }
    }
    if (runtime.navigation_cursor >= runtime.navigation_path.size()) return true;
    const Vec3 waypoint = runtime.navigation_path[runtime.navigation_cursor];
    const float dx = waypoint.x - runtime.instance.position.x;
    const float dy = waypoint.y - runtime.instance.position.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance <= 0.05f) {
        ++runtime.navigation_cursor;
        return true;
    }
    const float step = std::min(distance, kNpcMotorSpeed * delta_seconds);
    const Vec3 candidate{
        runtime.instance.position.x + dx / distance * step,
        runtime.instance.position.y + dy / distance * step,
        runtime.instance.position.z};
    const AABB body{{candidate.x - kNpcRadius, candidate.y - kNpcRadius, candidate.z},
                    {candidate.x + kNpcRadius, candidate.y + kNpcRadius,
                     candidate.z + kNpcHeight}};
    if (runtime.navigation_task != kNavigationCombat &&
        player_target_active_ &&
        OverlapsPlayerPersonalSpace(candidate, player_position_,
                                    kNpcRadius, kNpcHeight)) {
        runtime.navigation_path.clear();
        runtime.navigation_cursor = 0;
        if (!PlanRoute(runtime, runtime.navigation_goal)) {
            runtime.route_failed = true;
        }
        return false;
    }
    if (world_query_ != nullptr && world_query_->AabbBlocked(body)) {
        runtime.navigation_path.clear();
        runtime.navigation_cursor = 0;
        if (!PlanRoute(runtime, runtime.navigation_goal)) {
            runtime.route_failed = true;
        }
        return false;
    }
    runtime.instance.position = candidate;
    runtime.instance.yaw = std::atan2(dy, dx);
    if (step >= distance - 0.001f) ++runtime.navigation_cursor;
    return true;
}

void AutonomousNpcSystem::UpdateMotor(uint64_t frame) {
    const uint64_t elapsed_frames = frame >= last_motor_frame_
        ? std::max<uint64_t>(1, std::min<uint64_t>(frame - last_motor_frame_, 12))
        : 1;
    last_motor_frame_ = frame;
    const float delta_seconds = static_cast<float>(elapsed_frames) / 120.0f;
    for (auto& runtime : npcs_) {
        if (runtime.room != active_room_ || runtime.instance.state == NPCState::Dead ||
            runtime.instance.state == NPCState::Stunned ||
            !runtime.has_navigation_goal) continue;
        if (!MoveAlongRoute(runtime, delta_seconds)) continue;
        if (runtime.navigation_cursor < runtime.navigation_path.size()) continue;
        const float dx = runtime.navigation_goal.x - runtime.instance.position.x;
        const float dy = runtime.navigation_goal.y - runtime.instance.position.y;
        if (std::sqrt(dx * dx + dy * dy) > kNavigationArrivalRadius) continue;
        if (runtime.navigation_task == kNavigationPatrol && !runtime.patrol_points.empty()) {
            runtime.patrol_index = (runtime.patrol_index + 1) % runtime.patrol_points.size();
            const Vec3 next = runtime.patrol_points[runtime.patrol_index];
            SetNavigationGoal(runtime, next, kNavigationPatrol);
        } else if (runtime.navigation_task == kNavigationInvestigate) {
            runtime.has_navigation_goal = false;
            runtime.navigation_task = kNavigationNone;
            runtime.navigation_path.clear();
            runtime.navigation_cursor = 0;
            runtime.navigation_hold_until_frame = frame + kDecisionPeriodFrames * 5;
            runtime.instance.state = NPCState::Investigate;
            runtime.instance.state_timer_frames =
                static_cast<uint32_t>(kDecisionPeriodFrames * 5);
        } else {
            runtime.has_navigation_goal = false;
            runtime.navigation_task = kNavigationNone;
            runtime.navigation_path.clear();
            runtime.navigation_cursor = 0;
            runtime.navigation_hold_until_frame = 0;
        }
    }
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
    discovery_inspect_until_frame_ = 0;
    discovery_complete_ = false;
    return true;
}

bool AutonomousNpcSystem::ArmBodyDiscovery(uint64_t due_frame) {
    if (!cleaner_npc_.IsValid() || !discovery_body_.IsValid() ||
        !discovery_container_.IsValid() || FindRuntimeNpc(cleaner_npc_) == nullptr) {
        return false;
    }
    discovery_due_frame_ = due_frame;
    discovery_inspect_until_frame_ = 0;
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
    const bool sees_player = perception.sees_player;
    const MemoryKind kind = sees_player ? MemoryKind::Observation
                                        : MemoryKind::EventRecall;
    const KnowledgeSource source = sees_player ? KnowledgeSource::DirectWitness
                                               : KnowledgeSource::HeardSound;
    const char* semantic_tag = sees_player ? "player_observed"
                                           : "gunshot_heard_unknown";
    const float salience = sees_player ? 0.85f : 0.55f;
    const float confidence = sees_player ? perception.sight_confidence
                                        : std::clamp(perception.noise_loudness, 0.0f, 1.0f);
    const EntityId npc = EntityId::New(runtime.instance.id.GetValue());
    // Hearing a shot supplies a location/stimulus, not attribution.  Only a
    // sight observation may name the player as the observed subject.
    const EntityId subject = sees_player ? EntityId::New(1) : EntityId::Invalid();

    // Perception runs at 10 Hz, but a continuous sight/noise stimulus is one
    // semantic fact. Refresh the existing record for a bounded window instead
    // of manufacturing a new durable memory on every decision interval.
    const std::vector<MemoryRecord> prior = systemic_->MemoriesOf(npc);
    for (auto it = prior.rbegin(); it != prior.rend(); ++it) {
        const bool same_tag = std::find(it->tags.begin(), it->tags.end(), semantic_tag) !=
                              it->tags.end();
        if (it->kind == kind && it->source == source && it->subject == subject &&
            it->target == subject && it->room == runtime.room && same_tag &&
            frame >= it->frame && frame - it->frame <= kMemoryRefreshWindowFrames) {
            return systemic_->RefreshMemory(it->id, frame,
                                             std::max(it->confidence, confidence),
                                             std::max(it->salience, salience));
        }
    }

    MemoryRecord memory;
    memory.id = MemoryId::New(NextMemoryId(*systemic_));
    memory.npc = npc;
    memory.kind = kind;
    memory.subject = subject;
    memory.target = subject;
    memory.room = runtime.room;
    memory.frame = frame;
    memory.salience = salience;
    memory.confidence = confidence;
    memory.source = source;
    memory.text_key = sees_player ? ResourceId::New(0xB1001)
                                  : ResourceId::New(0xB1002);
    memory.tags.push_back(semantic_tag);
    return systemic_->AddMemory(memory);
}

bool AutonomousNpcSystem::CanSeePlayer(const RuntimeNpc& runtime) const {
    if (!player_target_active_ || world_query_ == nullptr || runtime.room != active_room_ ||
        runtime.instance.state == NPCState::Dead ||
        runtime.instance.state == NPCState::Stunned) return false;
    PerceptionSystem perception_system;
    PerceptionResult perception = perception_system.Update(
        runtime.instance, *world_query_, player_position_, player_eye_z_, {},
        std::numeric_limits<uint32_t>::max());
    if (!perception.sees_player) return false;
    return PassesPlayerVisibility(perception.sight_confidence,
                                  player_visibility_);
}

void AutonomousNpcSystem::RunDecision(uint64_t frame) {
    if (world_query_ == nullptr) return;
    PerceptionSystem perception_system;
    for (auto& runtime : npcs_) {
        if (runtime.room != active_room_ ||
            runtime.instance.state == NPCState::Dead ||
            runtime.instance.state == NPCState::Stunned) {
            // This field is the previous decision interval's visibility, not
            // a permanent historical fact. Leaving the room or losing
            // perception creates a fresh transition when visible again.
            runtime.player_observed = false;
            continue;
        }
        const bool body_task = runtime.instance.id == cleaner_npc_ &&
                               !discovery_complete_ && frame >= discovery_due_frame_;
        if (body_task) {
            const BodyRecord* body = systemic_ != nullptr
                ? systemic_->GetBody(discovery_body_) : nullptr;
            if (body != nullptr && body->disposition == BodyDisposition::HiddenInContainer) {
                runtime.instance.state = NPCState::Investigate;
                continue;
            }
        }
        // Reaching a noise location is an observable inspect action. Keep
        // that state for a short bounded hold instead of relabelling the NPC
        // Patrol on the very next quiet decision tick.  Perception still runs
        // during the hold: a genuinely new shot or sighting must refresh the
        // semantic memory and may supersede the inspection.
        const bool holding_investigate =
            runtime.instance.state == NPCState::Investigate &&
            frame < runtime.navigation_hold_until_frame &&
            !runtime.has_navigation_goal;
        if (frame >= runtime.navigation_hold_until_frame) {
            runtime.navigation_hold_until_frame = 0;
        }
        PerceptionResult perception = perception_system.Update(
            runtime.instance, *world_query_, player_position_, player_eye_z_, noises_,
            static_cast<uint32_t>(std::min<uint64_t>(
                frame, std::numeric_limits<uint32_t>::max())));
        if (!player_target_active_) {
            // A dead/restarting player is not a valid perception target.  The
            // guard combat loop and the durable observation path must not
            // turn a stale player pose into a new hostile sighting.
            perception.sees_player = false;
        }
        if (perception.sees_player) {
            if (!PassesPlayerVisibility(perception.sight_confidence,
                                        player_visibility_)) {
                perception.sees_player = false;
            }
        }
        const bool newly_sees_player = perception.sees_player && !runtime.player_observed;
        runtime.player_observed = perception.sees_player;
        const bool stimulus = perception.sees_player || perception.hears_noise;
        Receipt(runtime.instance.id, AutonomousPhase::Observe,
                runtime.instance.state, frame, stimulus);
        if (stimulus) {
            const bool remembered = AddObservationMemory(runtime, perception, frame);
            Receipt(runtime.instance.id, AutonomousPhase::Remember,
                    runtime.instance.state, frame, remembered);
        } else {
            Receipt(runtime.instance.id, AutonomousPhase::Remember,
                    runtime.instance.state, frame, true);
        }

        Receipt(runtime.instance.id, AutonomousPhase::Evaluate,
                runtime.instance.state, frame, true);
        const NPCState chosen = perception.sees_player &&
                                        runtime.instance.role == Role::Guard
                                    ? NPCState::Combat
                                : perception.sees_player ? NPCState::Alert :
                                (perception.hears_noise || holding_investigate)
                                    ? NPCState::Investigate
                                    : NPCState::Patrol;
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
        if (chosen == NPCState::Patrol && !runtime.patrol_points.empty() &&
            !runtime.has_navigation_goal) {
            SetNavigationGoal(runtime, runtime.patrol_points[runtime.patrol_index],
                              kNavigationPatrol);
        } else if (chosen == NPCState::Investigate && perception.hears_noise &&
                   (!runtime.has_navigation_goal ||
                    runtime.navigation_task != kNavigationInvestigate ||
                    std::fabs(runtime.navigation_goal.x - perception.noise_position.x) > 0.25f ||
                    std::fabs(runtime.navigation_goal.y - perception.noise_position.y) > 0.25f)) {
            SetNavigationGoal(runtime, perception.noise_position,
                              kNavigationInvestigate);
        } else if (chosen == NPCState::Combat && runtime.instance.role == Role::Guard) {
            const float dx = player_position_.x - runtime.instance.position.x;
            const float dy = player_position_.y - runtime.instance.position.y;
            if (std::sqrt(dx * dx + dy * dy) > kGuardCombatRange &&
                !runtime.has_navigation_goal) {
                SetNavigationGoal(runtime, player_position_, kNavigationCombat);
            }
        }
        if (runtime.instance.cognition == CognitionTier::Full &&
            newly_sees_player && frame >= runtime.next_speech_frame && events_ != nullptr) {
            events_->Post(EventNpcSpeak{runtime.instance.id, StringId::New(0xB1003)},
                          EventKind::Notification,
                          EntityId::New(runtime.instance.id.GetValue()),
                          EntityId::New(1), EventId::Invalid(), frame);
            runtime.next_speech_frame = frame + kDecisionPeriodFrames * 10;
        }
        Receipt(runtime.instance.id, AutonomousPhase::Act,
                runtime.instance.state, frame, true);
        if (stimulus) ++autonomous_loop_count_;
    }
}

void AutonomousNpcSystem::UpdateGuardCombat(uint64_t frame) {
    if (events_ == nullptr || world_query_ == nullptr) return;
    for (auto& runtime : npcs_) {
        if (runtime.room != active_room_ || runtime.instance.role != Role::Guard ||
            runtime.instance.state == NPCState::Dead ||
            runtime.instance.state == NPCState::Stunned ||
            (runtime.instance.state != NPCState::Alert &&
             runtime.instance.state != NPCState::Combat)) continue;
        if (!CanSeePlayer(runtime)) {
            runtime.has_navigation_goal = false;
            runtime.navigation_path.clear();
            runtime.navigation_cursor = 0;
            if (runtime.instance.state == NPCState::Combat) {
                runtime.instance.state = NPCState::Alert;
            }
            continue;
        }
        const float dx = player_position_.x - runtime.instance.position.x;
        const float dy = player_position_.y - runtime.instance.position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance > kGuardCombatRange) {
            runtime.instance.state = NPCState::Combat;
            // Replan only when the target has moved materially.  This keeps
            // the motor smooth while preventing a guard from finishing a
            // stale route to the player's previous position.
            const bool target_moved =
                !runtime.has_navigation_goal ||
                runtime.navigation_task != kNavigationCombat ||
                std::fabs(runtime.navigation_goal.x - player_position_.x) > 0.75f ||
                std::fabs(runtime.navigation_goal.y - player_position_.y) > 0.75f;
            if (target_moved) {
                SetNavigationGoal(runtime, player_position_, kNavigationCombat);
            }
            continue;
        }
        runtime.has_navigation_goal = false;
        runtime.navigation_path.clear();
        runtime.navigation_cursor = 0;
        if (frame < runtime.next_attack_frame) continue;
        events_->Post(EventPlayerDamage{8, 0,
                                        EntityId::New(runtime.instance.id.GetValue())},
                      EventKind::Mutation,
                      EntityId::New(runtime.instance.id.GetValue()),
                      EntityId::New(1), EventId::Invalid(), frame);
        runtime.next_attack_frame = frame + kGuardAttackPeriodFrames;
        ++guard_attack_count_;
    }
}

void AutonomousNpcSystem::Tick(uint64_t frame) {
    noises_.erase(std::remove_if(noises_.begin(), noises_.end(),
                                 [frame](const NoiseSource& noise) {
        return frame > noise.sim_frame && frame - noise.sim_frame > 120;
    }), noises_.end());

    // The motor is a 120 Hz path follower. Decisions/perception remain
    // bounded at 10 Hz, so visible movement is not a sequence of 0.1 m jumps.
    UpdateMotor(frame);
    TryBodyDiscovery(frame);
    if (frame % kDecisionPeriodFrames == 0) RunDecision(frame);
    UpdateGuardCombat(frame);
}

bool AutonomousNpcSystem::TryBodyDiscovery(uint64_t frame) {
    if (discovery_complete_ || systemic_ == nullptr || !cleaner_npc_.IsValid() ||
        frame < discovery_due_frame_) return false;
    RuntimeNpc* cleaner = FindRuntimeNpc(cleaner_npc_);
    if (cleaner == nullptr || cleaner->room != active_room_ ||
        cleaner->instance.state == NPCState::Stunned ||
        cleaner->instance.state == NPCState::Dead) {
        // Incapacitated actors cannot open a cart or become a DirectWitness.
        // The observation and response records must come from an executing
        // runtime actor, not merely from a configured due frame.
        return false;
    }
    const BodyRecord* body = systemic_->GetBody(discovery_body_);
    if (body == nullptr || body->disposition != BodyDisposition::HiddenInContainer) {
        return false;
    }
    const HideableContainer* container = systemic_->GetContainer(discovery_container_);
    if (container == nullptr || container->room != active_room_ ||
        body->room != active_room_ || container->accessibility == 0 ||
        std::find(container->routine_tags.begin(), container->routine_tags.end(),
                  RoutineTag::Cleaner) == container->routine_tags.end()) {
        return false;
    }

    const float dx = container->position.x - cleaner->instance.position.x;
    const float dy = container->position.y - cleaner->instance.position.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    constexpr float kArrivalRadius = 0.80f;
    if (distance > kArrivalRadius) {
        if (!cleaner->has_navigation_goal ||
            cleaner->navigation_task != kNavigationBodyDiscovery ||
            std::fabs(cleaner->navigation_goal.x - container->position.x) > 0.01f ||
            std::fabs(cleaner->navigation_goal.y - container->position.y) > 0.01f) {
            SetNavigationGoal(*cleaner, container->position,
                              kNavigationBodyDiscovery);
        }
        cleaner->instance.state = NPCState::Investigate;
        // The discovery callback may be entered between motor ticks (for
        // example immediately after a sparse replay event).  Advance only
        // one bounded motor step here; subsequent movement still follows the
        // normal 120 Hz path follower.
        (void)MoveAlongRoute(*cleaner, 1.0f / 120.0f);
        Receipt(cleaner_npc_, AutonomousPhase::Act,
                cleaner->instance.state, frame,
                !cleaner->route_failed);
        return false;
    }

    cleaner->has_navigation_goal = false;
    cleaner->navigation_task = kNavigationNone;
    cleaner->navigation_path.clear();
    cleaner->navigation_cursor = 0;

    // Arrival is not discovery. The cleaner holds position for a bounded
    // inspection interval before opening the container.
    if (discovery_inspect_until_frame_ == 0) {
        discovery_inspect_until_frame_ = frame + kDiscoveryInspectionFrames;
        cleaner->instance.state = NPCState::Investigate;
        cleaner->instance.state_timer_frames =
            static_cast<uint32_t>(kDiscoveryInspectionFrames);
        Receipt(cleaner_npc_, AutonomousPhase::Act,
                cleaner->instance.state, frame, true);
        return false;
    }
    if (frame < discovery_inspect_until_frame_) {
        cleaner->instance.state = NPCState::Investigate;
        cleaner->instance.state_timer_frames = static_cast<uint32_t>(
            discovery_inspect_until_frame_ - frame);
        Receipt(cleaner_npc_, AutonomousPhase::Act,
                cleaner->instance.state, frame, false);
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
    discovery_inspect_until_frame_ = 0;
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
    serializer.WriteU32(2);
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
    serializer.WriteU64(discovery_inspect_until_frame_);
}

bool AutonomousNpcSystem::Load(Deserializer& deserializer) {
    const uint32_t version = deserializer.ReadU32();
    const uint32_t count = deserializer.ReadU32();
    if (deserializer.HasError() || (version != 1 && version != 2) ||
        count > kMaxRuntimeNpcs) {
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
    const uint64_t inspect_until = version >= 2 ? deserializer.ReadU64() : 0;
    if (deserializer.HasError() || !deserializer.AtEnd()) {
        deserializer.MarkError();
        return false;
    }
    // Preflight every identity before applying any live runtime mutation.
    // A corrupt later record must not leave earlier NPCs half-restored.
    for (const auto& value : saved) {
        if (FindRuntimeNpc(value.id) == nullptr) {
            deserializer.MarkError();
            return false;
        }
    }
    for (const auto& value : saved) {
        RuntimeNpc* runtime = FindRuntimeNpc(value.id);
        runtime->room = value.room;
        runtime->instance.position = value.position;
        runtime->instance.yaw = value.yaw;
        runtime->instance.health = value.health;
        runtime->instance.alertness = value.alertness;
        runtime->instance.state = static_cast<NPCState>(value.state);
        runtime->instance.state_timer_frames = value.timer;
        runtime->instance.plan_step = value.plan;
        runtime->player_observed = value.observed;
        runtime->navigation_path.clear();
        runtime->navigation_cursor = 0;
        runtime->has_navigation_goal = false;
        runtime->navigation_task = kNavigationNone;
        runtime->navigation_hold_until_frame = 0;
        runtime->route_failed = false;
        runtime->next_attack_frame = 0;
    }
    // Motor cadence is runtime-only.  A loaded checkpoint starts with a
    // fresh cadence sample instead of integrating from an unrelated frame
    // in the previous session.
    last_motor_frame_ = 0;
    cleaner_npc_ = cleaner;
    discovery_body_ = body;
    discovery_container_ = container;
    discovery_due_frame_ = due;
    discovery_inspect_until_frame_ = inspect_until;
    discovery_complete_ = complete;
    return true;
}

} // namespace writeover
