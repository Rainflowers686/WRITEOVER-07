#pragma once
// Small, production-wired autonomous NPC loop. This is deliberately a
// semantic adapter, not a second world or a general-purpose AI framework:
// perception reads geometry, systemic owns durable memory/relationships, and
// EventBus carries the resulting facts to the other modules.

#include "writeover/ai/npc.h"
#include "writeover/ai/perception.h"
#include "writeover/player/combat.h"
#include "writeover/systemic/systemic.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace writeover {

enum class AutonomousPhase : uint8_t {
    Observe = 0,
    Remember = 1,
    Evaluate = 2,
    Choose = 3,
    Act = 4,
};

struct AutonomousReceipt {
    NpcId npc;
    AutonomousPhase phase = AutonomousPhase::Observe;
    NPCState state = NPCState::Idle;
    uint64_t frame = 0;
    bool success = false;
};

struct RuntimeNpc {
    NPCInstance instance;
    RoomId room;
    bool player_observed = false;
    // Runtime-only motor state.  Authored routes are bounded inputs; the
    // deterministic path is rebuilt from the current position after load.
    std::vector<Vec3> navigation_path;
    size_t navigation_cursor = 0;
    std::vector<Vec3> patrol_points;
    size_t patrol_index = 0;
    Vec3 navigation_goal;
    bool has_navigation_goal = false;
    uint8_t navigation_task = 0; // 1 patrol, 2 investigate, 3 body, 4 combat
    uint64_t navigation_hold_until_frame = 0;
    uint64_t next_attack_frame = 0;
    uint64_t next_speech_frame = 0;
    bool route_failed = false;
};

struct ShotFeedback {
    HitscanResult hit;
    NpcId npc;
    bool target_was_npc = false;
    bool target_died = false;
    bool target_stunned = false;
};

class AutonomousNpcSystem {
public:
    using ShotFeedbackCallback = std::function<void(const ShotFeedback&)>;

    void Attach(SystemicWorld* systemic, EventBus* events,
                DeterministicRNG* sim_rng);
    void SetWorldQuery(const IWorldQuery* query) { world_query_ = query; }
    void SetActiveRoom(RoomId room) { active_room_ = room; }
    void SetPlayerPose(const Vec3& position, float eye_z) {
        player_position_ = position;
        player_eye_z_ = eye_z;
    }
    void SetPlayerTargetActive(bool active) { player_target_active_ = active; }
    void SetPlayerVisibility(float visibility) {
        player_visibility_ = std::clamp(visibility, 0.0f, 1.0f);
    }
    bool SetPatrolRoute(NpcId npc, const std::vector<Vec3>& points);

    bool AddNpc(const NPCInstance& npc, RoomId room);
    bool ConfigureBodyDiscovery(NpcId cleaner, EntityId body,
                                ContainerId container, uint64_t due_frame);
    // Arm the configured discovery chain from a scene-relative frame.  The
    // caller invokes this when the body is actually hidden.
    bool ArmBodyDiscovery(uint64_t due_frame);
    void SetShotFeedbackCallback(ShotFeedbackCallback callback) {
        shot_feedback_callback_ = std::move(callback);
    }

    // One deterministic Observe -> Remember -> Evaluate -> Choose -> Act
    // pass runs at a bounded 10 Hz cadence on the fixed simulation clock.
    void Tick(uint64_t frame);
    ShotFeedback HandlePlayerShot(const FireRequest& request,
                                  const WeaponDef& weapon, uint64_t frame);

    const std::vector<RuntimeNpc>& Npcs() const { return npcs_; }
    const std::vector<AutonomousReceipt>& Receipts() const { return receipts_; }
    size_t AutonomousLoopCount() const { return autonomous_loop_count_; }
    size_t DiscoveryResponseCount() const { return discovery_response_count_; }
    size_t GuardAttackCount() const { return guard_attack_count_; }

    void Save(Serializer& serializer) const;
    bool Load(Deserializer& deserializer);

private:
    void OnWorldEvent(const WorldEvent& event);
    void Receipt(NpcId npc, AutonomousPhase phase, NPCState state,
                 uint64_t frame, bool success);
    bool AddObservationMemory(const RuntimeNpc& runtime,
                              const PerceptionResult& perception,
                              uint64_t frame);
    bool TryBodyDiscovery(uint64_t frame);
    void UpdateMotor(uint64_t frame);
    void RunDecision(uint64_t frame);
    void UpdateGuardCombat(uint64_t frame);
    bool PlanRoute(RuntimeNpc& runtime, const Vec3& target);
    bool MoveAlongRoute(RuntimeNpc& runtime, float delta_seconds);
    bool CanSeePlayer(const RuntimeNpc& runtime) const;
    void SetNavigationGoal(RuntimeNpc& runtime, const Vec3& target,
                           uint8_t task);
    NpcId FindNpcByEntity(EntityId entity) const;
    RuntimeNpc* FindRuntimeNpc(NpcId id);
    const RuntimeNpc* FindRuntimeNpc(NpcId id) const;

    SystemicWorld* systemic_ = nullptr;
    EventBus* events_ = nullptr;
    DeterministicRNG* sim_rng_ = nullptr;
    const IWorldQuery* world_query_ = nullptr;
    EventBus::ConsumerId event_consumer_id_ = 0;
    RoomId active_room_;
    Vec3 player_position_;
    float player_eye_z_ = kEyeStand;
    float player_visibility_ = 1.0f;
    bool player_target_active_ = true;
    std::vector<NoiseSource> noises_;
    std::vector<RuntimeNpc> npcs_;
    std::vector<AutonomousReceipt> receipts_;
    size_t autonomous_loop_count_ = 0;
    size_t discovery_response_count_ = 0;
    size_t guard_attack_count_ = 0;
    uint64_t last_motor_frame_ = 0;

    NpcId cleaner_npc_;
    EntityId discovery_body_;
    ContainerId discovery_container_;
    uint64_t discovery_due_frame_ = 0;
    uint64_t discovery_inspect_until_frame_ = 0;
    bool discovery_complete_ = false;
    ShotFeedbackCallback shot_feedback_callback_;
};

} // namespace writeover
