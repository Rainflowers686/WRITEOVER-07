// Composition Root (M1 owns this file).
// Core never links the semantic modules: this executable assembles
// world/player/ai/narrative/render/platform and registers them into the
// Engine (M-002 closure). Everything here is private app wiring.

#include "writeover/core/console.h"
#include "writeover/core/engine.h"
#include "writeover/core/profile.h"
#include "writeover/core/save.h"
#include "writeover/core/settings.h"
#include "writeover/audio/audio_backend.h"
#include "writeover/narrative/causality.h"
#include "writeover/narrative/dialog.h"
#include "writeover/narrative/judge.h"
#include "writeover/narrative/narrator.h"
#include "writeover/narrative/storylet.h"
#include "writeover/player/combat.h"
#include "writeover/ai/runtime.h"
#include "writeover/player/controller.h"
#include "writeover/player/input.h"
#include "writeover/player/input_runtime.h"
#include "writeover/player/weapon.h"
#include "writeover/systemic/systemic.h"
#include "writeover/common/math.h"
#include "writeover/render/character_renderer.h"
#include "writeover/render/hud.h"
#include "writeover/render/raycaster.h"
#include "writeover/render/reference_renderer.h"
#include "writeover/render/terminal_backend.h"
#include "writeover/world/fact_belief.h"
#include "writeover/world/grid.h"
#include "writeover/world/infrastructure.h"
#include "writeover/world/room.h"

#include "src/app/composition_root.h"
#include "src/app/interaction_runtime.h"
#include "src/app/scene_runtime.h"
#include "src/player/dynamic_collision.h"
#include "src/app/runtime_paths.h"
#include "writeover/platform/platform_api.h"

#include <cstdint>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace writeover {

namespace {

class PlayerActorWorldQuery final : public IWorldQuery {
public:
    PlayerActorWorldQuery(const IWorldQuery* static_query,
                          const std::vector<RuntimeNpc>* npcs,
                          RoomId active_room)
        : static_query_(static_query), npcs_(npcs), active_room_(active_room) {}

    void SetStaticQuery(const IWorldQuery* query) { static_query_ = query; }
    void SetActiveRoom(RoomId room) { active_room_ = room; }

    bool IsSolidAt(int32_t col, int32_t row) const override {
        return static_query_ != nullptr && static_query_->IsSolidAt(col, row);
    }
    GridCell GetCell(int32_t col, int32_t row) const override {
        return static_query_ != nullptr ? static_query_->GetCell(col, row)
                                        : GridCell{};
    }
    int32_t Width() const override {
        return static_query_ != nullptr ? static_query_->Width() : 0;
    }
    int32_t Height() const override {
        return static_query_ != nullptr ? static_query_->Height() : 0;
    }
    float FloorHeightAt(float x, float y) const override {
        return static_query_ != nullptr ? static_query_->FloorHeightAt(x, y) : 0.0f;
    }
    float CeilingHeightAt(float x, float y) const override {
        return static_query_ != nullptr ? static_query_->CeilingHeightAt(x, y) : 0.0f;
    }
    bool LineOfSight(const Vec3& a, const Vec3& b, float eye_z) const override {
        return static_query_ != nullptr && static_query_->LineOfSight(a, b, eye_z);
    }

    bool AabbBlocked(const AABB& box) const override {
        if (static_query_ == nullptr || static_query_->AabbBlocked(box)) {
            return true;
        }
        if (npcs_ == nullptr || !active_room_.IsValid()) return false;
        constexpr float kStandingNpcRadius = 0.42f;
        constexpr float kStandingNpcHeight = 1.80f;
        for (const auto& runtime : *npcs_) {
            if (runtime.room != active_room_ ||
                runtime.instance.state == NPCState::Stunned ||
                runtime.instance.state == NPCState::Dead) {
                continue;
            }
            if (DynamicActorOverlaps(box, runtime.instance.position,
                                     kStandingNpcRadius, kStandingNpcHeight)) {
                return true;
            }
        }
        return false;
    }

private:
    const IWorldQuery* static_query_ = nullptr;
    const std::vector<RuntimeNpc>* npcs_ = nullptr;
    RoomId active_room_;
};

const char* QualityPresetName(QualityPreset preset) {
    switch (preset) {
    case QualityPreset::Ultra120: return "ULTRA120";
    case QualityPreset::HighRefresh: return "HIGH_REFRESH";
    case QualityPreset::Presentation60: return "PRESENTATION60";
    case QualityPreset::Compatibility: return "COMPATIBILITY";
    }
    return "UNKNOWN";
}

WeaponSlot ValidWeaponSlot(const CombatState* combat) {
    if (combat == nullptr ||
        static_cast<size_t>(combat->slot) >= kWeaponSlotCount) {
        return WeaponSlot::Pistol;
    }
    return combat->slot;
}

const char* WeaponSlotName(WeaponSlot slot) {
    switch (slot) {
    case WeaponSlot::Pistol: return "PISTOL";
    case WeaponSlot::Smg: return "SMG";
    case WeaponSlot::Stunner: return "STUNNER";
    case WeaponSlot::Count: break;
    }
    return "UNKNOWN";
}

uint64_t StableContentId(std::string_view value) {
    uint64_t hash = 0xCBF29CE484222325ull;
    for (const unsigned char byte : value) {
        hash ^= byte;
        hash *= 0x100000001B3ull;
    }
    return hash;
}

FactId RuntimeFactId(std::string_view value) {
    return FactId::New(StableContentId(value));
}

// The scheduler keeps its fixed 120 Hz cadence, while gameplay time is
// frozen by the player pause gate.  This private composition seam avoids
// changing the frozen public EngineContext/SimClock contract.
class RuntimeTimeGate {
public:
    void ObserveSchedulerFrame(uint64_t scheduler_frame) {
        if (!observed_) {
            observed_ = true;
            last_scheduler_frame_ = scheduler_frame;
            return;
        }
        if (scheduler_frame <= last_scheduler_frame_) return;
        if (!paused_) game_frame_ += scheduler_frame - last_scheduler_frame_;
        last_scheduler_frame_ = scheduler_frame;
    }

    void SetPaused(bool paused) { paused_ = paused; }
    bool Paused() const { return paused_; }
    uint64_t GameFrame() const { return game_frame_; }

private:
    uint64_t last_scheduler_frame_ = 0;
    uint64_t game_frame_ = 0;
    bool observed_ = false;
    bool paused_ = false;
};

} // namespace

class AiModule final : public IEngineModule {
public:
    void Init(const EngineContext& ctx) override {
        ctx_ = ctx;
        runtime_.Attach(systemic_, ctx.events, ctx.sim_rng);
    }
    void Shutdown() override {}
    void SimTick(const SimClock& clock) override {
        if (pause_source_ && pause_source_()) return;
        const uint64_t frame = game_frame_source_
            ? game_frame_source_() : clock.FrameCount();
        if (player_position_source_) runtime_.SetPlayerPose(
            player_position_source_(), player_eye_source_ ? player_eye_source_() : kEyeStand);
        if (player_visibility_source_) {
            runtime_.SetPlayerVisibility(player_visibility_source_());
        }
        if (player_target_active_source_) {
            runtime_.SetPlayerTargetActive(player_target_active_source_());
        }
        runtime_.Tick(frame);
    }
    void AttachSystemic(SystemicWorld* systemic) {
        systemic_ = systemic;
        if (ctx_.events != nullptr) runtime_.Attach(systemic_, ctx_.events, ctx_.sim_rng);
    }
    void SetWorldQuery(const IWorldQuery* query) { runtime_.SetWorldQuery(query); }
    void SetActiveRoom(RoomId room) { runtime_.SetActiveRoom(room); }
    void SetPlayerPositionSource(std::function<Vec3()> source) {
        player_position_source_ = std::move(source);
    }
    void SetPlayerEyeSource(std::function<float()> source) {
        player_eye_source_ = std::move(source);
    }
    void SetPlayerVisibilitySource(std::function<float()> source) {
        player_visibility_source_ = std::move(source);
    }
    void SetPlayerTargetActiveSource(std::function<bool()> source) {
        player_target_active_source_ = std::move(source);
    }
    void SetPauseSource(std::function<bool()> source) {
        pause_source_ = std::move(source);
    }
    void SetGameFrameSource(std::function<uint64_t()> source) {
        game_frame_source_ = std::move(source);
    }
    bool AddNpc(const NPCInstance& npc, RoomId room) { return runtime_.AddNpc(npc, room); }
    bool SetPatrolRoute(NpcId npc, const std::vector<Vec3>& points) {
        return runtime_.SetPatrolRoute(npc, points);
    }
    bool ConfigureBodyDiscovery(NpcId cleaner, EntityId body, ContainerId container,
                                uint64_t due_frame) {
        return runtime_.ConfigureBodyDiscovery(cleaner, body, container, due_frame);
    }
    bool ArmBodyDiscovery(uint64_t due_frame) {
        return runtime_.ArmBodyDiscovery(due_frame);
    }
    void SetShotFeedbackCallback(AutonomousNpcSystem::ShotFeedbackCallback callback) {
        runtime_.SetShotFeedbackCallback(std::move(callback));
    }
    ShotFeedback HandlePlayerShot(const FireRequest& request, const WeaponDef& weapon,
                                  uint64_t frame) {
        return runtime_.HandlePlayerShot(request, weapon, frame);
    }
    const std::vector<RuntimeNpc>& Npcs() const { return runtime_.Npcs(); }
    size_t AutonomousLoopCount() const { return runtime_.AutonomousLoopCount(); }
    size_t DiscoveryResponseCount() const { return runtime_.DiscoveryResponseCount(); }
    size_t GuardAttackCount() const { return runtime_.GuardAttackCount(); }
    void Save(Serializer& s) const { runtime_.Save(s); }
    bool Load(Deserializer& d) { return runtime_.Load(d); }
    const char* Name() const override { return "ai"; }

private:
    EngineContext ctx_{};
    SystemicWorld* systemic_ = nullptr;
    AutonomousNpcSystem runtime_;
    std::function<Vec3()> player_position_source_;
    std::function<float()> player_eye_source_;
    std::function<float()> player_visibility_source_;
    std::function<bool()> player_target_active_source_;
    std::function<bool()> pause_source_;
    std::function<uint64_t()> game_frame_source_;
};

class WorldModule final : public IEngineModule {
public:
    void SetRoomOverride(const std::string& id) { room_override_ = id; }
    bool LoadRoomById(const std::string& id) {
        if (ctx_.data_dir.empty()) return false;
        auto room = LoadRoomFile(ctx_.data_dir + "/rooms/" + id + ".woc");
        if (room.IsError()) return false;
        loaded_room_ = room.Value();
        query_ = std::make_unique<GridWorldQuery>(&loaded_room_.grid);
        room_override_ = id;
        loaded_room_key_ = id;
        EnsureB1Gate();
        return true;
    }
    void Init(const EngineContext& ctx) override {
        ctx_ = ctx;
        if (!ctx_.data_dir.empty()) {
            const std::string primary = !room_override_.empty()
                ? ctx_.data_dir + "/rooms/" + room_override_ + ".woc"
                : ctx_.data_dir + "/rooms/room_b1_revival.woc";
            auto room = LoadRoomFile(primary);
            if (room.IsError()) {
                const std::string fallback = ctx_.data_dir + "/rooms/room_01_calibration.woc";
                room = LoadRoomFile(fallback);
                if (room.IsOk()) loaded_room_key_ = "room_01_calibration";
            }
            if (room.IsOk()) {
                loaded_room_ = room.Value();
                query_ = std::make_unique<GridWorldQuery>(&loaded_room_.grid);
                if (loaded_room_key_.empty()) {
                    loaded_room_key_ = !room_override_.empty()
                        ? room_override_ : "room_b1_revival";
                }
            }
        }
        if (query_ == nullptr) {
            synthetic_grid_ = Grid(16, 12);
            query_ = std::make_unique<GridWorldQuery>(&synthetic_grid_);
        }
        EnsureB1Gate();
    }

    void Shutdown() override {}

    const IWorldQuery& Query() const { return *query_; }
    FactStore& Facts() { return facts_; }
    InfrastructureSystem& Infra() { return infra_; }
    bool LoadAuthoredFacts(const std::string& path) {
        const auto bytes = ReadFileBinary(path);
        if (bytes.IsError()) return false;
        Deserializer d(bytes.Value().data(), bytes.Value().size());
        if (d.ReadU32() != kWocMagic || d.ReadU32() != kWocVersion) {
            return false;
        }
        const uint32_t count = d.ReadU32();
        if (d.HasError() || count > 4096) return false;
        std::set<FactId> seen;
        FactStore restored;
        for (uint32_t i = 0; i < count; ++i) {
            const std::string id = d.ReadString();
            const uint8_t predicate = d.ReadU8();
            const FactId fact_id = RuntimeFactId(id);
            if (d.HasError() || id.empty() || id.size() > 128 ||
                !fact_id.IsValid() || !seen.insert(fact_id).second ||
                predicate >= static_cast<uint8_t>(PredicateType::Count)) {
                return false;
            }
            // The current facts compiler emits the initial registry as
            // validation data; all recovery facts are boolean and begin
            // false until an owning world action changes them.
            if (predicate == static_cast<uint8_t>(PredicateType::State)) {
                restored.Set(WorldFact{fact_id, EntityId::Invalid(),
                                       PredicateType::State, false});
            } else if (predicate == static_cast<uint8_t>(PredicateType::Count)) {
                restored.Set(WorldFact{fact_id, EntityId::Invalid(),
                                       PredicateType::Count, int32_t{0}});
            } else {
                restored.Set(WorldFact{fact_id, EntityId::Invalid(),
                                       PredicateType::Relation,
                                       EntityId::Invalid()});
            }
        }
        if (d.HasError() || !d.AtEnd()) return false;
        facts_ = std::move(restored);
        return true;
    }
    void SetBooleanFact(FactId id, EntityId subject, bool value) {
        facts_.Set(WorldFact{id, subject, PredicateType::State, value});
    }
    void SaveState(Serializer& serializer) const {
        serializer.WriteU32(1);
        infra_.Save(serializer);
        facts_.Save(serializer);
    }
    bool LoadState(Deserializer& deserializer) {
        const uint32_t version = deserializer.ReadU32();
        if (deserializer.HasError() || version != 1) {
            deserializer.MarkError();
            return false;
        }
        InfrastructureSystem restored_infra;
        FactStore restored_facts;
        if (!restored_infra.Load(deserializer) || !restored_facts.Load(deserializer) ||
            deserializer.HasError() || !deserializer.AtEnd()) {
            deserializer.MarkError();
            return false;
        }
        infra_ = std::move(restored_infra);
        facts_ = std::move(restored_facts);
        EnsureB1Gate();
        return true;
    }
    Room& LoadedRoom() { return loaded_room_; }
    bool HasLoadedRoom() const {
        return query_ != nullptr && loaded_room_.grid.Width() > 0 &&
               loaded_room_.grid.Height() > 0;
    }

    void PushCommand(WorldCommand cmd) { commands_.push_back(std::move(cmd)); }

    void SetB1GateId(DoorId id) {
        if (!id.IsValid()) return;
        b1_gate_id_ = id;
        EnsureB1Gate();
    }
    DoorId B1GateId() const { return b1_gate_id_; }
    bool IsB1Loaded() const { return loaded_room_key_ == "room_b1_revival"; }
    bool B1GateOpen() const {
        const DoorState* door = infra_.GetDoor(B1GateId());
        return door != nullptr && door->open && !door->locked;
    }
    bool UnlockB1Gate() {
        if (!IsB1Loaded() || infra_.GetDoor(B1GateId()) == nullptr) return false;
        return infra_.SetDoorLocked(B1GateId(), false);
    }
    bool OpenB1Gate() {
        if (!IsB1Loaded() || !infra_.SetDoorOpen(B1GateId(), true)) return false;
        ApplyB1GateGeometry();
        return true;
    }
    void SetB1GateCell(int32_t col, int32_t row) {
        gate_col_ = col;
        gate_row_ = row;
        ApplyB1GateGeometry();
    }

    void SimTick(const SimClock& clock) override {
        if (pause_source_ && pause_source_()) return;
        const uint64_t frame = game_frame_source_
                                   ? game_frame_source_()
                                   : clock.FrameCount();
        for (auto& cmd : commands_) {
            ApplyCommand(cmd, frame);
        }
        commands_.clear();
    }

    void SetPauseSource(std::function<bool()> source) {
        pause_source_ = std::move(source);
    }
    void SetGameFrameSource(std::function<uint64_t()> source) {
        game_frame_source_ = std::move(source);
    }

    const char* Name() const override { return "world"; }

private:
    void EnsureB1Gate() {
        if (!IsB1Loaded() || !HasLoadedRoom()) return;
        if (infra_.GetDoor(B1GateId()) == nullptr) {
            infra_.AddDoor(DoorState{B1GateId(), false, true});
        }
        ApplyB1GateGeometry();
    }

    void ApplyB1GateGeometry() {
        if (!IsB1Loaded() || !HasLoadedRoom()) return;
        const bool open = B1GateOpen();
        const int32_t kGateCol = std::clamp(gate_col_, 1,
                                            loaded_room_.grid.Width() - 2);
        const int32_t kGateRow = std::clamp(gate_row_, 1,
                                            loaded_room_.grid.Height() - 2);
        for (int32_t row = 1; row + 1 < loaded_room_.grid.Height(); ++row) {
            GridCell cell = loaded_room_.grid.GetCell(kGateCol, row);
            cell.flags = static_cast<uint8_t>(cell.flags | CellFlag_Door);
            if (row == kGateRow && open) {
                cell.flags = static_cast<uint8_t>(cell.flags & ~CellFlag_Solid);
            } else {
                cell.flags = static_cast<uint8_t>(cell.flags | CellFlag_Solid);
            }
            loaded_room_.grid.SetCell(kGateCol, row, cell);
        }
    }

    void ApplyCommand(const WorldCommand& cmd, uint64_t frame) {
        if (std::holds_alternative<CommandSetDoor>(cmd)) {
            const auto& c = std::get<CommandSetDoor>(cmd);
            if (infra_.SetDoorOpen(c.door, c.open)) {
                ctx_.events->Post(EventDoorChange{c.door, c.open},
                                  EventKind::Mutation,
                                  EntityId::Invalid(), EntityId::Invalid(),
                                  EventId::Invalid(), frame);
            }
        } else if (std::holds_alternative<CommandSetPower>(cmd)) {
            const auto& c = std::get<CommandSetPower>(cmd);
            if (infra_.SetPowered(c.system, c.powered)) {
                ctx_.events->Post(EventPowerToggle{c.system, c.powered},
                                  EventKind::Mutation,
                                  EntityId::Invalid(), EntityId::Invalid(),
                                  EventId::Invalid(), frame);
            }
        }
        // Character-space commands (move/jump/posture/lean/fire) are consumed
        // by the player module; world applies world-space commands only.
    }

    EngineContext ctx_{};
    std::string room_override_;
    std::string loaded_room_key_;
    Room loaded_room_;
    Grid synthetic_grid_{16, 12};
    std::unique_ptr<GridWorldQuery> query_;
    FactStore facts_;
    InfrastructureSystem infra_;
    std::vector<WorldCommand> commands_;
    DoorId b1_gate_id_ = DoorId::New(9101);
    int32_t gate_col_ = 21;
    int32_t gate_row_ = 8;
    std::function<bool()> pause_source_;
    std::function<uint64_t()> game_frame_source_;
};

class PlayerModule final : public IEngineModule {
public:
    void Init(const EngineContext& ctx) override {
        ctx_ = ctx;
        // Initialize mapper from Settings context-aware bindings.
        mapper_ = InputMapper();
        if (ctx.settings != nullptr) {
            ApplySettingsBindings(*ctx.settings);
        }
    }
    void Shutdown() override {}

    InputState& Input() { return input_; }
    LocomotionState& Locomotion() { return locomotion_; }
    CombatState& Combat() { return combat_; }
    InputMapper& Mapper() { return mapper_; }
    bool Paused() const { return paused_; }
    const uint16_t& Health() const { return health_; }
    bool Dead() const { return dead_; }
    bool ApplyDamage(uint16_t amount) {
        if (dead_ || amount == 0) return false;
        if (amount >= health_) {
            health_ = 0;
            dead_ = true;
        } else {
            health_ = static_cast<uint16_t>(health_ - amount);
        }
        return true;
    }

    void SetHealthState(uint16_t health, bool dead) {
        health_ = health;
        dead_ = dead || health == 0;
    }
    uint64_t CurrentFrame() const { return current_frame_; }

    void ApplySettingsBindings(const Settings& st) {
        for (size_t c = 0; c < kInputContextCount; ++c) {
            for (size_t a = 0; a < kGameActionCount; ++a) {
                mapper_.SetBinding(static_cast<InputContext>(c),
                                   static_cast<GameAction>(a),
                                   st.key_bindings[c][a]);
            }
        }
    }

    void SimTick(const SimClock& clock) override {
        if (time_gate_ != nullptr) {
            time_gate_->ObserveSchedulerFrame(clock.FrameCount());
        }
        const uint64_t frame = time_gate_ != nullptr
                                   ? time_gate_->GameFrame()
                                   : clock.FrameCount();
        current_frame_ = frame;

        if (input_.action_pressed[static_cast<size_t>(GameAction::Pause)]) {
            paused_ = !paused_;
            if (time_gate_ != nullptr) time_gate_->SetPaused(paused_);
            if (pause_callback_) pause_callback_();
        }
        if (paused_) return;
        if (dead_) {
            // A dead player has no movement or interaction authority.  Load is
            // the only gameplay action accepted until the checkpoint restores
            // a live authoritative state.
            if (input_.action_pressed[static_cast<size_t>(GameAction::LoadGame)] &&
                load_callback_) {
                load_callback_();
            }
            return;
        }

        bool dragging = false;
        float drag_modifier = 1.0f;
        bool sprint_forbidden = false;
        bool weapon_restricted = false;
        if (drag_state_callback_) {
            dragging = drag_state_callback_(drag_modifier, sprint_forbidden,
                                            weapon_restricted);
            drag_modifier = std::clamp(drag_modifier, 0.0f, 1.0f);
        }

        // Mouse look: apply accumulated mouse delta before movement
        // (camera-relative movement depends on the current yaw).
        ApplyMouseLook(locomotion_, input_.mouse_delta,
                       ctx_.settings != nullptr
                           ? ctx_.settings->mouse_sensitivity
                           : 50);

        // Camera-relative movement: WASD in local space -> world axes.
        Vec2 local{0.0f, 0.0f};
        if (input_.action_down[static_cast<size_t>(GameAction::MoveForward)]) {
            local.y += 1.0f;
        }
        if (input_.action_down[static_cast<size_t>(GameAction::MoveBackward)]) {
            local.y -= 1.0f;
        }
        if (input_.action_down[static_cast<size_t>(GameAction::MoveLeft)]) {
            local.x -= 1.0f;
        }
        if (input_.action_down[static_cast<size_t>(GameAction::MoveRight)]) {
            local.x += 1.0f;
        }
        Vec2 move = CameraRelativeWish(local, locomotion_.yaw);
        if (dragging) {
            move.x *= drag_modifier;
            move.y *= drag_modifier;
        }
        const bool sprint =
            !sprint_forbidden &&
            input_.action_down[static_cast<size_t>(GameAction::Sprint)];
        IntegrateLocomotion(locomotion_, move, sprint, *world_query_,
                            SimClock::kFixedDeltaTime);
        // A malformed runtime position must not leave the player falling or
        // propagating non-finite coordinates forever.  The owning world
        // supplies the current room's authored recovery point; this is a
        // bounded safety seam, not a second movement model.
        if (recovery_spawn_source_ &&
            ((!std::isfinite(locomotion_.position.x)) ||
             (!std::isfinite(locomotion_.position.y)) ||
             (!std::isfinite(locomotion_.position.z)) ||
             locomotion_.position.z < -8.0f)) {
            const Vec3 recovery = recovery_spawn_source_();
            if (std::isfinite(recovery.x) && std::isfinite(recovery.y) &&
                std::isfinite(recovery.z)) {
                locomotion_.position = recovery;
                locomotion_.velocity = Vec3{};
                locomotion_.contact.grounded = true;
            }
        }

        if (input_.action_down[static_cast<size_t>(GameAction::LeanLeft)]) {
            SetLean(locomotion_, -1);
        } else if (input_.action_down[static_cast<size_t>(GameAction::LeanRight)]) {
            SetLean(locomotion_, 1);
        } else {
            SetLean(locomotion_, 0);
        }

        if (input_.action_pressed[static_cast<size_t>(GameAction::Jump)]) {
            TryJump(locomotion_);
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::Crouch)]) {
            TrySetPosture(locomotion_,
                          locomotion_.posture == Posture::Crouch
                              ? Posture::Stand
                              : Posture::Crouch,
                          *world_query_);
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::Prone)]) {
            TrySetPosture(locomotion_,
                          locomotion_.posture == Posture::Prone
                              ? Posture::Crouch
                              : Posture::Prone,
                          *world_query_);
        }

        // ADS is intentionally disabled for this bounded recovery slice. The
        // input remains reserved for a future authored feature, but there is
        // no player-facing half-feature or invisible aiming state.
        combat_.aiming = false;
        const WeaponSlot slot_before_switch = combat_.slot;
        if (!dragging && !weapon_restricted) {
            if (input_.action_pressed[static_cast<size_t>(GameAction::WeaponSlot1)]) {
                combat_.slot = WeaponSlot::Pistol;
            } else if (input_.action_pressed[static_cast<size_t>(GameAction::WeaponSlot2)]) {
                combat_.slot = WeaponSlot::Smg;
            } else if (input_.action_pressed[static_cast<size_t>(GameAction::WeaponSlot3)]) {
                combat_.slot = WeaponSlot::Stunner;
            }
        }
        // Reload is bound to the weapon that started it.  A slot switch
        // cancels that transaction instead of allowing AdvanceReload() to
        // complete against the newly selected magazine.
        if (combat_.slot != slot_before_switch && combat_.reload_frames_left > 0) {
            combat_.reload_frames_left = 0;
        }

        const bool smg_held_fire = combat_.slot == WeaponSlot::Smg &&
                                   input_.action_down[static_cast<size_t>(GameAction::Fire)];
        if (!weapon_restricted &&
            (input_.action_pressed[static_cast<size_t>(GameAction::Fire)] ||
             smg_held_fire)) {
            const size_t slot = static_cast<size_t>(combat_.slot);
            const WeaponDef& weapon = DefaultWeapons()[slot];
            FireRequest fire_request;
            fire_request.origin = locomotion_.EyePosition();
            fire_request.yaw = locomotion_.yaw;
            fire_request.pitch = locomotion_.pitch;
            fire_request.slot = combat_.slot;
            fire_request.spread_factor = combat_.spread_factor;
            if (ConsumeShot(combat_, weapon, static_cast<uint32_t>(frame))) {
                ctx_.events->Post(
                    EventWeaponFire{EntityId::New(1), combat_.slot,
                                    locomotion_.EyePosition(),
                                    locomotion_.yaw, locomotion_.pitch,
                                    weapon.loudness},
                    EventKind::Notification, EntityId::New(1),
                    EntityId::Invalid(), EventId::Invalid(), frame);
                if (fire_callback_) fire_callback_(fire_request, weapon);
            }
        }
        if (!weapon_restricted &&
            input_.action_pressed[static_cast<size_t>(GameAction::Reload)]) {
            const size_t slot = static_cast<size_t>(combat_.slot);
            StartReload(combat_, combat_.slot, DefaultWeapons()[slot]);
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::DevPanel)] &&
            debug_toggle_callback_) {
            debug_toggle_callback_();
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::Help)] && narrator_intrusion_callback_) {
            narrator_intrusion_callback_();
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::SaveGame)] && save_callback_) {
            save_callback_();
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::LoadGame)] && load_callback_) {
            load_callback_();
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::Interact)] && interact_callback_) {
            interact_callback_();
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::Melee)] &&
            !weapon_restricted && melee_callback_) {
            melee_callback_();
        }
        if (dragging && drag_update_callback_) {
            drag_update_callback_(locomotion_.position, current_room_, frame);
        }
        AdvanceReload(combat_, 1);
        if (combat_.spread_factor > 0.0f) {
            combat_.spread_factor -= 0.01f;
            if (combat_.spread_factor < 0.0f) {
                combat_.spread_factor = 0.0f;
            }
        }
    }

    void SetWorldQuery(const IWorldQuery* query) { world_query_ = query; }
    void SetRecoverySpawnSource(std::function<Vec3()> source) {
        recovery_spawn_source_ = std::move(source);
    }
    void SetDebugToggleCallback(std::function<void()> cb) { debug_toggle_callback_ = std::move(cb); }
    void SetCurrentRoom(const std::string& id) { current_room_ = id; }
    const std::string& CurrentRoom() const { return current_room_; }
    void SetRoomSwitchCallback(std::function<void(const std::string&, const Vec3&)> cb) { room_switch_callback_ = std::move(cb); }
    void SwitchRoom(const std::string& id, const Vec3& spawn) {
        if (room_switch_callback_) room_switch_callback_(id, spawn);
    }
    void SetDragStateCallback(std::function<bool(float&, bool&, bool&)> cb) { drag_state_callback_ = std::move(cb); }
    void SetDragUpdateCallback(std::function<void(const Vec3&, const std::string&, uint64_t)> cb) { drag_update_callback_ = std::move(cb); }
    void SetInteractCallback(std::function<void()> cb) { interact_callback_ = std::move(cb); }
    void SetNarratorIntrusionCallback(std::function<void()> cb) { narrator_intrusion_callback_ = std::move(cb); }
    void SetSaveCallback(std::function<void()> cb) { save_callback_ = std::move(cb); }
    void SetLoadCallback(std::function<void()> cb) { load_callback_ = std::move(cb); }
    void SetPauseCallback(std::function<void()> cb) { pause_callback_ = std::move(cb); }
    void SetTimeGate(RuntimeTimeGate* gate) { time_gate_ = gate; }
    void SetMeleeCallback(std::function<void()> cb) { melee_callback_ = std::move(cb); }
    void SetFireCallback(std::function<void(const FireRequest&, const WeaponDef&)> cb) {
        fire_callback_ = std::move(cb);
    }

    const char* Name() const override { return "player"; }

private:
    EngineContext ctx_{};
    InputMapper mapper_;
    InputState input_;
    LocomotionState locomotion_;
    CombatState combat_;
    const IWorldQuery* world_query_ = nullptr;
    std::function<void()> debug_toggle_callback_;
    std::string current_room_;
    std::function<void(const std::string&, const Vec3&)> room_switch_callback_;
    std::function<bool(float&, bool&, bool&)> drag_state_callback_;
    std::function<void(const Vec3&, const std::string&, uint64_t)> drag_update_callback_;
    std::function<void()> interact_callback_;
    std::function<void()> narrator_intrusion_callback_;
    std::function<void()> save_callback_;
    std::function<void()> load_callback_;
    std::function<void()> pause_callback_;
    std::function<void()> melee_callback_;
    std::function<void(const FireRequest&, const WeaponDef&)> fire_callback_;
    std::function<Vec3()> recovery_spawn_source_;
    bool paused_ = false;
    uint16_t health_ = 100;
    bool dead_ = false;
    uint64_t current_frame_ = 0;
    RuntimeTimeGate* time_gate_ = nullptr;
};

class NarrativeModule final : public IEngineModule {
public:
    void Init(const EngineContext& ctx) override {
        ctx_ = ctx;
        if (!ctx_.data_dir.empty()) {
            LoadTextResources(ctx_.data_dir + "/text/recovery_text.txt");
            const auto result = engine_.LoadBinary(
                ctx_.data_dir + "/storylets/storylets.bin");
            if (result.IsError()) {
                // No content = graceful no-storylet state (never a fake pass).
                engine_ = StoryletEngine();
            }
        }
    }
    void Shutdown() override {}

    void SetFacts(const FactStore* facts) { facts_ = facts; }
    StoryletEngine& Storylets() { return engine_; }
    DialogueQueue& Subtitles() { return queue_; }
    CausalityLedger& Ledger() { return ledger_; }
    void SetWorldCommandSink(std::function<void(const WorldCommand&)> sink) {
        world_command_sink_ = std::move(sink);
    }

    void SaveState(Serializer& serializer) const {
        serializer.WriteU32(1);
        engine_.Save(serializer);
        queue_.Save(serializer);
        ledger_.Save(serializer);
    }

    bool LoadState(Deserializer& deserializer) {
        const uint32_t version = deserializer.ReadU32();
        if (deserializer.HasError() || version != 1) {
            deserializer.MarkError();
            return false;
        }
        StoryletEngine restored_engine;
        DialogueQueue restored_queue;
        CausalityLedger restored_ledger;
        restored_engine.Load(deserializer);
        restored_queue.Load(deserializer);
        restored_ledger.Load(deserializer);
        if (deserializer.HasError() || !deserializer.AtEnd()) {
            deserializer.MarkError();
            return false;
        }
        engine_ = std::move(restored_engine);
        queue_ = std::move(restored_queue);
        ledger_ = std::move(restored_ledger);
        return true;
    }

    void SetPauseSource(std::function<bool()> source) {
        pause_source_ = std::move(source);
    }
    void SetGameFrameSource(std::function<uint64_t()> source) {
        game_frame_source_ = std::move(source);
    }
    void SetDifficulty(uint8_t difficulty) {
        difficulty_ = std::min<uint8_t>(difficulty, 2);
    }

    void SimTick(const SimClock& clock) override {
        if (facts_ == nullptr) return;
        if (pause_source_ && pause_source_()) return;
        const uint64_t frame = game_frame_source_
                                   ? game_frame_source_()
                                   : clock.FrameCount();
        const Storylet* s = engine_.SelectEligible(
            *facts_, {}, {}, {}, difficulty_, frame);
        if (s != nullptr) {
            const Storylet selected = *s;
            // Resolve player-facing text before consuming the once-only
            // storylet. Missing production text fails closed; an internal id
            // is never rendered as dialogue.
            std::map<std::string, std::string> resolved_text;
            bool text_ready = true;
            for (const auto& action : selected.actions) {
                std::visit([&](const auto& value) {
                    using Action = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<Action, NarratorLineAction> ||
                                  std::is_same_v<Action, DialogAction>) {
                        const std::string text = ResolveTextId(value.text_id);
                        if (text.empty()) {
                            text_ready = false;
                        } else {
                            resolved_text[value.text_id] = text;
                        }
                    }
                }, action);
            }
            if (!text_ready) {
                if (!text_resolution_error_reported_) {
                    std::fprintf(stderr,
                                 "NARRATIVE_TEXT_RESOLUTION=FAILED_CLOSED\n");
                    text_resolution_error_reported_ = true;
                }
                queue_.Advance(static_cast<uint32_t>(frame));
                return;
            }
            engine_.MarkFired(s->id);
            const EventId trigger_event = ctx_.events != nullptr
                ? ctx_.events->Post(EventStoryletTrigger{
                                        selected.id, EntityId::New(1)},
                                    EventKind::Notification, EntityId::New(1),
                                    EntityId::Invalid(), EventId::Invalid(), frame)
                : EventId::Invalid();
            for (const auto& action : selected.actions) {
                std::visit([&](const auto& value) {
                    using Action = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<Action, NarratorLineAction>) {
                        last_presented_text_ = resolved_text[value.text_id];
                        ++presented_action_count_;
                        queue_.Push(SubtitleLine{last_presented_text_,
                                                  static_cast<uint32_t>(frame),
                                                  240, NarratorSpeakerId(),
                                                  value.persona, 0});
                    } else if constexpr (std::is_same_v<Action, DialogAction>) {
                        last_presented_text_ = resolved_text[value.text_id];
                        ++presented_action_count_;
                        queue_.Push(SubtitleLine{last_presented_text_,
                                                  static_cast<uint32_t>(frame),
                                                  240, NpcId{}, 0, 0});
                    } else if constexpr (std::is_same_v<Action, WorldCommandAction>) {
                        if (world_command_sink_) world_command_sink_(value.command);
                    } else if constexpr (std::is_same_v<Action, EndGameCommand>) {
                        // Endgame is intentionally not present in the current
                        // recovery storylet content. If authored later, emit
                        // the typed fact and let the owning game flow decide
                        // whether the slice has reached an ending.
                        if (ctx_.events != nullptr) {
                            ctx_.events->Post(EventGameOver{value.ending_index},
                                              EventKind::Notification,
                                              EntityId::New(1), EntityId::Invalid(),
                                              trigger_event, frame);
                        }
                    }
                }, action);
            }
            // Real causality: the ledger entry references the storylet event.
            ledger_.Push(CausalityEntry{
                trigger_event,
                EventId::Invalid(),  // root of a causality chain
                frame, EventKind::Notification});
        }
        queue_.Advance(static_cast<uint32_t>(frame));
        // No fake frame->frame-1 causality chains (F-07 closure).
        // The ledger is populated only by real storylet/event occurrences.
    }

    const char* Name() const override { return "narrative"; }
    const std::string& LastPresentedText() const { return last_presented_text_; }
    size_t PresentedActionCount() const { return presented_action_count_; }

private:
    bool LoadTextResources(const std::string& path) {
        std::ifstream input(path);
        if (!input) {
            text_resources_.clear();
            return false;
        }
        std::map<std::string, std::string> parsed;
        std::string line;
        size_t line_count = 0;
        while (std::getline(input, line)) {
            if (++line_count > 512) return false;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            const size_t tab = line.find('\t');
            if (tab == std::string::npos || tab == 0 || tab + 1 >= line.size() ||
                tab > 128 || line.size() - tab - 1 > 512) {
                return false;
            }
            if (!parsed.emplace(line.substr(0, tab), line.substr(tab + 1)).second) {
                return false;
            }
        }
        text_resources_ = std::move(parsed);
        return true;
    }

    std::string ResolveTextId(const std::string& id) const {
        const auto it = text_resources_.find(id);
        return it == text_resources_.end() ? std::string{} : it->second;
    }

    EngineContext ctx_{};
    StoryletEngine engine_;
    DialogueQueue queue_;
    CausalityLedger ledger_;
    const FactStore* facts_ = nullptr;
    std::function<bool()> pause_source_;
    std::function<uint64_t()> game_frame_source_;
    std::function<void(const WorldCommand&)> world_command_sink_;
    uint8_t difficulty_ = 1;
    std::map<std::string, std::string> text_resources_;
    bool text_resolution_error_reported_ = false;
    std::string last_presented_text_;
    size_t presented_action_count_ = 0;
};

class RenderModule final : public IRenderModule {
public:
    RenderModule(std::unique_ptr<ITerminalBackend> backend, int w, int h)
        : backend_(std::move(backend)),
          width_(w),
        height_(h) {
        body_.assign(static_cast<size_t>(w) * h, CharCell{});
        backend_->Init(w, h);
    }

    void SetPlayerView(const Vec3& pos, float yaw) {
        player_pos_ = pos;
        player_yaw_ = yaw;
    }
    // Per-frame source of truth: the renderer reads the player's live
    // locomotion state instead of a one-time snapshot (F-23 closure).
    void SetCombatSource(const CombatState* combat) { combat_ = combat; }
    void SetHealthSource(const uint16_t* health) { health_ = health; }
    void SetNpcSource(const std::vector<RuntimeNpc>* npcs) { npcs_ = npcs; }
    void SetSystemicSource(const SystemicWorld* systemic) { systemic_ = systemic; }
    void SetDialogueSource(const DialogueQueue* dialogue) { dialogue_ = dialogue; }
    void SetSceneEntities(const std::vector<SceneEntity>* entities) {
        scene_entities_ = entities;
    }
    void SetSettingsSource(const Settings* settings) { settings_ = settings; }
    void SetPauseSource(std::function<bool()> source) {
        pause_source_ = std::move(source);
    }
    void SetGameFrameSource(std::function<uint64_t()> source) {
        game_frame_source_ = std::move(source);
    }
    void SetActiveRoom(RoomId room) { active_room_ = room; }
    void SetSceneId(const std::string& id, uint64_t frame = 0) {
        scene_id_ = id;
        scene_enter_frame_ = frame;
    }
    void SetObjectiveSource(std::function<std::string()> source) {
        objective_source_ = std::move(source);
    }
    void SetInteractionPromptSource(std::function<std::string()> source) {
        interaction_prompt_source_ = std::move(source);
    }
    bool LoadCharacterArt(const std::string& path) {
        return character_art_.Load(path);
    }
    bool CharacterArtLoadedFromFile() const {
        return character_art_.LoadedFromFile();
    }
    void SetCameraOverride(const Vec3& position, float yaw, float pitch) {
        camera_override_ = true;
        override_position_ = position;
        override_yaw_ = yaw;
        override_pitch_ = pitch;
    }
    void SetDebugOverlay(bool enabled) { debug_overlay_ = enabled; }
    void SetSubtitleOnce(const std::string& text, uint64_t frames) { subtitle_override_ = text; subtitle_override_remaining_ = frames; }
    void TriggerNarratorIntrusion(uint64_t frames) {
        narrator_intrusion_remaining_ = frames;
        narrator_intrusion_total_ = frames;
    }
    bool NarratorTypographyActive() const { return narrator_intrusion_remaining_ > 0; }
    bool DebugOverlay() const { return debug_overlay_; }
    bool ObjectiveVisible() const { return !objective_.empty(); }
    // A completed quest may legitimately clear its active objective.  Replay
    // validation needs the durable observation that the objective was shown
    // while the quest was active, rather than sampling the transient HUD at
    // the end of the route.
    bool ObjectiveWasPresented() const { return objective_was_presented_; }
    const std::string& ObjectiveText() const { return objective_; }

    void SetLocomotionSource(const LocomotionState* locomotion) {
        locomotion_ = locomotion;
    }
    void TriggerShotFeedback(const ShotFeedback& feedback) {
        shot_flash_remaining_ = 4;
        shake_remaining_ = feedback.target_was_npc ? 7 : 4;
        if (feedback.target_was_npc) hit_flash_remaining_ = 6;
        if (feedback.target_died) explosion_remaining_ = 10;
    }
    void TriggerExplosion(uint64_t frames = 10) {
        explosion_remaining_ = std::max(explosion_remaining_, frames);
        shake_remaining_ = std::max(shake_remaining_, frames);
    }
    const std::vector<CharCell>& Cells() const { return body_; }

    void SetGridData(const GridCell* cells, int w, int h) {
        grid_cells_ = cells;
        grid_w_ = w;
        grid_h_ = h;
    }

    void RenderFrame(uint64_t frame_index, float alpha) override {
        (void)alpha;
        const uint64_t game_frame = game_frame_source_
                                        ? game_frame_source_()
                                        : frame_index;
        const bool paused = pause_source_ && pause_source_();
        std::fill(body_.begin(), body_.end(), CharCell{});
        if (!camera_override_ && locomotion_ != nullptr) {
            player_pos_ = locomotion_->position;
            player_yaw_ = locomotion_->yaw;
        }
        if (grid_cells_ != nullptr && grid_w_ > 0 && grid_h_ > 0) {
            CharacterView view;
            view.origin = camera_override_ ? override_position_ : player_pos_;
            view.yaw = camera_override_ ? override_yaw_ : player_yaw_;
            view.pitch = camera_override_
                             ? override_pitch_
                             : (locomotion_ != nullptr ? locomotion_->pitch : 0.0f);
            if (!camera_override_ && locomotion_ != nullptr) {
                view.origin.z = locomotion_->EyePosition().z;
            } else if (!camera_override_) {
                view.origin.z = kEyeStand;
            } else if (std::fabs(view.origin.z) < 0.01f) {
                // Evidence cameras specify floor XY coordinates.  Keep the
                // normal player eye height unless a test explicitly supplies
                // a non-zero camera Z; otherwise authored sprites are
                // projected from the floor and disappear into the ceiling.
                view.origin.z = kEyeStand;
            }
            const float focal =
                0.5f * static_cast<float>(height_) /
                std::tan((settings_ != nullptr ? settings_->fov : 60.0f) *
                         3.14159265f / 360.0f);
            CharacterRenderOptions render_options;
            render_options.high_contrast = settings_ != nullptr &&
                                           settings_->high_contrast;
            RenderCharacterFrame(grid_cells_, grid_w_, grid_h_, view,
                                 body_.data(), width_, height_, focal,
                                 render_options);

            std::vector<CharacterSpriteInstance> sprites;
            const auto make_world_sprite = [&](uint64_t stable_id,
                                               const Vec3& position,
                                               float height,
                                               CharacterSpriteKind kind,
                                               float yaw) {
                CharacterSpriteInstance sprite;
                sprite.position = position;
                sprite.height = height;
                sprite.kind = kind;
                sprite.yaw = yaw;
                sprite.stable_id = stable_id;
                const float dx = position.x - view.origin.x;
                const float dy = position.y - view.origin.y;
                const float distance = std::sqrt(dx * dx + dy * dy);
                const auto prior = lod_states_.find(stable_id);
                const CharacterLod previous = prior == lod_states_.end()
                                                  ? SelectCharacterLod(distance)
                                                  : prior->second;
                sprite.lod_hint = prior == lod_states_.end()
                                      ? previous
                                      : SelectCharacterLodHysteretic(distance, previous);
                sprite.has_lod_hint = true;
                lod_states_[stable_id] = sprite.lod_hint;
                return sprite;
            };
            if (scene_id_ == "room_b1_revival") {
                if (npcs_ != nullptr) {
                    for (const auto& runtime : *npcs_) {
                        if (runtime.room != active_room_ ||
                            runtime.instance.state == NPCState::Dead ||
                            runtime.instance.state == NPCState::Stunned) continue;
                        CharacterSpriteKind kind = CharacterSpriteKind::SecurityGuard;
                        if (runtime.instance.cognition == CognitionTier::Full) {
                            kind = CharacterSpriteKind::FullHuman;
                        } else if (runtime.instance.role == Role::Cleaner ||
                                   runtime.instance.role == Role::Technician) {
                            kind = CharacterSpriteKind::MaintenanceWorker;
                        }
                        sprites.push_back(make_world_sprite(
                            0xA000000000000000ull | runtime.instance.id.GetValue(),
                            runtime.instance.position,
                            kind == CharacterSpriteKind::FullHuman ? 1.8f : 1.75f,
                            kind, runtime.instance.yaw));
                    }
                }
                // An incapacitated actor has one visual authority: the
                // exposed BodyRecord.  Never draw a standing NPC and its
                // floor body at the same time, and never let a hidden body
                // leak into the scene.
                if (systemic_ != nullptr) {
                    for (const auto& body : systemic_->Bodies()) {
                        if (body.room != active_room_ ||
                            body.disposition == BodyDisposition::HiddenInContainer ||
                            body.status == BodyStatus::Alive) {
                            continue;
                        }
                        const CharacterSpriteKind body_kind =
                            body.status == BodyStatus::Dead
                                ? CharacterSpriteKind::BodyDead
                                : CharacterSpriteKind::BodyUnconscious;
                        // Body art is a distinct floor pose.  Its visual
                        // authority is BodyRecord::position, not the
                        // incapacitated actor's standing sprite.
                        sprites.push_back(make_world_sprite(
                            0xB000000000000000ull | body.id.GetValue(),
                            body.position, 0.45f, body_kind, 0.0f));
                    }
                }
            } else if (scene_id_ == "room_1f_security") {
                if (npcs_ != nullptr) {
                    for (const auto& runtime : *npcs_) {
                        if (runtime.room == active_room_ &&
                            runtime.instance.role == Role::Guard &&
                            runtime.instance.state != NPCState::Dead &&
                            runtime.instance.state != NPCState::Stunned) {
                            sprites.push_back(make_world_sprite(
                                0xA000000000000000ull | runtime.instance.id.GetValue(),
                                runtime.instance.position, 1.8f,
                                CharacterSpriteKind::SecurityGuard,
                                runtime.instance.yaw));
                        }
                    }
                }
            } else {
                // Other authored recovery rooms use the same runtime NPC
                // records.  Keep the visual kind mapping identical to B1 so
                // rendering never invents a second placement source or
                // silently drops an active actor after a room transition.
                if (npcs_ != nullptr) {
                    for (const auto& runtime : *npcs_) {
                        if (runtime.room != active_room_ ||
                            runtime.instance.state == NPCState::Dead ||
                            runtime.instance.state == NPCState::Stunned) {
                            continue;
                        }
                        CharacterSpriteKind kind = CharacterSpriteKind::SecurityGuard;
                        if (runtime.instance.cognition == CognitionTier::Full) {
                            kind = CharacterSpriteKind::FullHuman;
                        } else if (runtime.instance.role == Role::Cleaner ||
                                   runtime.instance.role == Role::Technician) {
                            kind = CharacterSpriteKind::MaintenanceWorker;
                        }
                        sprites.push_back(make_world_sprite(
                            0xA000000000000000ull | runtime.instance.id.GetValue(),
                            runtime.instance.position,
                            kind == CharacterSpriteKind::FullHuman ? 1.8f : 1.75f,
                            kind, runtime.instance.yaw));
                    }
                }
            }
            if (scene_entities_ != nullptr) {
                for (const auto& entity : *scene_entities_) {
                    if (entity.room != scene_id_) continue;
                    sprites.push_back(make_world_sprite(
                        entity.stable_id, entity.position, entity.height,
                        entity.visual, entity.yaw));
                }
            }
            DrawCharacterSprites(view, sprites, character_art_, grid_cells_, grid_w_,
                                 grid_h_, body_.data(), width_, height_, focal,
                                 render_options);
            const WeaponSlot active_slot = ValidWeaponSlot(combat_);
            int vm_state = 0;
            float recoil = 0.0f;
            if (combat_ != nullptr && game_frame >= combat_->last_shot_frame &&
                game_frame - combat_->last_shot_frame < 4) {
                vm_state = 1;
                recoil = 1.0f - static_cast<float>(game_frame - combat_->last_shot_frame) / 4.0f;
            }
            if (combat_ != nullptr && combat_->reload_frames_left > 0) {
                vm_state = 3;
            }
            const PistolFrame weapon_frame = vm_state == 1 ? PistolFrame::Fire
                : vm_state == 3 ? PistolFrame::Reload
                : (game_frame % 96 < 48 ? PistolFrame::IdleA : PistolFrame::IdleB);
            DrawWeaponViewmodel(body_.data(), width_, height_, character_art_,
                                active_slot, weapon_frame, recoil, render_options);
            const bool advance_visual_time =
                !paused && (!has_rendered_game_frame_ ||
                            game_frame != last_render_game_frame_);
            DrawVisualEffects(game_frame, advance_visual_time);
        }
        const uint64_t scene_frame = game_frame >= scene_enter_frame_
                                         ? game_frame - scene_enter_frame_ : 0;
        if (scene_frame < 300 && scene_id_ == "room_b1_revival") {
            subtitle_ = "SYS/07: Wake cycle verified. B1 anomaly detected. Proceed to calibration.";
        } else if (scene_frame < 180 && scene_id_ == "room_01_calibration") {
            subtitle_ = "CALIBRATION / LOGISTICS: Verify the route. Keep the service door clear.";
        } else if (scene_frame < 180 && scene_id_ == "room_1f_security") {
            subtitle_ = "1F SECURITY: Present identity. Watch the cameras.";
        } else if (scene_frame < 180 && scene_id_ == "room_service_medical") {
            subtitle_ = "SERVICE / MEDICAL: The building still remembers its staff.";
        } else if (scene_frame < 180 && scene_id_ == "room_elevator_lobby") {
            subtitle_ = "ELEVATOR LOBBY: Restricted floors remain listening.";
        } else {
            subtitle_ = "";
        }
        if (scene_id_ == "room_b1_revival" && npcs_ != nullptr) {
            for (const auto& runtime : *npcs_) {
                if (runtime.room != active_room_ ||
                    (runtime.instance.role != Role::Cleaner &&
                     runtime.instance.role != Role::Technician) ||
                    runtime.instance.state == NPCState::Dead ||
                    runtime.instance.state == NPCState::Stunned) {
                    continue;
                }
                const float npc_dx = player_pos_.x - runtime.instance.position.x;
                const float npc_dy = player_pos_.y - runtime.instance.position.y;
                if ((npc_dx * npc_dx + npc_dy * npc_dy) < 9.0f) {
                    subtitle_ = "Maintenance: 07... you are not scheduled to be here.";
                    break;
                }
            }
        }
        if (dialogue_ != nullptr) {
            const auto active_lines = dialogue_->ActiveLines(
                static_cast<uint32_t>(std::min<uint64_t>(
                    game_frame, std::numeric_limits<uint32_t>::max())));
            if (!active_lines.empty()) subtitle_ = active_lines.back().text;
        }
        if (subtitle_override_remaining_ > 0) {
            subtitle_ = subtitle_override_;
            if (!paused) --subtitle_override_remaining_;
        }
        if (debug_overlay_) {
            subtitle_ = "F3 DEBUG | pos " + std::to_string(player_pos_.x) + "," +
                        std::to_string(player_pos_.y) + " yaw " + std::to_string(player_yaw_);
        }
        HudFrame hud;
        hud.health = health_ != nullptr ? *health_ : 100;
        if (combat_ != nullptr) {
            const size_t slot = static_cast<size_t>(ValidWeaponSlot(combat_));
            hud.ammo_mag = combat_->ammo_in_mag[slot];
            hud.ammo_reserve = combat_->reserve[slot];
        } else {
            hud.ammo_mag = 12;
            hud.ammo_reserve = 48;
        }
        if (settings_ != nullptr && settings_->preset == QualityPreset::Ultra120) {
            hud.preset_name = "ULTRA120";
        } else if (settings_ != nullptr && settings_->preset == QualityPreset::HighRefresh) {
            hud.preset_name = "HIGH_REFRESH";
        } else if (settings_ != nullptr && settings_->preset == QualityPreset::Compatibility) {
            hud.preset_name = "COMPATIBILITY";
        } else {
            hud.preset_name = "PRESENTATION60";
        }
        hud.weapon_name = WeaponSlotName(ValidWeaponSlot(combat_));
        objective_.clear();
        if (objective_source_) {
            objective_ = objective_source_();
        }
        if (objective_.empty() && systemic_ != nullptr) {
            for (const auto& quest : systemic_->Quests()) {
                if (quest.status == QuestStatus::Accepted ||
                    quest.status == QuestStatus::Active) {
                    objective_ = quest.presentation_objective;
                    break;
                }
            }
            // Keep the checkpoint legible for the player for the remainder of
            // the loaded scene.  Completing the quest must not make the
            // player-facing objective disappear at the exact moment the
            // checkpoint is reached.
            if (objective_.empty()) {
                for (const auto& quest : systemic_->Quests()) {
                    if (quest.status == QuestStatus::Completed &&
                        !quest.presentation_objective.empty()) {
                        objective_ = "CHECKPOINT REACHED: " +
                                     quest.presentation_objective;
                        break;
                    }
                }
            }
        }
        hud.objective = objective_.empty() ? nullptr : objective_.c_str();
        if (!objective_.empty()) objective_was_presented_ = true;
        interaction_prompt_ = interaction_prompt_source_
                                  ? interaction_prompt_source_()
                                  : std::string{};
        hud.interaction_prompt = interaction_prompt_.empty()
                                     ? nullptr
                                     : interaction_prompt_.c_str();
        hud.grid_width = grid_w_;
        hud.grid_height = grid_h_;
        hud.developer_overlay = debug_overlay_;
        hud.subtitle = settings_ == nullptr || settings_->subtitles
                           ? subtitle_.c_str() : nullptr;
        hud_.Draw(body_.data(), width_, height_, hud);
        if (narrator_intrusion_remaining_ > 0) {
            const bool reduce_flicker = settings_ != nullptr && settings_->reduce_flicker;
            const bool reduce_shake = settings_ != nullptr && settings_->reduce_camera_shake;
            for (int yy = 0; yy < height_; ++yy) {
                for (int xx = 0; xx < width_; ++xx) {
                    CharCell& c = body_[static_cast<size_t>(yy) * width_ + xx];
                    c.bg_r = 5; c.bg_g = 7; c.bg_b = 15;
                    c.fg_r = 64; c.fg_g = 42; c.fg_b = 66;
                }
            }
            const uint64_t elapsed = narrator_intrusion_total_ > narrator_intrusion_remaining_
                                         ? narrator_intrusion_total_ - narrator_intrusion_remaining_ : 0;
            DrawNarratorTypography(elapsed, reduce_flicker, reduce_shake);
            if (!paused) --narrator_intrusion_remaining_;
        }
        backend_->Submit(body_.data(), width_, height_);
        last_render_game_frame_ = game_frame;
        has_rendered_game_frame_ = true;
    }

    const char* Name() const override { return "render"; }

private:
    void DrawNarratorTypography(uint64_t elapsed, bool reduce_flicker,
                                bool reduce_shake) {
        // This is a small embedded BlockFontAtlas, rather than enlarged raw
        // ASCII.  Each 5x7 glyph is composed from block cells, laid on a
        // bottom-left to top-right diagonal.  The atlas keeps the effect
        // deterministic and asset-free while leaving a direct replacement
        // point for authored font art.
        using Glyph = std::array<const char*, 7>;
        const auto glyph = [](char ch) -> const Glyph& {
            static const Glyph blank{{".....", ".....", ".....", ".....",
                                      ".....", ".....", "....."}};
            static const Glyph y{{"X...X", "X...X", ".X.X.", "..X..",
                                  "..X..", "..X..", "..X.."}};
            static const Glyph o{{".XXX.", "X...X", "X...X", "X...X",
                                  "X...X", "X...X", ".XXX."}};
            static const Glyph u{{"X...X", "X...X", "X...X", "X...X",
                                  "X...X", "X...X", ".XXX."}};
            static const Glyph t{{"XXXXX", "..X..", "..X..", "..X..",
                                  "..X..", "..X..", "..X.."}};
            static const Glyph h{{"X...X", "X...X", "X...X", "XXXXX",
                                  "X...X", "X...X", "X...X"}};
            static const Glyph i{{"XXXXX", "..X..", "..X..", "..X..",
                                  "..X..", "..X..", "XXXXX"}};
            static const Glyph n{{"X...X", "XX..X", "XX..X", "X.X.X",
                                  "X..XX", "X..XX", "X...X"}};
            static const Glyph k{{"X...X", "X..X.", "X.X..", "XX...",
                                  "X.X..", "X..X.", "X...X"}};
            static const Glyph c{{".XXX.", "X...X", "X....", "X....",
                                  "X....", "X...X", ".XXX."}};
            static const Glyph a{{".XXX.", "X...X", "X...X", "XXXXX",
                                  "X...X", "X...X", "X...X"}};
            static const Glyph s{{".XXXX", "X....", "X....", ".XXX.",
                                  "....X", "....X", "XXXX."}};
            static const Glyph v{{"X...X", "X...X", "X...X", "X...X",
                                  "X...X", ".X.X.", "..X.."}};
            static const Glyph e{{"XXXXX", "X....", "X....", "XXXX.",
                                  "X....", "X....", "XXXXX"}};
            static const Glyph q{{".XXX.", "X...X", "....X", "...X.",
                                  "..X..", ".....", "..X.."}};
            switch (ch) {
            case 'Y': return y;
            case 'O': return o;
            case 'U': return u;
            case 'T': return t;
            case 'H': return h;
            case 'I': return i;
            case 'N': return n;
            case 'K': return k;
            case 'C': return c;
            case 'A': return a;
            case 'S': return s;
            case 'V': return v;
            case 'E': return e;
            case '?': return q;
            default: return blank;
            }
        };

        constexpr const char* kText = "YOU THINK YOU CAN SAVE THIS?";
        const int length = static_cast<int>(std::char_traits<char>::length(kText));
        const int scale = width_ >= 220 ? 2 : 1;
        // At ULTRA, a two-pixel glyph with a condensed eight-cell advance
        // occupies almost the whole 240-cell canvas while remaining legible.
        const int advance = scale == 2 ? 8 : 6;
        const int glyph_width = 5 * scale;
        const int glyph_height = 7 * scale;
        const int total_width = (length - 1) * advance + glyph_width;
        const int start_x = std::max(0, (width_ - total_width) / 2);
        const int start_y = std::max(0, height_ - glyph_height - 4);
        const int rise = std::max(0, height_ - glyph_height - 8);
        const uint64_t units = static_cast<uint64_t>(length) * 35;
        const uint64_t shown = std::min(units,
            ((elapsed + 1) * units) /
                std::max<uint64_t>(1, narrator_intrusion_total_));
        const int shake = (!reduce_shake && (!reduce_flicker || elapsed % 7 != 0))
                              ? static_cast<int>(elapsed % 3) - 1 : 0;

        for (int glyph_index = 0; glyph_index < length; ++glyph_index) {
            const Glyph& rows = glyph(kText[glyph_index]);
            const int x0 = start_x + glyph_index * advance + shake;
            const int y0 = start_y - (glyph_index * rise) /
                                            std::max(1, length - 1);
            for (int row = 0; row < 7; ++row) {
                for (int col = 0; col < 5; ++col) {
                    const uint64_t unit = static_cast<uint64_t>(glyph_index) * 35u +
                                          static_cast<uint64_t>(row * 5 + col);
                    if (rows[row][col] != 'X' || unit >= shown) continue;
                    for (int py = 0; py < scale; ++py) {
                        for (int px = 0; px < scale; ++px) {
                            const int x = x0 + col * scale + px;
                            const int y = y0 + row * scale + py;
                            if (x < 0 || x >= width_ || y < 0 || y >= height_) continue;
                            CharCell& cell = body_[static_cast<size_t>(y) * width_ + x];
                            const bool accent = !reduce_flicker &&
                                                ((elapsed + unit) % 29u == 0u);
                            cell.code_point = U'█';
                            cell.fg_r = accent ? 255 : 244;
                            cell.fg_g = accent ? 74 : (reduce_flicker ? 196 : 224);
                            cell.fg_b = accent ? 62 : 86;
                            cell.bg_r = 18;
                            cell.bg_g = 10;
                            cell.bg_b = 24;
                            cell.flags = 0x01;
                        }
            }
            }
        }
    }
    }

    void DrawVisualEffects(uint64_t frame_index, bool advance_timers) {
        const bool reduce_flicker = settings_ != nullptr && settings_->reduce_flicker;
        const bool reduce_shake = settings_ != nullptr && settings_->reduce_camera_shake;
        DrawCharacterEffects(body_.data(), width_, height_, frame_index,
                             shot_flash_remaining_ > 0,
                             hit_flash_remaining_ > 0,
                             explosion_remaining_ > 0,
                             reduce_flicker, reduce_shake);
        if (advance_timers) {
            if (shot_flash_remaining_ > 0) --shot_flash_remaining_;
            if (hit_flash_remaining_ > 0) --hit_flash_remaining_;
            if (explosion_remaining_ > 0) --explosion_remaining_;
            if (shake_remaining_ > 0) --shake_remaining_;
        }
    }

    std::unique_ptr<ITerminalBackend> backend_;
    const int width_;
    const int height_;
    HudRenderer hud_;
    CharacterArtBank character_art_;
    std::vector<CharCell> body_;
    Vec3 player_pos_;
    float player_yaw_ = 0.0f;
    bool camera_override_ = false;
    Vec3 override_position_;
    float override_yaw_ = 0.0f;
    float override_pitch_ = 0.0f;
    const LocomotionState* locomotion_ = nullptr;
    const CombatState* combat_ = nullptr;
    const uint16_t* health_ = nullptr;
    const std::vector<RuntimeNpc>* npcs_ = nullptr;
    const SystemicWorld* systemic_ = nullptr;
    const DialogueQueue* dialogue_ = nullptr;
    const std::vector<SceneEntity>* scene_entities_ = nullptr;
    const Settings* settings_ = nullptr;
    bool debug_overlay_ = false;
    std::string subtitle_override_;
    uint64_t subtitle_override_remaining_ = 0;
    uint64_t narrator_intrusion_remaining_ = 0;
    uint64_t narrator_intrusion_total_ = 0;
    uint64_t shot_flash_remaining_ = 0;
    uint64_t hit_flash_remaining_ = 0;
    uint64_t explosion_remaining_ = 0;
    uint64_t shake_remaining_ = 0;
    const GridCell* grid_cells_ = nullptr;
    int grid_w_ = 0;
    int grid_h_ = 0;
    std::string subtitle_;
    std::string objective_;
    std::string interaction_prompt_;
    bool objective_was_presented_ = false;
    std::string scene_id_ = "room_b1_revival";
    uint64_t scene_enter_frame_ = 0;
    RoomId active_room_;
    std::function<bool()> pause_source_;
    std::function<uint64_t()> game_frame_source_;
    std::function<std::string()> objective_source_;
    std::function<std::string()> interaction_prompt_source_;
    uint64_t last_render_game_frame_ = 0;
    bool has_rendered_game_frame_ = false;
    std::map<uint64_t, CharacterLod> lod_states_;
};

struct GameServices {
    std::unique_ptr<WorldModule> world;
    std::unique_ptr<PlayerModule> player;
    std::unique_ptr<AiModule> ai;
    std::unique_ptr<NarrativeModule> narrative;
    std::unique_ptr<SystemicWorld> systemic;  // M1-owned runtime systemic state
};

class ReplayKeyboardBackend final : public IInputBackend {
public:
    explicit ReplayKeyboardBackend(std::string path) : path_(std::move(path)) {
        std::ifstream input(path_);
        if (!input) {
            error_ = "cannot open replay file";
            return;
        }
        std::string line;
        size_t line_number = 0;
        while (std::getline(input, line)) {
            ++line_number;
            const size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos || line[first] == '#') continue;
            std::istringstream fields(line.substr(first));
            uint64_t frame = 0;
            std::string key_name;
            std::string transition;
            if (!(fields >> frame >> key_name >> transition)) {
                error_ = "malformed replay line " + std::to_string(line_number);
                return;
            }
            std::string normalized_key = key_name;
            std::transform(normalized_key.begin(), normalized_key.end(),
                           normalized_key.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
            if (normalized_key == "MOUSEMOVE" || normalized_key == "MOUSEDELTA") {
                // Mouse deltas are consumed by ReplayMouseBackend.  The
                // keyboard parser must tolerate the shared replay file.
                continue;
            }
            const PhysicalKey key = ParseKey(key_name);
            if (key == PhysicalKey::Unknown) {
                error_ = "unknown replay key on line " + std::to_string(line_number);
                return;
            }
            bool pressed = false;
            if (transition == "down" || transition == "press" || transition == "1") {
                pressed = true;
            } else if (transition != "up" && transition != "release" && transition != "0") {
                error_ = "unknown replay transition on line " +
                         std::to_string(line_number);
                return;
            }
            events_.push_back({frame, InputEvent{key, pressed, 0.0f}});
        }
        std::stable_sort(events_.begin(), events_.end(),
                         [](const ReplayEvent& a, const ReplayEvent& b) {
            return a.frame < b.frame;
        });
        valid_ = true;
    }

    bool Init() override { return valid_; }
    void Shutdown() override {}
    bool Poll(InputEvent& out_event) override {
        // The keyboard is sampled once per fixed tick. All records at the
        // current replay frame are emitted, then one false return advances to
        // the next tick so InputRuntime's existing drain semantics are used.
        while (next_ < events_.size() && events_[next_].frame < frame_) ++next_;
        if (next_ < events_.size() && events_[next_].frame == frame_) {
            out_event = events_[next_++].event;
            ++consumed_events_;
            return true;
        }
        ++frame_;
        return false;
    }
    bool HasFocus() const override { return true; }
    const char* Name() const override { return "replay-keyboard"; }
    const std::string& Error() const { return error_; }
    size_t ConsumedEventCount() const { return consumed_events_; }

private:
    struct ReplayEvent {
        uint64_t frame = 0;
        InputEvent event;
    };

    static PhysicalKey ParseKey(std::string name) {
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        if (name == "W") return PhysicalKey::W;
        if (name == "A") return PhysicalKey::A;
        if (name == "S") return PhysicalKey::S;
        if (name == "D") return PhysicalKey::D;
        if (name == "Q") return PhysicalKey::Q;
        if (name == "E") return PhysicalKey::E;
        if (name == "R") return PhysicalKey::R;
        if (name == "F") return PhysicalKey::F;
        if (name == "V") return PhysicalKey::V;
        if (name == "SHIFT") return PhysicalKey::Shift;
        if (name == "CTRL" || name == "CONTROL") return PhysicalKey::Ctrl;
        if (name == "SPACE") return PhysicalKey::Space;
        if (name == "ESC" || name == "ESCAPE") return PhysicalKey::Escape;
        if (name == "F1") return PhysicalKey::F1;
        if (name == "F3") return PhysicalKey::F3;
        if (name == "F5") return PhysicalKey::F5;
        if (name == "F9") return PhysicalKey::F9;
        if (name == "NUM1" || name == "1") return PhysicalKey::Num1;
        if (name == "NUM2" || name == "2") return PhysicalKey::Num2;
        if (name == "NUM3" || name == "3") return PhysicalKey::Num3;
        if (name == "MOUSELEFT" || name == "LMB") return PhysicalKey::MouseLeft;
        if (name == "MOUSERIGHT" || name == "RMB") return PhysicalKey::MouseRight;
        return PhysicalKey::Unknown;
    }

    std::string path_;
    std::string error_;
    std::vector<ReplayEvent> events_;
    size_t next_ = 0;
    uint64_t frame_ = 0;
    bool valid_ = false;
    size_t consumed_events_ = 0;
};

class ReplayMouseBackend final : public IInputBackend {
public:
    explicit ReplayMouseBackend(std::string path) : path_(std::move(path)) {
        std::ifstream input(path_);
        if (!input) {
            error_ = "cannot open replay file";
            return;
        }
        std::string line;
        size_t line_number = 0;
        while (std::getline(input, line)) {
            ++line_number;
            const size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos || line[first] == '#') continue;
            std::istringstream fields(line.substr(first));
            uint64_t frame = 0;
            std::string kind;
            if (!(fields >> frame >> kind)) {
                error_ = "malformed replay line " + std::to_string(line_number);
                return;
            }
            std::transform(kind.begin(), kind.end(), kind.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            if (kind != "MOUSEMOVE" && kind != "MOUSEDELTA") continue;
            float dx = 0.0f;
            float dy = 0.0f;
            if (!(fields >> dx >> dy) || !std::isfinite(dx) || !std::isfinite(dy)) {
                error_ = "malformed mouse replay line " + std::to_string(line_number);
                return;
            }
            events_.push_back({frame, Vec2{dx, dy}});
        }
        std::stable_sort(events_.begin(), events_.end(),
                         [](const ReplayEvent& a, const ReplayEvent& b) {
            return a.frame < b.frame;
        });
        valid_ = true;
    }

    bool Init() override { return valid_; }
    void Shutdown() override {}
    bool Poll(InputEvent& out_event) override {
        while (next_ < events_.size() && events_[next_].frame < frame_) ++next_;
        if (next_ < events_.size() && events_[next_].frame == frame_) {
            delta_.x += events_[next_].delta.x;
            delta_.y += events_[next_].delta.y;
            ++next_;
            ++consumed_events_;
            out_event = InputEvent{}; // sample-only event; delta is consumed below
            return true;
        }
        ++frame_;
        return false;
    }
    bool HasFocus() const override { return true; }
    const char* Name() const override { return "replay-mouse"; }
    size_t ConsumedEventCount() const { return consumed_events_; }
    bool ConsumeMouseDelta(Vec2& out) override {
        out = delta_;
        delta_ = Vec2{};
        return out.x != 0.0f || out.y != 0.0f;
    }
    const std::string& Error() const { return error_; }

private:
    struct ReplayEvent {
        uint64_t frame = 0;
        Vec2 delta;
    };

    std::string path_;
    std::string error_;
    std::vector<ReplayEvent> events_;
    size_t next_ = 0;
    uint64_t frame_ = 0;
    Vec2 delta_;
    bool valid_ = false;
    size_t consumed_events_ = 0;
};

// InputModule: private integration class that samples the keyboard and mouse
// backends through InputRuntime each sim tick. Uses the production runtime
// backend selection (Raw Input primary, CursorDelta fallback) so the app and
// the input probe share the same pointer path.
class InputModule final : public IEngineModule {
public:
    explicit InputModule(std::unique_ptr<IInputBackend> replay_keyboard = nullptr,
                         std::unique_ptr<IInputBackend> replay_mouse = nullptr)
        : selection_(CreateRuntimeBackendSelection(PointerBackendPreference::Auto)),
          runtime_(nullptr, nullptr) {
        if (replay_keyboard) {
            selection_.pointer_name = "replay";
            selection_.mouse_button_source = "replay";
            runtime_ = InputRuntime(std::move(replay_keyboard),
                                    std::move(replay_mouse));
        } else {
            runtime_ = InputRuntime(std::move(selection_.keyboard),
                                    std::move(selection_.pointer));
        }
    }
    ~InputModule() override = default;

    // Exposes the backend report for diagnostics / probe parity.
    const PointerBackendSelection& Selection() const { return selection_; }

    void Init(const EngineContext&) override { ready_ = runtime_.Init(); }
    void Shutdown() override { runtime_.Shutdown(); }
    bool Ready() const { return ready_; }

    void SetTarget(InputState* input, const InputMapper* mapper) {
        input_ = input;
        mapper_ = mapper;
    }

    void SimTick(const SimClock&) override {
        if (!input_ || !mapper_) return;
        runtime_.SampleTick(*input_, *mapper_);
    }
    size_t ReplayKeyboardEventCount() const {
        const auto* backend = dynamic_cast<const ReplayKeyboardBackend*>(runtime_.Keyboard());
        return backend != nullptr ? backend->ConsumedEventCount() : 0;
    }
    size_t ReplayMouseEventCount() const {
        const auto* backend = dynamic_cast<const ReplayMouseBackend*>(runtime_.Mouse());
        return backend != nullptr ? backend->ConsumedEventCount() : 0;
    }
    const char* Name() const override { return "input"; }

private:
    PointerBackendSelection selection_;
    InputRuntime runtime_;
    InputState* input_ = nullptr;
    const InputMapper* mapper_ = nullptr;
    bool ready_ = false;
};

// Builds the four semantic modules against one shared EngineContext.
// The render module and terminal backend are wired by the app entry after
// probing the terminal (they need wall resolution decisions).
Result<GameServices> BuildGame(const EngineContext& ctx, const GameConfig& config) {
    (void)config;
    GameServices g;
    g.world = std::make_unique<WorldModule>();
    g.world->SetRoomOverride(config.room_id);
    g.world->Init(ctx);
    if (!ctx.data_dir.empty() &&
        !g.world->LoadAuthoredFacts(ctx.data_dir + "/facts/facts.bin")) {
        return Result<GameServices>::Err(13, "authored fact registry rejected");
    }
    g.player = std::make_unique<PlayerModule>();
    g.player->Init(ctx);
    g.player->SetWorldQuery(&g.world->Query());
    g.player->Locomotion().position =
        g.world->HasLoadedRoom() ? g.world->LoadedRoom().spawn_point
                                 : Vec3{1.5f, 6.0f, 0.0f};
    g.ai = std::make_unique<AiModule>();
    g.ai->Init(ctx);
    g.narrative = std::make_unique<NarrativeModule>();
    g.narrative->Init(ctx);
    g.narrative->SetFacts(&g.world->Facts());
    g.systemic = std::make_unique<SystemicWorld>();
    if (!ctx.data_dir.empty()) {
        const std::string seed_path = ctx.data_dir + "/systemic/systemic_seed.bin";
        const auto seed = g.systemic->LoadSeedBinary(seed_path);
        if (seed.IsError()) {
            return Result<GameServices>::Err(seed.Error().code, seed.Error().message);
        }
    }
    g.ai->AttachSystemic(g.systemic.get());
    return Result<GameServices>::Ok(std::move(g));
}

int RunComposition(const GameConfig& config) {
    const RuntimePaths runtime_paths = ResolveRuntimePaths(
        config.executable_path, config.data_dir, config.user_data_dir);
    const std::filesystem::path& data_root = runtime_paths.data_dir;
    const std::filesystem::path& user_data_root = runtime_paths.user_data_dir;
    std::error_code data_ec;
    const bool seed_present = std::filesystem::is_regular_file(
        data_root / "systemic" / "systemic_seed.bin", data_ec);
    if (!seed_present) {
        std::fprintf(stderr,
                     "Missing game data: expected %s relative to the player executable.\n",
                     (data_root / "systemic" / "systemic_seed.bin").string().c_str());
        return 8;
    }
    std::error_code user_data_ec;
    std::filesystem::create_directories(user_data_root, user_data_ec);

    SimClock clock;
    EventBus events;
    DeterministicRNG sim_rng(config.seed);
    Settings settings = Settings::Defaults();
    bool settings_loaded_from_disk = false;
    Logger logger;
    logger.SetMinLevel(LogLevel::Info);
    if (std::filesystem::is_regular_file(user_data_root / "settings.cfg")) {
        SettingsRegistry registry;
        const auto loaded = registry.Load((user_data_root / "settings.cfg").string());
        if (loaded.IsOk()) {
            settings = loaded.Value();
            settings_loaded_from_disk = true;
        } else {
            logger.Warn("settings", "settings.cfg is invalid; using defaults");
        }
    }
    if (user_data_ec) {
        logger.Warn("userdata", "user data directory is unavailable; saves may fail");
    }

    EngineContext ctx;
    ctx.clock = &clock;
    ctx.events = &events;
    ctx.sim_rng = &sim_rng;
    ctx.settings = &settings;
    ctx.logger = &logger;
    ctx.data_dir = data_root.string();

    auto build_result = BuildGame(ctx, config);
    if (build_result.IsError()) {
        std::fprintf(stderr, "systemic seed startup failed: %s\n",
                     build_result.Error().message.c_str());
        return 8;
    }
    GameServices services = std::move(build_result.Value());
    services.player->SetRecoverySpawnSource([&services] {
        return services.world->HasLoadedRoom()
                   ? services.world->LoadedRoom().spawn_point
                   : Vec3{1.5f, 6.0f, 0.0f};
    });
    RuntimeTimeGate time_gate;
    services.player->SetTimeGate(&time_gate);
    services.world->SetPauseSource([&time_gate] { return time_gate.Paused(); });
    services.world->SetGameFrameSource([&time_gate] { return time_gate.GameFrame(); });
    services.ai->SetPauseSource([&time_gate] { return time_gate.Paused(); });
    services.ai->SetGameFrameSource([&time_gate] { return time_gate.GameFrame(); });
    services.narrative->SetPauseSource([&time_gate] { return time_gate.Paused(); });
    services.narrative->SetGameFrameSource([&time_gate] { return time_gate.GameFrame(); });
    services.narrative->SetDifficulty(settings.difficulty);
    services.narrative->SetWorldCommandSink([&](const WorldCommand& command) {
        services.world->PushCommand(command);
    });
    if (!services.world->HasLoadedRoom()) {
        std::fprintf(stderr,
                     "Missing game data: expected a compiled room under %s relative to the player executable.\n",
                     (data_root / "rooms").string().c_str());
        return 8;
    }
    SystemicEventBridge systemic_bridge(services.systemic.get());
    systemic_bridge.Register(events);

    auto audio = CreateAudioBackend();
    if (!audio || !audio->Init()) {
        logger.Warn("audio", "audio backend unavailable; subtitles remain enabled");
    } else {
        audio->SetVolume(settings.master_volume / 100.0f,
                         settings.sfx_volume / 100.0f,
                         settings.narrator_volume / 100.0f);
        logger.Info("audio", audio->Name());
    }
    const EventBus::ConsumerId audio_consumer = events.Register(
        [&](const WorldEvent& event) {
            if (!audio) return;
            if (std::holds_alternative<EventWeaponFire>(event.payload)) {
                audio->PlaySfx(AudioId::New(1), 0.9f);
            } else if (std::holds_alternative<EventDamage>(event.payload)) {
                audio->PlaySfx(AudioId::New(2), 0.8f);
            } else if (std::holds_alternative<EventDoorChange>(event.payload)) {
                audio->PlaySfx(AudioId::New(3), 0.7f);
            } else if (std::holds_alternative<EventNpcSpeak>(event.payload)) {
                audio->PlayVo(AudioId::New(4), 0.85f, 0xB10003u);
            }
        });

    // Runtime input bridge (ISSUE B): polls keyboard + mouse backends every
    // tick and maps them into PlayerModule's InputState.
    std::unique_ptr<IInputBackend> replay_keyboard;
    std::unique_ptr<IInputBackend> replay_mouse;
    if (!config.replay_path.empty()) {
        replay_keyboard = std::make_unique<ReplayKeyboardBackend>(config.replay_path);
        replay_mouse = std::make_unique<ReplayMouseBackend>(config.replay_path);
    }
    auto input_module = std::make_unique<InputModule>(std::move(replay_keyboard),
                                                     std::move(replay_mouse));
    input_module->SetTarget(&services.player->Input(),
                            &services.player->Mapper());
    input_module->Init(ctx);
    if (!input_module->Ready()) {
        std::fprintf(stderr, "input backend initialization failed%s\n",
                     config.replay_path.empty() ? "" : " for replay");
        return 12;
    }

    Engine engine;
    engine.SetContext(ctx);
    engine.RegisterModule(input_module.get());
    engine.RegisterModule(services.player.get());
    engine.RegisterModule(services.world.get());
    engine.RegisterModule(services.ai.get());
    engine.RegisterModule(services.narrative.get());

    // Terminal backend selection: marker variables are hints only; the
    // platform backend negotiates VT first so direct/Explorer launches do
    // not silently lose a capable Windows Terminal host.
    TerminalProbe probe = ProbeTerminalEnv();
    int terminal_w = config.terminal_w;
    int terminal_h = config.terminal_h;
    if (probe.max_width > 0) terminal_w = std::min(terminal_w, probe.max_width);
    if (probe.max_height > 0) terminal_h = std::min(terminal_h, probe.max_height);
    if (terminal_w != config.terminal_w || terminal_h != config.terminal_h) {
        std::fprintf(stderr,
                     "TERMINAL_DIMENSIONS_FIT=requested=%dx%d surface=%dx%d effective=%dx%d\n",
                     config.terminal_w, config.terminal_h,
                     probe.max_width, probe.max_height, terminal_w, terminal_h);
    }
    std::unique_ptr<ITerminalBackend> backend =
        CreateTerminalBackend(terminal_w, terminal_h, probe);
    if (!backend) {
        std::fprintf(stderr, "fatal: no terminal backend available\n");
        return 2;
    }
    if (!settings_loaded_from_disk ||
        (probe.vt_probe_succeeded && probe.true_color_verified &&
         settings.preset == QualityPreset::Compatibility)) {
        const int refresh_hint_hz = probe.vt_probe_succeeded ? 120 : 60;
        settings.preset = SuggestPreset(probe, refresh_hint_hz);
    }
    TerminalCaps terminal_caps = backend->GetCaps();
    // A host can resize between the initial probe and backend creation. Fit
    // once more to the backend's verified surface before constructing the
    // renderer; never silently submit a larger frame that would clip.
    if ((terminal_caps.max_width > 0 && terminal_w > terminal_caps.max_width) ||
        (terminal_caps.max_height > 0 && terminal_h > terminal_caps.max_height)) {
        terminal_w = std::min(terminal_w, terminal_caps.max_width > 0
                                             ? terminal_caps.max_width : terminal_w);
        terminal_h = std::min(terminal_h, terminal_caps.max_height > 0
                                             ? terminal_caps.max_height : terminal_h);
        backend->Shutdown();
        backend = CreateTerminalBackend(terminal_w, terminal_h, probe);
        if (!backend) {
            std::fprintf(stderr, "fatal: terminal surface changed during setup\n");
            return 2;
        }
        terminal_caps = backend->GetCaps();
    }
    std::fprintf(stderr,
                 "TERMINAL_BACKEND=%s TERMINAL_DIMENSIONS=%dx%d "
                 "TERMINAL_QUALITY_PRESET=%s TERMINAL_COLOR_CAPABILITY=%s "
                 "TERMINAL_PROBE=%s\n",
                 backend->Name(), terminal_w, terminal_h,
                 QualityPresetName(settings.preset),
                 terminal_caps.true_color ? "TRUECOLOR"
                                          : terminal_caps.win32_native ? "ANSI16"
                                                                       : "UNKNOWN",
                 TerminalKindName(probe));
    std::fprintf(stderr, "TERMINAL_CAPABILITY_REASON=%s VT_PROBE=%s\n",
                 probe.capability_reason.c_str(),
                 probe.vt_probe_succeeded ? "SUCCESS" : "FAILED_OR_UNAVAILABLE");
    auto render = std::make_unique<RenderModule>(std::move(backend),
                                                  terminal_w,
                                                  terminal_h);
    render->SetSettingsSource(&settings);
    render->SetPauseSource([&time_gate] { return time_gate.Paused(); });
    render->SetGameFrameSource([&time_gate] { return time_gate.GameFrame(); });
    render->SetSceneId(config.room_id.empty() ? "room_b1_revival" : config.room_id);
    const bool character_art_loaded = render->LoadCharacterArt(
        (data_root / "characters" / "b1_character_art.txt").string());
    if (!character_art_loaded) {
        logger.Warn("render", "character art bank unavailable; using bounded fallback");
    }
    if (config.camera_override) {
        render->SetCameraOverride(config.camera_position, config.camera_yaw,
                                  config.camera_pitch);
    }
    const Vec3 spawn = services.world->HasLoadedRoom()
                           ? services.world->LoadedRoom().spawn_point
                           : Vec3{1.5f, 6.0f, 0.0f};
    render->SetPlayerView(spawn, 0.0f);
    render->SetLocomotionSource(&services.player->Locomotion());
    render->SetCombatSource(&services.player->Combat());
    render->SetHealthSource(&services.player->Health());
    render->SetSystemicSource(services.systemic.get());
    render->SetDialogueSource(&services.narrative->Subtitles());

    // Recovery-04 owns a deliberately small compiled placement set. These
    // records are consumed by rendering, interaction, and room switching;
    // systemic records below reference the same authored coordinates.
    SceneRuntime scene_runtime;
    if (!scene_runtime.Load((data_root / "scenes" / "recovery_scene.bin").string())) {
        std::fprintf(stderr, "recovery scene content load failed\n");
        return 12;
    }
    const SceneEntity* gate_entity =
        scene_runtime.FindEntity("b1_service_door");
    const SceneEntity* reader_entity =
        scene_runtime.FindEntity("b1_service_reader");
    const SceneEntity* cart_entity =
        scene_runtime.FindEntity("b1_cleaning_cart");
    const SceneEntity* camera_entity =
        scene_runtime.FindEntity("b1_camera_04");
    const SceneEntity* terminal_entity =
        scene_runtime.FindEntity("b1_calibration_terminal");
    if (gate_entity != nullptr && reader_entity != nullptr &&
        cart_entity != nullptr && camera_entity != nullptr &&
        terminal_entity != nullptr && gate_entity->systemic_id != 0 &&
        gate_entity->systemic_id == reader_entity->systemic_id &&
        cart_entity->systemic_id != 0 && camera_entity->systemic_id != 0 &&
        terminal_entity->systemic_id != 0) {
        services.world->SetB1GateCell(
            static_cast<int32_t>(std::floor(gate_entity->position.x)),
            static_cast<int32_t>(std::floor(gate_entity->position.y)));
        services.world->SetB1GateId(DoorId::New(gate_entity->systemic_id));
    } else {
        std::fprintf(stderr, "recovery scene missing required systemic placements\n");
        return 12;
    }
    struct SliceRuntime {
        EntityId player = EntityId::New(1);
        NpcId guard_npc;
        NpcId cleaner_npc;
        EntityId body = EntityId::New(9002);
        ContainerId cart;
        TerminalId terminal;
        ObservationSourceId camera;
        QuestId opening_quest = QuestId::New(9005);
        RoomId b1_room;
        ItemId badge;
        ItemId cash;
        bool terminal_session = false;
        bool bribe_done = false;
        bool schedule_found = false;
        bool body_created = false;
        bool body_hidden = false;
        bool body_discovered = false;
        bool cleaner_response = false;
        bool shot_hit = false;
        bool nonlethal_hit = false;
        bool terminal_attempted = false;
        bool terminal_denied = false;
        bool player_died = false;
        bool player_recovered = false;
        bool access_attempted = false;
        bool access_denied = false;
        bool gate_unlocked = false;
        bool gate_open = false;
        bool gate_crossed = false;
    } slice;
    slice.cart = ContainerId::New(cart_entity->systemic_id);
    slice.terminal = TerminalId::New(terminal_entity->systemic_id);
    slice.camera = ObservationSourceId::New(camera_entity->systemic_id);
    render->SetSceneEntities(&scene_runtime.Entities());
    std::vector<std::string> replay_route;
    bool replay_save_attempted = false;
    bool replay_load_attempted = false;
    bool replay_save_ok = false;
    bool replay_load_ok = false;
    const EventBus::ConsumerId player_damage_consumer = events.Register(
        [&](const WorldEvent& event) {
            const auto* damage = std::get_if<EventPlayerDamage>(&event.payload);
            if (damage == nullptr || event.target_entity != EntityId::New(1)) return;
            if (!services.player->ApplyDamage(damage->amount)) return;
            if (services.player->Dead()) {
                slice.player_died = true;
            }
            render->SetSubtitleOnce(services.player->Dead()
                                        ? "YOU ARE DOWN. F9 loads the last checkpoint."
                                        : "IMPACT. Health is now authoritative.",
                                    services.player->Dead() ? 240 : 90);
        });
    if (services.world->HasLoadedRoom() && services.world->IsB1Loaded()) {
        slice.b1_room = services.world->LoadedRoom().id;
    }
    for (const auto& actor : services.systemic->Actors()) {
        if (actor.role == Role::Guard && !slice.guard_npc.IsValid()) {
            slice.guard_npc = actor.id;
        }
        if (actor.role == Role::Cleaner && !slice.cleaner_npc.IsValid()) {
            slice.cleaner_npc = actor.id;
        }
    }
    if (!slice.guard_npc.IsValid() && !services.systemic->Actors().empty()) {
        slice.guard_npc = services.systemic->Actors().front().id;
    }
    if (!slice.cleaner_npc.IsValid()) slice.cleaner_npc = slice.guard_npc;
    for (const auto& item : services.systemic->Items()) {
        if (item.type == ItemType::Badge && !slice.badge.IsValid()) slice.badge = item.id;
        if (item.type == ItemType::Cash && !slice.cash.IsValid()) slice.cash = item.id;
    }
    if (slice.badge.IsValid()) {
        const ItemRecord* badge = services.systemic->GetItem(slice.badge);
        if (badge != nullptr && badge->owner.IsValid() &&
            services.systemic->GetActor(NpcId::New(badge->owner.GetValue())) != nullptr) {
            // The authored badge owner is the incapacitated NPC in this slice;
            // do not silently bind the body to whichever guard appears first
            // in the seed ordering.
            slice.guard_npc = NpcId::New(badge->owner.GetValue());
        }
    }
    if (slice.b1_room.IsValid() && slice.guard_npc.IsValid()) {
        HideableContainer cart;
        cart.id = slice.cart;
        cart.kind = ContainerKind::CleaningCart;
        const SceneEntity* placement_cart =
            scene_runtime.FindEntity("b1_cleaning_cart");
        if (placement_cart == nullptr) {
            std::fprintf(stderr, "recovery scene missing b1_cleaning_cart\n");
            return 9;
        }
        cart.position = placement_cart->position;
        cart.room = slice.b1_room;
        cart.capacity_volume = 0.6f;
        cart.concealment = 95;
        cart.accessibility = 80;
        cart.routine_tags.push_back(RoutineTag::Cleaner);
        TerminalRecord terminal;
        terminal.id = slice.terminal;
        terminal.room = slice.b1_room;
        terminal.credential_requirement = 2;
        terminal.access_scope.push_back("B1_CALIBRATION");
        ObservationSource camera;
        camera.id = slice.camera;
        camera.type = ObservationSourceType::Camera;
        camera.room = slice.b1_room;
        camera.online = true;
        camera.network_segment = "B1_SECURITY_LOOP";
        camera.provenance = "B1 ceiling camera 04";
        QuestRecord opening;
        opening.id = slice.opening_quest;
        opening.title = "Calibration route";
        opening.presentation_objective = "Reach the security checkpoint";
        opening.status = QuestStatus::Offered;
        const bool setup_ok = services.systemic->AddContainer(cart) &&
                              services.systemic->AddTerminal(terminal) &&
                              services.systemic->AddObservationSource(camera) &&
                              services.systemic->AddQuest(opening);
        if (!setup_ok) {
            std::fprintf(stderr, "pvs slice systemic setup rejected\n");
            return 9;
        }
        (void)services.systemic->TransitionQuest(
            slice.opening_quest, QuestStatus::Accepted, 0,
            "B1 recovery route accepted");
        (void)services.systemic->TransitionQuest(
            slice.opening_quest, QuestStatus::Active, 0,
            "Reach the security checkpoint");
    }
    services.world->SetBooleanFact(RuntimeFactId("fact_player_has_gun"),
                                    slice.player, false);
    // Alpha-01 milestone facts are authored in data/facts and are reset for
    // a fresh runtime.  Durable consequences continue to live in the
    // existing systemic/save state rather than in replay-only flags.
    for (const std::string_view fact : {
             "fact_b1_badge_obtained", "fact_b1_camera_disabled",
             "fact_b1_terminal_accessed", "fact_b1_gate_denied",
             "fact_b1_body_hidden", "fact_b1_checkpoint_reached",
             "fact_b1_loud_action", "fact_b1_nonlethal_action",
             "fact_alpha_medical_reached", "fact_alpha_security_reached",
             "fact_alpha_bribe_accepted"}) {
        services.world->SetBooleanFact(RuntimeFactId(fact), slice.player, false);
    }

    // Bind authored identity records to a small real runtime population. The
    // incapacitated badge owner remains a body, while the other seed actors
    // receive their authored binary profile and use the same perception,
    // memory, decision, and event path as production play.
    if (services.world->HasLoadedRoom()) {
        const auto profiles = LoadNpcProfiles(ctx.data_dir + "/npcs/npcs.bin");
        if (profiles.IsError()) {
            std::fprintf(stderr, "npc profile startup failed: %s\n",
                         profiles.Error().message.c_str());
            return 10;
        }
        for (const auto& profile : profiles.Value()) {
            const ActorRecord* actor = services.systemic->GetActor(profile.id);
            if (actor == nullptr) {
                std::fprintf(stderr, "npc profile has no systemic actor\n");
                return 10;
            }
            // The authored B1 profile places the background administrator on
            // the same cell as officer_davis.  Keeping both runtime collision
            // targets makes a deterministic hitscan choose the wrong person
            // for the badge/body slice.  The administrator remains a systemic
            // actor; this bounded recovery scene does not instantiate that
            // overlapping background target.
            if (profile.spawn_room == slice.b1_room &&
                actor->role == Role::Administrator && profile.id != slice.guard_npc) {
                continue;
            }
            NPCInstance npc;
            npc.id = profile.id;
            npc.data_key = actor->data_key;
            npc.cognition = actor->cognition;
            npc.faction = actor->faction;
            npc.role = actor->role;
            npc.position = profile.spawn_position;
            npc.yaw = profile.spawn_yaw;
            npc.health = profile.health;
            npc.state = NPCState::Patrol;
            npc.is_critical = profile.is_critical;
            npc.sight_range = profile.sight_range;
            npc.sight_fov_rad = profile.sight_fov_rad;
            npc.hearing_range = profile.hearing_range;
            if (!services.ai->AddNpc(npc, profile.spawn_room)) {
                std::fprintf(stderr, "pvs runtime npc setup rejected\n");
                return 10;
            }
        }
        services.ai->SetWorldQuery(&services.world->Query());
        services.ai->SetActiveRoom(services.world->LoadedRoom().id);
        services.ai->SetPlayerPositionSource([&] {
            return services.player->Locomotion().position;
        });
        services.ai->SetPlayerEyeSource([&] {
            return services.player->Locomotion().EyePosition().z;
        });
        services.ai->SetPlayerVisibilitySource([&] {
            const LocomotionState& locomotion = services.player->Locomotion();
            const float speed = std::sqrt(locomotion.velocity.x * locomotion.velocity.x +
                                          locomotion.velocity.y * locomotion.velocity.y);
            const float posture_factor = locomotion.posture == Posture::Stand ? 1.0f
                : locomotion.posture == Posture::Crouch ? 0.55f : 0.30f;
            const float motion_factor = std::clamp(speed / 2.5f, 0.0f, 1.0f);
            const int col = static_cast<int>(std::floor(locomotion.position.x));
            const int row = static_cast<int>(std::floor(locomotion.position.y));
            const GridCell cell = services.world->Query().GetCell(col, row);
            const float light_factor = 0.40f + 0.60f *
                (static_cast<float>(cell.light) / 255.0f);
            return std::clamp(posture_factor * (0.45f + 0.55f * motion_factor) *
                              light_factor, 0.0f, 1.0f);
        });
        services.ai->SetPlayerTargetActiveSource([&] {
            return !services.player->Dead();
        });
        for (const auto& route : scene_runtime.PatrolRoutes()) {
            const auto route_npc = std::find_if(
                services.ai->Npcs().begin(), services.ai->Npcs().end(),
                [&](const RuntimeNpc& runtime) {
                    return runtime.instance.id == route.npc;
                });
            if (route_npc == services.ai->Npcs().end() ||
                route_npc->room != RoomId::New(StableContentId(route.room))) {
                std::fprintf(stderr, "recovery scene patrol room/entity mismatch\n");
                return 11;
            }
            if (!services.ai->SetPatrolRoute(route.npc, route.points)) {
                std::fprintf(stderr, "recovery scene patrol route rejected\n");
                return 11;
            }
        }
        const bool cleaner_present = std::any_of(
            services.ai->Npcs().begin(), services.ai->Npcs().end(),
            [&](const RuntimeNpc& runtime) {
                return runtime.instance.id == slice.cleaner_npc;
            });
        if (slice.b1_room.IsValid() && slice.cleaner_npc.IsValid() && cleaner_present &&
            !services.ai->ConfigureBodyDiscovery(slice.cleaner_npc, slice.body,
                                                 slice.cart, 0)) {
            std::fprintf(stderr, "pvs body-discovery runtime setup rejected\n");
            return 11;
        }
    }
    PlayerActorWorldQuery player_world_query(
        &services.world->Query(), &services.ai->Npcs(),
        services.world->LoadedRoom().id);
    services.player->SetWorldQuery(&player_world_query);
    render->SetNpcSource(&services.ai->Npcs());
    bool debug_overlay = false;
    services.player->SetDebugToggleCallback([&] {
        debug_overlay = !debug_overlay;
        render->SetDebugOverlay(debug_overlay);
    });
    services.player->SetNarratorIntrusionCallback([&] {
        render->TriggerNarratorIntrusion(240);
    });
    auto sync_body_from_feedback = [&](const ShotFeedback& feedback) {
        if (!feedback.target_was_npc ||
            (!feedback.target_stunned && !feedback.target_died) ||
            feedback.npc != slice.guard_npc ||
            services.systemic->GetBody(slice.body) != nullptr) {
            return;
        }
        const RuntimeNpc* target = nullptr;
        for (const auto& runtime : services.ai->Npcs()) {
            if (runtime.instance.id == feedback.npc) {
                target = &runtime;
                break;
            }
        }
        if (target == nullptr || target->room != slice.b1_room) return;
        BodyRecord body;
        body.id = slice.body;
        body.npc = feedback.npc;
        body.status = feedback.target_died ? BodyStatus::Dead : BodyStatus::Unconscious;
        body.disposition = BodyDisposition::Exposed;
        body.position = target->instance.position;
        body.room = target->room;
        if (!services.systemic->AddBody(body)) return;
        const EntityId target_entity = EntityId::New(feedback.npc.GetValue());
        if (slice.badge.IsValid() &&
            services.systemic->ItemHeldBy(slice.badge, target_entity)) {
            (void)services.systemic->TransferItem(slice.badge, slice.body);
        }
        slice.body_created = true;
        slice.nonlethal_hit = feedback.target_stunned;
        render->SetSubtitleOnce(feedback.target_stunned
                                    ? "Target down. Search the body before moving on."
                                    : "Target dead. The scene is now evidence.",
                                180);
    };
    services.ai->SetShotFeedbackCallback([&](const ShotFeedback& feedback) {
        render->TriggerShotFeedback(feedback);
        sync_body_from_feedback(feedback);
    });
    const EventBus::ConsumerId speech_consumer = events.Register(
        [&](const WorldEvent& event) {
            const auto* speech = std::get_if<EventNpcSpeak>(&event.payload);
            if (speech == nullptr) return;
            const std::string text = speech->line == StringId::New(0xB1003)
                ? "SECURITY: Stop. Identify yourself."
                : "NPC: communication received.";
            render->SetSubtitleOnce(text, 150);
        });
    services.player->SetFireCallback([&](const FireRequest& request,
                                          const WeaponDef& weapon) {
        const ShotFeedback feedback = services.ai->HandlePlayerShot(
            request, weapon, services.player->CurrentFrame());
        slice.shot_hit = slice.shot_hit || feedback.target_was_npc;
        slice.nonlethal_hit = slice.nonlethal_hit || feedback.target_stunned;
        services.world->SetBooleanFact(RuntimeFactId("fact_player_has_gun"),
                                        slice.player, true);
        services.world->SetBooleanFact(RuntimeFactId("fact_b1_loud_action"),
                                        slice.player, true);
    });

    auto switch_room = [&](const std::string& id, const Vec3& spawn_point,
                           float destination_yaw = std::numeric_limits<float>::quiet_NaN()) -> bool {
        if (!services.world->LoadRoomById(id)) return false;
        // A drag is a room-local interaction transaction.  End it only after
        // the destination was loaded successfully, so a failed transition
        // does not discard an otherwise valid active drag.
        if (services.systemic->GetDrag(slice.body) != nullptr) {
            (void)services.systemic->EndDrag(slice.body,
                                              services.player->CurrentFrame());
        }
        player_world_query.SetStaticQuery(&services.world->Query());
        player_world_query.SetActiveRoom(services.world->LoadedRoom().id);
        services.player->SetWorldQuery(&player_world_query);
        services.ai->SetWorldQuery(&services.world->Query());
        services.player->Locomotion().position = spawn_point;
        services.player->Locomotion().velocity = Vec3{};
        const float room_yaw = services.world->LoadedRoom().spawn_yaw;
        services.player->Locomotion().yaw = std::isfinite(destination_yaw)
                                                ? destination_yaw : room_yaw;
        services.player->SetCurrentRoom(id);
        if (id == "room_service_medical") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_alpha_medical_reached"),
                slice.player, true);
        } else if (id == "room_1f_security") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_alpha_security_reached"),
                slice.player, true);
        }
        services.ai->SetActiveRoom(services.world->LoadedRoom().id);
        const Room& room = services.world->LoadedRoom();
        render->SetGridData(room.grid.Data().data(), room.grid.Width(), room.grid.Height());
        render->SetActiveRoom(room.id);
        render->SetPlayerView(spawn_point, services.player->Locomotion().yaw);
        render->SetSceneId(id, services.player->CurrentFrame());
        if (!config.replay_path.empty() &&
            (replay_route.empty() || replay_route.back() != id)) {
            replay_route.push_back(id);
        }
        return true;
    };

    const auto try_room_portal = [&](const Vec3& position) {
        const std::string& current_room = services.player->CurrentRoom();
        const SceneTransition* portal = FindSceneTransitionAt(
            scene_runtime, current_room, position);
        if (portal == nullptr) return false;
        if (!switch_room(portal->destination_room, portal->destination_spawn,
                         portal->destination_yaw)) {
            render->SetSubtitleOnce(portal->unavailable_message, 120);
        }
        return true;
    };

    auto serialize_player = [&]() {
        std::vector<uint8_t> bytes;
        Serializer s(bytes);
        s.WriteString(services.player->CurrentRoom());
        const LocomotionState& loco = services.player->Locomotion();
        s.WriteF32(loco.position.x);
        s.WriteF32(loco.position.y);
        s.WriteF32(loco.position.z);
        s.WriteF32(loco.velocity.x);
        s.WriteF32(loco.velocity.y);
        s.WriteF32(loco.velocity.z);
        s.WriteF32(loco.yaw);
        s.WriteF32(loco.pitch);
        s.WriteU8(static_cast<uint8_t>(loco.posture));
        s.WriteU8(static_cast<uint8_t>(loco.traversal));
        s.WriteU8(static_cast<uint8_t>(loco.lean));
        s.WriteU8(loco.contact.grounded ? 1 : 0);
        s.WriteU8(loco.contact.on_ladder ? 1 : 0);
        s.WriteU8(loco.contact.on_climbable ? 1 : 0);
        SerializeCombatState(s, services.player->Combat());
        s.WriteU16(services.player->Health());
        s.WriteU8(services.player->Dead() ? 1 : 0);
        s.WriteU16(loco.jump_cooldown_frames);
        return bytes;
    };

    services.player->SetSaveCallback([&] {
        replay_save_attempted = true;
        std::vector<SaveSection> sections;
        std::vector<uint8_t> rng_b, ev_b, pl_b, w_b, ai_b, n_b, sy_b;
        { Serializer s(rng_b); sim_rng.Save(s); }
        { Serializer s(ev_b); events.Save(s); }
        pl_b = serialize_player();
        { Serializer s(w_b); services.world->SaveState(s); }
        { Serializer s(ai_b); services.ai->Save(s); }
        { Serializer s(n_b); services.narrative->SaveState(s); }
        sy_b = services.systemic->Serialize();
        if (sy_b.empty()) {
            render->SetSubtitleOnce("Save failed: systemic state invalid.", 180);
            return;
        }
        sections.push_back({SaveSectionId::Player, std::move(pl_b)});
        sections.push_back({SaveSectionId::World, std::move(w_b)});
        sections.push_back({SaveSectionId::Rng, std::move(rng_b)});
        sections.push_back({SaveSectionId::Events, std::move(ev_b)});
        sections.push_back({SaveSectionId::Ai, std::move(ai_b)});
        sections.push_back({SaveSectionId::Narrative, std::move(n_b)});
        sections.push_back({SaveSectionId::Systemic, std::move(sy_b)});
        std::error_code ec;
        const std::filesystem::path save_dir = user_data_root / "saves";
        std::filesystem::create_directories(save_dir, ec);
        if (ec) {
            replay_save_ok = false;
            render->SetSubtitleOnce("Save failed: user data unavailable.", 180);
            return;
        }
        SaveManager save;
        const auto res = save.SaveWorld((save_dir / "pvs_manual").string(), sections);
        replay_save_ok = res.IsOk();
        render->SetSubtitleOnce(res.IsOk() ? "Saved." : "Save failed.", 120);
    });
    services.player->SetLoadCallback([&] {
        replay_load_attempted = true;
        SaveManager save;
        const auto loaded = save.LoadWorld(
            (user_data_root / "saves" / "pvs_manual").string());
        if (loaded.IsError()) { render->SetSubtitleOnce("Load failed.", 120); return; }
        const SaveSection* player_section = nullptr;
        const SaveSection* world_section = nullptr;
        const SaveSection* rng_section = nullptr;
        const SaveSection* events_section = nullptr;
        const SaveSection* ai_section = nullptr;
        const SaveSection* narrative_section = nullptr;
        const SaveSection* systemic_section = nullptr;
        for (const auto& sec : loaded.Value()) {
            switch (sec.id) {
            case SaveSectionId::Player: player_section = &sec; break;
            case SaveSectionId::World: world_section = &sec; break;
            case SaveSectionId::Rng: rng_section = &sec; break;
            case SaveSectionId::Events: events_section = &sec; break;
            case SaveSectionId::Ai: ai_section = &sec; break;
            case SaveSectionId::Narrative: narrative_section = &sec; break;
            case SaveSectionId::Systemic: systemic_section = &sec; break;
            default: break;
            }
        }
        if (player_section == nullptr || world_section == nullptr ||
            rng_section == nullptr || events_section == nullptr || ai_section == nullptr ||
            narrative_section == nullptr || systemic_section == nullptr) {
            render->SetSubtitleOnce("Load failed: incomplete save.", 180);
            return;
        }

        // The staged sections above are validated without touching the live
        // modules.  Keep a complete live checkpoint as well: a failure after
        // the first commit operation must restore every section, not merely
        // the player pose.  This is deliberately local to the existing save
        // seam; it is not a second save architecture.
        const std::string live_room = services.player->CurrentRoom();
        const LocomotionState live_loco = services.player->Locomotion();
        const CombatState live_combat = services.player->Combat();
        const uint16_t live_health = services.player->Health();
        const bool live_dead = services.player->Dead();
        const std::vector<std::string> live_replay_route = replay_route;
        const std::vector<uint8_t> live_player = serialize_player();
        std::vector<uint8_t> live_world;
        std::vector<uint8_t> live_events;
        std::vector<uint8_t> live_rng;
        std::vector<uint8_t> live_ai;
        std::vector<uint8_t> live_narrative;
        {
            Serializer s(live_world);
            services.world->SaveState(s);
        }
        {
            Serializer s(live_events);
            events.Save(s);
        }
        {
            Serializer s(live_rng);
            sim_rng.Save(s);
        }
        {
            Serializer s(live_ai);
            services.ai->Save(s);
        }
        {
            Serializer s(live_narrative);
            services.narrative->SaveState(s);
        }
        const std::vector<uint8_t> live_systemic = services.systemic->Serialize();
        std::string restored_room;
        LocomotionState restored_loco = services.player->Locomotion();
        CombatState restored_combat = services.player->Combat();
        uint16_t restored_health = 100;
        bool restored_dead = false;
        SystemicWorld restored_systemic;
        WorldModule restored_world;
        EventBus restored_events;
        DeterministicRNG restored_rng;
        NarrativeModule restored_narrative;
        AutonomousNpcSystem restored_ai;
        auto fail_load = [&](const char* text) {
            render->SetSubtitleOnce(text, 180);
        };

        const auto fault_requested = [](const char* stage) {
            const char* configured = std::getenv(
                "WRITEOVER_RECOVERY_FAIL_FINAL_COMMIT_STAGE");
            return configured != nullptr && std::string_view(configured) == stage;
        };

        const auto rollback_live = [&]() {
            bool ok = switch_room(live_room, live_loco.position, live_loco.yaw);
            if (!ok) return false;

            {
                Deserializer d(live_world.data(), live_world.size());
                ok = services.world->LoadState(d) && !d.HasError() && d.AtEnd() && ok;
            }
            const auto restored = SystemicWorld::Deserialize(
                live_systemic.data(), live_systemic.size());
            if (restored.IsError()) return false;
            *services.systemic = restored.Value();
            {
                Deserializer d(live_events.data(), live_events.size());
                events.Load(d);
                ok = !d.HasError() && d.AtEnd() && ok;
            }
            {
                Deserializer d(live_rng.data(), live_rng.size());
                sim_rng.Load(d);
                ok = !d.HasError() && d.AtEnd() && ok;
            }
            {
                Deserializer d(live_narrative.data(), live_narrative.size());
                ok = services.narrative->LoadState(d) && !d.HasError() && d.AtEnd() && ok;
            }
            {
                Deserializer d(live_ai.data(), live_ai.size());
                ok = services.ai->Load(d) && !d.HasError() && d.AtEnd() && ok;
            }
            services.player->Locomotion() = live_loco;
            services.player->Combat() = live_combat;
            services.player->SetHealthState(live_health, live_dead);
            replay_route = live_replay_route;

            // Compare the serialized live sections after rollback.  This
            // catches a rollback that merely returned to the right room while
            // leaving events, RNG, AI, or durable systemic history changed.
            std::vector<uint8_t> world_now;
            std::vector<uint8_t> events_now;
            std::vector<uint8_t> rng_now;
            std::vector<uint8_t> ai_now;
            std::vector<uint8_t> narrative_now;
            {
                Serializer s(world_now);
                services.world->SaveState(s);
            }
            {
                Serializer s(events_now);
                events.Save(s);
            }
            {
                Serializer s(rng_now);
                sim_rng.Save(s);
            }
            {
                Serializer s(ai_now);
                services.ai->Save(s);
            }
            {
                Serializer s(narrative_now);
                services.narrative->SaveState(s);
            }
            return ok && serialize_player() == live_player &&
                   world_now == live_world && events_now == live_events &&
                   rng_now == live_rng && ai_now == live_ai &&
                   narrative_now == live_narrative &&
                   services.systemic->Serialize() == live_systemic &&
                   services.player->CurrentRoom() == live_room;
        };

        const auto fail_commit = [&](const char* text, const char* stage) {
            const bool rollback_ok = rollback_live();
            std::fprintf(stderr, "SAVE_FINAL_COMMIT_FAILURE_STAGE=%s\n", stage);
            std::fprintf(stderr, "SAVE_FINAL_COMMIT_ROLLBACK=%s\n",
                         rollback_ok ? "PASS" : "FAIL");
            replay_load_ok = false;
            fail_load(text);
        };
        {
            Deserializer d(player_section->data.data(), player_section->data.size());
            const uint32_t room_len = d.ReadU32();
            if (d.HasError() || room_len > 128 || room_len > d.Remaining()) {
                fail_load("Load failed: invalid player room."); return;
            }
            restored_room.resize(room_len);
            if (room_len > 0) d.ReadBytes(restored_room.data(), room_len);
            restored_loco.position.x = d.ReadF32();
            restored_loco.position.y = d.ReadF32();
            restored_loco.position.z = d.ReadF32();
            restored_loco.velocity.x = d.ReadF32();
            restored_loco.velocity.y = d.ReadF32();
            restored_loco.velocity.z = d.ReadF32();
            restored_loco.yaw = d.ReadF32();
            restored_loco.pitch = d.ReadF32();
            const uint8_t posture = d.ReadU8();
            const uint8_t traversal = d.ReadU8();
            const uint8_t lean = d.ReadU8();
            const uint8_t grounded = d.ReadU8();
            const uint8_t ladder = d.ReadU8();
            const uint8_t climbable = d.ReadU8();
            DeserializeCombatState(d, restored_combat);
            // Older local saves did not carry player health.  Treat their
            // absent field as a live full-health checkpoint while validating
            // the new fields whenever present.
            if (!d.AtEnd()) {
                restored_health = d.ReadU16();
                const uint8_t dead = d.ReadU8();
                if (dead > 1) {
                    fail_load("Load failed: invalid player health state."); return;
                }
                restored_dead = dead != 0;
            }
            // Older local saves have no jump-cooldown tail.  Their restored
            // cooldown is intentionally reset rather than inherited from
            // the live player; current saves carry the authoritative value.
            restored_loco.jump_cooldown_frames = 0;
            if (!d.AtEnd()) {
                restored_loco.jump_cooldown_frames = d.ReadU16();
            }
            if (grounded > 1 || ladder > 1 || climbable > 1 || d.HasError() || !d.AtEnd() ||
                posture > static_cast<uint8_t>(Posture::Prone) ||
                traversal > static_cast<uint8_t>(Traversal::Mantle) ||
                lean > static_cast<uint8_t>(Lean::Right) ||
                static_cast<uint8_t>(restored_combat.slot) >= kWeaponSlotCount ||
                !std::isfinite(restored_loco.position.x) ||
                !std::isfinite(restored_loco.position.y) ||
                !std::isfinite(restored_loco.position.z) ||
                !std::isfinite(restored_loco.velocity.x) ||
                !std::isfinite(restored_loco.velocity.y) ||
                !std::isfinite(restored_loco.velocity.z) ||
                 !std::isfinite(restored_loco.yaw) || !std::isfinite(restored_loco.pitch) ||
                 !std::isfinite(restored_combat.spread_factor) ||
                 restored_combat.spread_factor < 0.0f || restored_combat.spread_factor > 1.0f ||
                 restored_loco.jump_cooldown_frames > kJumpCooldownFrames ||
                 restored_health > 100 ||
                 (restored_dead && restored_health != 0)) {
                fail_load("Load failed: invalid player state."); return;
            }
            restored_loco.posture = static_cast<Posture>(posture);
            restored_loco.traversal = static_cast<Traversal>(traversal);
            restored_loco.lean = static_cast<Lean>(lean);
            restored_loco.contact.grounded = grounded != 0;
            restored_loco.contact.on_ladder = ladder != 0;
            restored_loco.contact.on_climbable = climbable != 0;
        }
        {
            const auto restored = SystemicWorld::Deserialize(
                systemic_section->data.data(), systemic_section->data.size());
            if (restored.IsError()) {
                fail_load("Load failed: systemic state rejected."); return;
            }
            restored_systemic = std::move(restored.Value());
        }
        {
            Deserializer d(world_section->data.data(), world_section->data.size());
            if (!restored_world.LoadState(d) || d.HasError() || !d.AtEnd()) {
                fail_load("Load failed: world state rejected."); return;
            }
        }
        {
            Deserializer d(events_section->data.data(), events_section->data.size());
            restored_events.Load(d);
            if (d.HasError() || !d.AtEnd()) {
                fail_load("Load failed: event state rejected."); return;
            }
        }
        {
            Deserializer d(rng_section->data.data(), rng_section->data.size());
            restored_rng.Load(d);
            if (d.HasError() || !d.AtEnd() ||
                (restored_rng.GetState0() == 0 && restored_rng.GetState1() == 0)) {
                fail_load("Load failed: RNG state rejected."); return;
            }
        }
        {
            Deserializer d(narrative_section->data.data(), narrative_section->data.size());
            if (!restored_narrative.LoadState(d) || d.HasError() || !d.AtEnd()) {
                fail_load("Load failed: narrative state rejected."); return;
            }
        }
        {
            // AI identities are preflighted against a staged runtime before
            // any live section is committed. This closes the partial-load
            // failure where an unknown later NPC left earlier NPCs restored.
            restored_ai.Attach(&restored_systemic, &restored_events, &restored_rng);
            for (const auto& runtime : services.ai->Npcs()) {
                if (!restored_ai.AddNpc(runtime.instance, runtime.room)) {
                    fail_load("Load failed: AI runtime setup rejected."); return;
                }
            }
            Deserializer ai_d(ai_section->data.data(), ai_section->data.size());
            if (!restored_ai.Load(ai_d) || ai_d.HasError() || !ai_d.AtEnd()) {
                fail_load("Load failed: AI state rejected."); return;
            }
        }
        if (restored_room.empty()) {
            fail_load("Load failed: incomplete player room."); return;
        }
        {
            // Loading a serialized world state does not itself load room
            // geometry. Probe the exact authored room before touching the
            // live room/query/player state; fallback rooms are not accepted.
            WorldModule room_probe;
            room_probe.SetRoomOverride(restored_room);
            room_probe.Init(ctx);
            if (!room_probe.HasLoadedRoom() ||
                room_probe.LoadedRoom().id !=
                    RoomId::New(StableContentId(restored_room))) {
                fail_load("Load failed: saved room unavailable."); return;
            }
        }
        if (!switch_room(restored_room, restored_loco.position)) {
            fail_commit("Load failed: saved room unavailable.", "room");
            return;
        }
        if (fault_requested("after_room")) {
            fail_commit("Load failed: injected final-commit fault.", "after_room");
            return;
        }
        Deserializer world_d(world_section->data.data(), world_section->data.size());
        Deserializer events_d(events_section->data.data(), events_section->data.size());
        Deserializer rng_d(rng_section->data.data(), rng_section->data.size());
        Deserializer narrative_d(narrative_section->data.data(), narrative_section->data.size());
        Deserializer ai_d(ai_section->data.data(), ai_section->data.size());
        if (!services.world->LoadState(world_d) || world_d.HasError() || !world_d.AtEnd()) {
            fail_commit("Load failed: world state commit rejected.", "world"); return;
        }
        if (fault_requested("after_world")) {
            fail_commit("Load failed: injected final-commit fault.", "after_world");
            return;
        }
        *services.systemic = std::move(restored_systemic);
        if (fault_requested("after_systemic")) {
            fail_commit("Load failed: injected final-commit fault.", "after_systemic");
            return;
        }
        events.Load(events_d);
        if (events_d.HasError() || !events_d.AtEnd()) {
            fail_commit("Load failed: event state commit rejected.", "events");
            return;
        }
        if (fault_requested("after_events")) {
            fail_commit("Load failed: injected final-commit fault.", "after_events");
            return;
        }
        sim_rng.Load(rng_d);
        if (rng_d.HasError() || !rng_d.AtEnd()) {
            fail_commit("Load failed: RNG state commit rejected.", "rng");
            return;
        }
        if (fault_requested("after_rng")) {
            fail_commit("Load failed: injected final-commit fault.", "after_rng");
            return;
        }
        if (!services.narrative->LoadState(narrative_d)) {
            fail_commit("Load failed: narrative state commit rejected.", "narrative");
            return;
        }
        if (narrative_d.HasError() || !narrative_d.AtEnd()) {
            fail_commit("Load failed: narrative state commit rejected.", "narrative");
            return;
        }
        if (fault_requested("after_narrative")) {
            fail_commit("Load failed: injected final-commit fault.", "after_narrative");
            return;
        }
        if (!services.ai->Load(ai_d) || ai_d.HasError() || !ai_d.AtEnd() ||
            events_d.HasError() || !events_d.AtEnd() || rng_d.HasError() || !rng_d.AtEnd()) {
            fail_commit("Load failed: runtime state commit rejected.", "ai"); return;
        }
        if (fault_requested("after_ai")) {
            fail_commit("Load failed: injected final-commit fault.", "after_ai");
            return;
        }
        services.player->Locomotion() = restored_loco;
        services.player->Combat() = restored_combat;
        services.player->SetHealthState(restored_health, restored_dead);
        if (fault_requested("after_player")) {
            fail_commit("Load failed: injected final-commit fault.", "after_player");
            return;
        }
        if (slice.player_died && !services.player->Dead()) {
            slice.player_recovered = true;
        }
        if (const DoorState* gate = services.world->Infra().GetDoor(services.world->B1GateId())) {
            slice.gate_unlocked = !gate->locked;
            slice.gate_open = gate->open && !gate->locked;
        }
        if (services.systemic->GetBody(slice.body) != nullptr) {
            slice.body_created = true;
        }
        // These are milestone facts, not current-state mirrors.  A successful
        // cleaner discovery intentionally changes the body disposition back to
        // Exposed, so reconstruct them from the durable systemic event ledger
        // instead of erasing the earlier hidden/discovered/response facts on
        // load.
        for (const auto& event : services.systemic->SystemEvents()) {
            if (event.type == SystemicEventType::BodyHidden &&
                event.target == slice.body) {
                slice.body_hidden = true;
            }
            if (event.type == SystemicEventType::BodyDiscovered &&
                event.target == EntityId::New(slice.guard_npc.GetValue())) {
                slice.body_discovered = true;
            }
            if ((event.type == SystemicEventType::Report ||
                 event.type == SystemicEventType::MedicalCall ||
                 event.type == SystemicEventType::HelpCoverUp) &&
                event.actor == EntityId::New(slice.cleaner_npc.GetValue())) {
                slice.cleaner_response = true;
            }
        }
        slice.terminal_session = false;
        for (const auto& session : services.systemic->TerminalSessions()) {
            if (session.terminal == slice.terminal && session.user == slice.player && session.active) {
                slice.terminal_session = true;
                break;
            }
        }
        replay_load_ok = true;
        render->SetSubtitleOnce("Loaded.", 120);
    });
    services.player->SetCurrentRoom(config.room_id.empty() ? std::string("room_b1_revival") : config.room_id);
    if (!config.replay_path.empty()) {
        replay_route.push_back(services.player->CurrentRoom());
    }
    services.player->SetRoomSwitchCallback([&](const std::string& id, const Vec3& spawn) {
        (void)switch_room(id, spawn);
    });
    // All player-facing room interactions use the same center view ray as
    // the B1 recovery slice.  The helper is a bounded shared seam, not a
    // second target system.
    const auto camera_looks_at = [&](const Vec3& target, float radius,
                                     float height) {
        return CameraRayHitsTarget(
            services.player->Locomotion(), terminal_w, terminal_h,
            settings.fov, services.world->Query(), target, radius, height);
    };
    // Alpha-01 presentation is intentionally a pair of small sources rather
    // than a new UI or quest framework. They read the same systemic facts and
    // authored scene records that the interaction callback below mutates.
    render->SetObjectiveSource([&] {
        const std::string& room = services.player->CurrentRoom();
        const bool has_badge = slice.badge.IsValid() &&
            services.systemic->ItemHeldBy(slice.badge, slice.player);
        if (room == "room_b1_revival") {
            if (const BodyRecord* body = services.systemic->GetBody(slice.body);
                body != nullptr && body->disposition == BodyDisposition::Exposed) {
                if (!body->searched) return std::string("B1: Search the downed guard");
                if (!has_badge) return std::string("B1: Take the access badge");
            }
            if (!has_badge) return std::string("B1: Find a way through the service route");
            if (!slice.terminal_session) return std::string("B1: Use the calibration terminal");
            if (!slice.gate_open) return std::string("B1: Use the service reader");
            if (!slice.gate_crossed) return std::string("B1: Cross the open service door");
            return std::string("B1 complete: continue to calibration");
        }
        if (room == "room_01_calibration") {
            return std::string("Calibration: follow the service route to Medical");
        }
        if (room == "room_service_medical") {
            return std::string("Medical: choose Security or the elevator route");
        }
        if (room == "room_1f_security") {
            return std::string("Security: observe the guard, then reach the checkpoint");
        }
        if (room == "room_elevator_lobby") {
            return std::string("CHAPTER CHECKPOINT: elevator lobby reached");
        }
        if (room == "room_restroom_staff") {
            return std::string("Staff route: reach the elevator lobby");
        }
        return std::string("Explore the marked service route");
    });
    render->SetInteractionPromptSource([&] {
        const std::string& room = services.player->CurrentRoom();
        const Vec3& position = services.player->Locomotion().position;
        if (room == "room_b1_revival") {
            if (services.systemic->GetDrag(slice.body) != nullptr) {
                const SceneEntity* cart = scene_runtime.FindEntity("b1_cleaning_cart");
                if (cart != nullptr && camera_looks_at(cart->position,
                                                        cart->radius,
                                                        cart->height)) {
                    return std::string("[F] HIDE BODY IN CART");
                }
                return std::string{};
            }
            if (const BodyRecord* body = services.systemic->GetBody(slice.body);
                body != nullptr && body->room == slice.b1_room &&
                body->disposition == BodyDisposition::Exposed &&
                camera_looks_at(body->position, 1.5f, 0.45f)) {
                return body->searched ? std::string("[F] DRAG BODY")
                                      : std::string("[F] SEARCH BODY");
            }
            for (const auto& entity : scene_runtime.Entities()) {
                if (entity.room != room ||
                    !camera_looks_at(entity.position, entity.radius, entity.height)) {
                    continue;
                }
                switch (entity.kind) {
                case SceneEntityKind::Cart:
                    return std::string("[F] INSPECT CLEANING CART");
                case SceneEntityKind::Camera:
                    return std::string("[F] DISABLE CAMERA");
                case SceneEntityKind::Terminal:
                    return slice.badge.IsValid() &&
                                   services.systemic->ItemHeldBy(slice.badge, slice.player)
                               ? std::string("[F] USE TERMINAL")
                               : std::string("[F] USE TERMINAL (BADGE REQUIRED)");
                case SceneEntityKind::DoorReader:
                    return std::string("[F] USE SERVICE READER");
                default:
                    break;
                }
            }
            for (const auto& runtime : services.ai->Npcs()) {
                if (runtime.room == services.world->LoadedRoom().id &&
                    runtime.instance.state != NPCState::Dead &&
                    runtime.instance.state != NPCState::Stunned &&
                    camera_looks_at(runtime.instance.position, 0.42f, 1.8f)) {
                    return runtime.instance.role == Role::Cleaner
                               ? std::string("[F] TALK TO CLEANER")
                               : std::string("[F] TALK");
                }
            }
            if (services.player->CurrentFrame() < 360) {
                return std::string("WASD MOVE | MOUSE LOOK | F INTERACT | LMB FIRE");
            }
            return std::string{};
        }
        if (room == "room_01_calibration") {
            const SceneEntity* terminal = scene_runtime.FindEntity("calibration_terminal");
            if (terminal != nullptr && camera_looks_at(terminal->position,
                                                       terminal->radius,
                                                       terminal->height)) {
                return slice.terminal_session ? std::string("[F] REVIEW CALIBRATION ROUTE")
                                              : std::string("[F] USE CALIBRATION TERMINAL");
            }
            if (FindSceneTransitionAt(scene_runtime, room, position) != nullptr) {
                return std::string("[F] ENTER MEDICAL SERVICE");
            }
            return services.player->CurrentFrame() < 360
                       ? std::string("WASD MOVE | MOUSE LOOK | F INTERACT")
                       : std::string{};
        }
        if (room == "room_1f_security") {
            for (const auto& runtime : services.ai->Npcs()) {
                if (runtime.room == services.world->LoadedRoom().id &&
                    runtime.instance.role == Role::Guard &&
                    runtime.instance.state != NPCState::Dead &&
                    runtime.instance.state != NPCState::Stunned &&
                    camera_looks_at(runtime.instance.position, 0.90f, 1.80f)) {
                    return std::string("[F] ADDRESS SECURITY");
                }
            }
            if (FindSceneTransitionAt(scene_runtime, room, position) != nullptr) {
                return std::string("[F] RETURN TO MEDICAL");
            }
            return std::string{};
        }
        if (room == "room_service_medical") {
            if (FindSceneTransitionAt(scene_runtime, room, position) != nullptr) {
                return position.y >= 10.5f ? std::string("[F] ENTER ELEVATOR LOBBY")
                                           : std::string("[F] ENTER SECURITY");
            }
            return std::string{};
        }
        if (room == "room_restroom_staff" &&
            FindSceneTransitionAt(scene_runtime, room, position) != nullptr) {
            return std::string("[F] ENTER ELEVATOR LOBBY");
        }
        return std::string{};
    });
    services.player->SetInteractCallback([&] {
        const Vec3& p = services.player->Locomotion().position;
        const uint64_t frame = services.player->CurrentFrame();
        const EntityId player = slice.player;
        NpcId interaction_guard_npc = slice.guard_npc;
        Vec3 interaction_guard_position{};
        bool interaction_guard_available = false;
        if (services.player->CurrentRoom() == "room_1f_security") {
            for (const auto& runtime : services.ai->Npcs()) {
                if (runtime.room == services.world->LoadedRoom().id &&
                    runtime.instance.role == Role::Guard) {
                    interaction_guard_npc = runtime.instance.id;
                    interaction_guard_position = runtime.instance.position;
                    interaction_guard_available = true;
                    break;
                }
            }
        }
        const EntityId guard = EntityId::New(interaction_guard_npc.GetValue());

        if (services.player->CurrentRoom() == "room_b1_revival") {
            enum class B1TargetKind : uint8_t {
                None, Body, Cart, Camera, Terminal, Gate, Npc
            };
            struct B1Target {
                B1TargetKind kind = B1TargetKind::None;
                Vec3 position;
                float radius = 0.0f;
                float height = 1.8f;
                NpcId npc;
            };
            B1Target focused;
            float focused_distance = 1000000.0f;
            const LocomotionState& locomotion = services.player->Locomotion();
            const Vec3 eye = locomotion.EyePosition();
            const bool body_drag_active =
                services.systemic->GetDrag(slice.body) != nullptr;
            const float focal = 0.5f * static_cast<float>(terminal_h) /
                                std::tan(settings.fov * 3.14159265f / 360.0f);
            const CameraProjection camera(eye, locomotion.yaw, locomotion.pitch,
                                          terminal_w, terminal_h,
                                          focal, kCharacterCellAspect);
            const Vec3 interaction_ray = camera.RayDirectionAt(
                static_cast<float>(terminal_w) * 0.5f,
                static_cast<float>(terminal_h) * 0.5f);
            const auto consider = [&](B1Target candidate) {
                // While a body is being dragged, the cart is the only valid
                // next interaction target. This is a semantic priority rule,
                // not a larger NPC halo: the player is completing the active
                // drag transaction and must not accidentally select a nearby
                // cleaner or reader.
                if (body_drag_active && candidate.kind != B1TargetKind::Cart) {
                    return;
                }
                const AABB bounds{
                    Vec3{candidate.position.x - candidate.radius,
                         candidate.position.y - candidate.radius,
                         candidate.position.z},
                     Vec3{candidate.position.x + candidate.radius,
                          candidate.position.y + candidate.radius,
                          candidate.position.z + candidate.height}};
                float hit_distance = 0.0f;
                const bool hit = IntersectRayAabb(eye, interaction_ray, bounds,
                                                  hit_distance);
                if (!hit || hit_distance >= focused_distance) {
                    return;
                }
                const Vec3 hit_point = eye + interaction_ray * hit_distance;
                // Every B1 target, including the reader, is a real visible
                // world target.  An opaque wall/door therefore blocks the
                // interaction ray instead of a radius/cone selecting it.
                if (!services.world->Query().LineOfSight(
                    eye, hit_point, hit_point.z)) return;
                focused = candidate;
                focused_distance = hit_distance;
            };
            if (services.systemic->GetDrag(slice.body) == nullptr) {
                if (const BodyRecord* body = services.systemic->GetBody(slice.body)) {
                    if (body->room == slice.b1_room &&
                        body->disposition == BodyDisposition::Exposed) {
                        consider(B1Target{B1TargetKind::Body, body->position, 1.5f,
                                          0.45f, {}});
                    }
                }
            }
            for (const auto& entity : scene_runtime.Entities()) {
                if (entity.room != "room_b1_revival") continue;
                    B1TargetKind kind = B1TargetKind::None;
                    switch (entity.kind) {
                    case SceneEntityKind::Cart: kind = B1TargetKind::Cart; break;
                    case SceneEntityKind::Camera: kind = B1TargetKind::Camera; break;
                    case SceneEntityKind::Terminal: kind = B1TargetKind::Terminal; break;
                    case SceneEntityKind::DoorReader:
                        kind = B1TargetKind::Gate;
                        break;
                    default: break;
                    }
                    if (kind != B1TargetKind::None) {
                        consider(B1Target{kind, entity.position, entity.radius,
                                          entity.height, {}});
                    }
            }
            for (const auto& runtime : services.ai->Npcs()) {
                if (runtime.room == slice.b1_room &&
                    runtime.instance.state != NPCState::Dead &&
                    runtime.instance.state != NPCState::Stunned) {
                    // Use the authored standing actor footprint for targeting;
                    // the old interaction halo could swallow nearby props
                    // and made a guard at the cart win every ray at distance 0.
                    consider(B1Target{B1TargetKind::Npc, runtime.instance.position, 0.42f,
                                      1.8f, runtime.instance.id});
                }
            }
            const auto player_holds_valid_badge = [&] {
                return slice.badge.IsValid() &&
                       services.systemic->ItemHeldBy(slice.badge, player) &&
                       services.systemic->ReaderAcceptsItem(slice.badge, 2);
            };
            const auto terminal_session_is_active = [&] {
                for (const auto& session : services.systemic->TerminalSessions()) {
                    if (session.terminal == slice.terminal && session.user == player &&
                        session.active) return true;
                }
                return false;
            };

            if (focused.kind == B1TargetKind::Gate) {
                slice.access_attempted = true;
                if (!player_holds_valid_badge()) {
                    slice.access_denied = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_b1_gate_denied"), player, true);
                    render->SetSubtitleOnce("ACCESS DENIED. The reader needs your held badge.", 180);
                } else if (!services.world->B1GateOpen()) {
                    (void)services.world->UnlockB1Gate();
                    if (services.world->OpenB1Gate() || services.world->B1GateOpen()) {
                        slice.gate_unlocked = true;
                        slice.gate_open = true;
                        events.Post(EventDoorChange{services.world->B1GateId(), true},
                                    EventKind::Mutation, player, EntityId::Invalid(),
                                    EventId::Invalid(), frame);
                        render->SetSubtitleOnce("ACCESS GRANTED. B1 service door unlocked.", 180);
                    }
                } else if (const SceneEntity* door =
                               scene_runtime.FindEntity("b1_service_door");
                           door != nullptr && p.x > door->position.x + 0.4f) {
                    const SceneTransition* link =
                        scene_runtime.FindTransition("b1_to_calibration");
                    if (link != nullptr &&
                        switch_room(link->destination_room, link->destination_spawn,
                                    link->destination_yaw)) {
                        slice.gate_crossed = true;
                        services.world->SetBooleanFact(
                            RuntimeFactId("fact_r1_checkpoint_reached"),
                            player, true);
                        services.world->SetBooleanFact(
                            RuntimeFactId("fact_b1_checkpoint_reached"),
                            player, true);
                        (void)services.systemic->TransitionQuest(
                            slice.opening_quest, QuestStatus::Completed, frame,
                            "B1 service checkpoint reached");
                    } else {
                        render->SetSubtitleOnce("Calibration room unavailable.", 120);
                    }
                } else {
                    render->SetSubtitleOnce("B1 service door is open. Move through the reader.", 140);
                }
                return;
            }
            if (focused.kind == B1TargetKind::Body) {
                const BodyRecord* body = services.systemic->GetBody(slice.body);
                if (body != nullptr && body->disposition == BodyDisposition::Exposed) {
                    if (!body->searched) {
                        SearchAction search;
                        search.actor = player;
                        search.target = slice.body;
                        search.target_type = SearchTargetType::Body;
                        search.consent = false;
                        search.room = slice.b1_room;
                        search.frame = frame;
                        const SearchOutcome outcome = services.systemic->PerformSearch(search);
                        bool took_badge = false;
                        for (const auto item_id : outcome.items_revealed) {
                            if (item_id == slice.badge &&
                                services.systemic->TheftItem(item_id, player, frame)) {
                                took_badge = true;
                                services.world->SetBooleanFact(
                                    RuntimeFactId("fact_b1_badge_obtained"),
                                    player, true);
                            }
                        }
                        render->SetSubtitleOnce(took_badge ? "Search complete. Badge taken." :
                                                   "Search complete. Nothing useful found.", 150);
                    } else if (services.systemic->BeginDrag(player, slice.body, frame)) {
                        render->SetSubtitleOnce("Body secured. Drag to the cleaning cart.", 150);
                    }
                }
                return;
            }
            if (focused.kind == B1TargetKind::Cart) {
                if (services.systemic->GetDrag(slice.body) != nullptr) {
                    const bool ended = services.systemic->EndDrag(slice.body, frame);
                    const bool hidden = ended && services.systemic->HideBody(slice.body, slice.cart, frame);
                    if (hidden) {
                        slice.body_hidden = true;
                        services.world->SetBooleanFact(
                            RuntimeFactId("fact_b1_body_hidden"), player, true);
                        (void)services.ai->ArmBodyDiscovery(frame + 240);
                        render->SetSubtitleOnce("Cart latched. The corridor is quiet again.", 180);
                    }
                } else {
                    render->SetSubtitleOnce("Cleaning cart: empty and unlocked.", 120);
                }
                return;
            }
            if (focused.kind == B1TargetKind::Camera) {
                const ObservationSource* camera_source =
                    services.systemic->GetObservationSource(slice.camera);
                if (camera_source != nullptr && camera_source->online) {
                    services.systemic->SetObservationSourceOnline(slice.camera, false);
                    SystemicEvent outage;
                    outage.id = EventId::New(10000 + services.systemic->EventCount());
                    outage.type = SystemicEventType::Vandalism;
                    outage.actor = player;
                    outage.location = slice.b1_room;
                    outage.frame = frame;
                    outage.legality = LegalityClass::Illegal;
                    outage.outcome = OutcomeType::Success;
                    outage.method = "camera_disable";
                    outage.tags.push_back("camera_outage");
                    services.systemic->AddSystemicEvent(outage);
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_b1_camera_disabled"), player, true);
                    render->SetSubtitleOnce("Camera offline. The blind spot is temporary.", 150);
                } else {
                    render->SetSubtitleOnce("Camera: offline.", 100);
                }
                return;
            }
            if (focused.kind == B1TargetKind::Terminal) {
                slice.terminal_attempted = true;
                slice.terminal_session = terminal_session_is_active();
                if (!slice.terminal_session && player_holds_valid_badge()) {
                    TerminalSession session;
                    session.terminal = slice.terminal;
                    session.user = player;
                    session.method = TerminalAccessMethod::Credential;
                    session.started_frame = frame;
                    session.active = true;
                    TerminalAuditLog audit;
                    audit.terminal = slice.terminal;
                    audit.user = player;
                    audit.method = TerminalAccessMethod::Credential;
                    audit.frame = frame;
                    audit.action = "read_calibration_route";
                    audit.unauthorized = false;
                    if (services.systemic->AddTerminalSession(session) &&
                        services.systemic->AddTerminalAudit(audit)) {
                        slice.terminal_session = true;
                        services.world->SetBooleanFact(
                            RuntimeFactId("fact_b1_terminal_accessed"),
                            player, true);
                        services.systemic->PlayerState().truth_exposure =
                            std::min(1.0f, services.systemic->PlayerState().truth_exposure + 0.08f);
                        render->SetSubtitleOnce("TERMINAL: calibration route unlocked.", 180);
                    }
                } else if (slice.terminal_session) {
                    render->SetSubtitleOnce("TERMINAL: session active. Route copied.", 120);
                } else {
                    slice.terminal_denied = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_b1_gate_denied"), player, true);
                    render->SetSubtitleOnce("TERMINAL: credential required.", 150);
                }
                return;
            }
            if (focused.kind == B1TargetKind::Npc) {
                if (focused.npc == slice.cleaner_npc) {
                    // This is the smallest durable relationship hook for the
                    // Recovery-04 delayed-discovery proof.  The later cleaner
                    // decision reads this directed record after save/load;
                    // it is not a replay-only flag.
                    RelationshipRecord relationship;
                    relationship.a = EntityId::New(slice.cleaner_npc.GetValue());
                    relationship.b = player;
                    relationship.trust = 0.70f;
                    relationship.debt = 0.60f;
                    if (services.systemic->SetRelationship(relationship)) {
                        render->SetSubtitleOnce(
                            "The worker recognizes you. The cart route is clear.",
                            150);
                    }
                } else {
                    render->SetSubtitleOnce(
                        "The worker watches the route. Your next action will matter.",
                        120);
                }
                return;
            }
            render->SetSubtitleOnce("B1: maintenance, camera loop, terminal, or checkpoint.", 100);
            return;
        }

        if (services.player->CurrentRoom() == "room_01_calibration") {
            if (try_room_portal(p)) {
                return;
            }
            const SceneEntity* calibration_terminal =
                scene_runtime.FindEntity("calibration_terminal");
            if (calibration_terminal != nullptr &&
                camera_looks_at(calibration_terminal->position,
                                calibration_terminal->radius,
                                calibration_terminal->height)) {
                render->SetSubtitleOnce(slice.terminal_session
                                            ? "CALIBRATION: route verified. Service access is open."
                                            : "CALIBRATION: use the B1 terminal before proceeding.",
                                        160);
                return;
            }
            render->SetSubtitleOnce("CALIBRATION: diagnostics, logistics, then medical service.", 120);
            return;
        }

        if (services.player->CurrentRoom() == "room_1f_security") {
            const bool has_badge = slice.badge.IsValid() &&
                services.systemic->ItemHeldBy(slice.badge, player) &&
                services.systemic->ReaderAcceptsItem(slice.badge, 2);
            if (interaction_guard_available &&
                camera_looks_at(interaction_guard_position, 0.90f, 1.80f)) {
                if (has_badge) {
                    render->SetSubtitleOnce("ACCESS GRANTED. Security checkpoint logged you.", 180);
                } else if (!slice.bribe_done && slice.cash.IsValid() &&
                           services.systemic->TransferItem(slice.cash, guard)) {
                    SocialExchangeRecord exchange;
                    exchange.id = SocialExchangeId::New(10000 + services.systemic->SocialExchangeCount());
                    exchange.type = SocialExchangeType::Bribe;
                    exchange.actor = player;
                    exchange.target = guard;
                    exchange.cash = 50;
                    exchange.outcome = SocialExchangeOutcome::AcceptedThenMayReport;
                    exchange.frame = frame;
                    exchange.risk_context = "checkpoint witness";
                    if (services.systemic->AddSocialExchange(exchange)) {
                        slice.bribe_done = true;
                        services.world->SetBooleanFact(
                            RuntimeFactId("fact_alpha_bribe_accepted"),
                            player, true);
                        render->SetSubtitleOnce("He takes the money. That is not the same as trust.", 180);
                    }
                } else if (slice.bribe_done) {
                    render->SetSubtitleOnce("Checkpoint: the guard remembers the exchange.", 150);
                } else {
                    render->SetSubtitleOnce("ACCESS DENIED. No valid credential.", 180);
                }
                return;
            }
            if (try_room_portal(p)) {
                return;
            }
            if (p.x < 8.0f && p.y < 5.0f) {
                if (!slice.schedule_found) {
                    KnowledgeAssetRecord asset;
                    asset.id = KnowledgeAssetId::New(10000 + services.systemic->KnowledgeCount());
                    asset.type = KnowledgeAssetType::ShiftSchedule;
                    asset.source = ResourceId::New(1);
                    asset.confidence = 0.7f;
                    asset.known_by.push_back(player);
                    if (services.systemic->AddKnowledgeAsset(asset)) slice.schedule_found = true;
                }
                render->SetSubtitleOnce("STAFF ROUTE: maintenance shift change at 02:10.", 180);
                return;
            }
            render->SetSubtitleOnce("1F: checkpoint, staff route, or maintenance access.", 100);
            return;
        }

        if (services.player->CurrentRoom() == "room_service_medical") {
            if (!try_room_portal(p)) {
                render->SetSubtitleOnce("MEDICAL: supplies are logged before they are used.", 160);
            }
            return;
        }

        if (services.player->CurrentRoom() == "room_restroom_staff") {
            if (!try_room_portal(p)) {
                render->SetSubtitleOnce("STAFF ROUTE: a service door stands open.", 140);
            }
            return;
        }

        if (services.player->CurrentRoom() == "room_elevator_lobby") {
            render->SetSubtitleOnce("ELEVATOR: floors 2–36 require a reason to exist.", 180);
        }
    });
    services.player->SetDragStateCallback([&](float& modifier, bool& sprint_forbidden,
                                               bool& weapon_restricted) {
        const BodyDragRecord* drag = services.systemic->GetDrag(slice.body);
        if (drag == nullptr) return false;
        modifier = drag->movement_modifier;
        sprint_forbidden = drag->sprint_forbidden;
        weapon_restricted = drag->weapon_restricted;
        return true;
    });
    services.player->SetDragUpdateCallback([&](const Vec3& position,
                                               const std::string& room, uint64_t frame) {
        if (room == "room_b1_revival" && services.world->HasLoadedRoom()) {
            services.systemic->UpdateDrag(slice.body, position,
                                          services.world->LoadedRoom().id, frame);
        }
    });
    services.player->SetPauseCallback([&] {
        render->SetSubtitleOnce(services.player->Paused() ? "PAUSED" : "RESUMED", 90);
    });
    services.player->SetMeleeCallback([&] {
        // The bounded stunner action is the third weapon slot, not a hidden
        // melee side channel.  Consume the same magazine/cooldown and emit
        // the same typed fire event used by the other selectable slots so
        // HUD, viewmodel, hearing, and hit resolution agree.
        // A melee shortcut is still a slot transition.  Cancel an in-flight
        // reload before changing slots, otherwise AdvanceReload() would use
        // the newly selected stunner magazine to complete a pistol reload.
        services.player->Combat().reload_frames_left = 0;
        services.player->Combat().slot = WeaponSlot::Stunner;
        const WeaponDef& weapon =
            DefaultWeapons()[static_cast<size_t>(WeaponSlot::Stunner)];
        const uint64_t frame = services.player->CurrentFrame();
        if (!ConsumeShot(services.player->Combat(), weapon,
                         static_cast<uint32_t>(frame))) {
            render->SetSubtitleOnce("Stunner: empty or cooling down.", 100);
            return;
        }
        FireRequest request;
        request.origin = services.player->Locomotion().EyePosition();
        request.yaw = services.player->Locomotion().yaw;
        request.pitch = services.player->Locomotion().pitch;
        request.slot = WeaponSlot::Stunner;
        request.spread_factor = 0.0f;
        events.Post(EventWeaponFire{slice.player, WeaponSlot::Stunner,
                                    request.origin, request.yaw, request.pitch,
                                    weapon.loudness},
                    EventKind::Notification, slice.player, EntityId::Invalid(),
                    EventId::Invalid(), frame);
        const ShotFeedback feedback = services.ai->HandlePlayerShot(
            request, weapon, frame);
        slice.shot_hit = slice.shot_hit || feedback.target_was_npc;
        slice.nonlethal_hit = slice.nonlethal_hit || feedback.target_stunned;
        // Stunner is a real selectable weapon slot in this bounded slice.
        // Keep the authored recovery fact aligned with the successful weapon
        // action, regardless of whether the action arrived through the
        // keyboard melee binding or the normal fire binding.
        services.world->SetBooleanFact(RuntimeFactId("fact_player_has_gun"),
                                        slice.player, true);
        services.world->SetBooleanFact(RuntimeFactId("fact_b1_nonlethal_action"),
                                        slice.player, true);
        if (!feedback.target_was_npc) {
            render->SetSubtitleOnce("Stunner: target out of range or line of fire.", 100);
        }
    });
    if (services.world->HasLoadedRoom()) {
        const Room& room = services.world->LoadedRoom();
        render->SetGridData(room.grid.Data().data(),
                            room.grid.Width(), room.grid.Height());
        render->SetActiveRoom(room.id);
    }
    engine.SetRenderModule(render.get());

    const int result = engine.Run(config.max_frames);
    events.Unregister(player_damage_consumer);
    events.Unregister(speech_consumer);

    // Discovery is an event-ledger fact, not an inference from the cleaner's
    // loop count.  Derive the slice receipts from the events that actually
    // reached the systemic kernel so replay output cannot certify a scripted
    // timer or a mere process exit.
    for (const auto& event : services.systemic->SystemEvents()) {
        if (event.type == SystemicEventType::BodyDiscovered &&
            event.target == EntityId::New(slice.guard_npc.GetValue())) {
            slice.body_discovered = true;
        }
        if ((event.type == SystemicEventType::Report ||
             event.type == SystemicEventType::MedicalCall ||
             event.type == SystemicEventType::HelpCoverUp) &&
            event.actor == EntityId::New(slice.cleaner_npc.GetValue())) {
            slice.cleaner_response = true;
        }
    }

    if (!config.frame_dump_path.empty() && !render->Cells().empty()) {
        if (!WriteCharacterFrameSvg(render->Cells().data(), terminal_w,
                                    terminal_h, config.frame_dump_path)) {
            std::fprintf(stderr, "warning: unable to write character frame dump\n");
        }
    }

    if (!config.replay_path.empty()) {
        size_t full_npcs = 0;
        size_t semi_npcs = 0;
        const size_t replay_keyboard_events = input_module->ReplayKeyboardEventCount();
        const size_t replay_mouse_events = input_module->ReplayMouseEventCount();
        const bool replay_process_ok = result == 0;
        const bool replay_input_consumed = replay_keyboard_events > 0 || replay_mouse_events > 0;
        const bool chapter_checkpoint_reached = slice.gate_crossed &&
                                                services.player->CurrentRoom() ==
                                                    "room_01_calibration";
        const bool success_replay = config.replay_path.find("recovery_b1_success") !=
                                    std::string::npos;
        const bool denied_replay = config.replay_path.find("recovery_b1_denied") !=
                                   std::string::npos;
        const bool terminal_denied_replay =
            config.replay_path.find("recovery_b1_terminal_denied") !=
            std::string::npos;
        const bool health_death_replay =
            config.replay_path.find("recovery_b1_health_death") !=
            std::string::npos;
        const bool badge_only_replay =
            config.replay_path.find("recovery_b1_badge_only") !=
            std::string::npos;
        const bool alpha_systemic_replay =
            config.replay_path.find("alpha01_systemic_success") !=
            std::string::npos;
        const bool alpha_aggressive_replay =
            config.replay_path.find("alpha01_aggressive_success") !=
            std::string::npos;
        const bool alpha_denied_replay =
            config.replay_path.find("alpha01_denied") != std::string::npos;
        const bool alpha_memory_replay =
            config.replay_path.find("alpha01_memory_consequence") !=
            std::string::npos;
        const bool badge_held_by_player =
            slice.badge.IsValid() &&
            services.systemic->ItemHeldBy(slice.badge, slice.player);
        const RelationshipRecord* cleaner_relationship =
            services.systemic->GetRelationship(
                EntityId::New(slice.cleaner_npc.GetValue()), slice.player);
        bool cleaner_helped_coverup = false;
        for (const auto& event : services.systemic->SystemEvents()) {
            if (event.type == SystemicEventType::HelpCoverUp &&
                event.actor == EntityId::New(slice.cleaner_npc.GetValue())) {
                cleaner_helped_coverup = true;
                break;
            }
        }
        const bool cleaner_relationship_established =
            cleaner_relationship != nullptr &&
            cleaner_relationship->trust > 0.55f &&
            cleaner_relationship->debt > 0.35f;
        const bool narrative_visible_action =
            services.narrative->PresentedActionCount() > 0 &&
            !services.narrative->LastPresentedText().empty();
        const bool quest_presented_during_route = render->ObjectiveWasPresented();
        const bool quest_presentation_visible = render->ObjectiveVisible();
        const bool expected_state_reached = (success_replay || alpha_systemic_replay)
            ? (slice.shot_hit && slice.nonlethal_hit && slice.body_created &&
               slice.body_hidden && slice.body_discovered && slice.cleaner_response &&
               slice.terminal_session && slice.gate_open &&
               narrative_visible_action && quest_presented_during_route &&
               replay_save_ok && replay_load_ok && chapter_checkpoint_reached)
            : alpha_aggressive_replay
                ? (slice.shot_hit && !slice.nonlethal_hit && slice.body_created &&
                   badge_held_by_player && slice.gate_open && slice.gate_crossed &&
                   narrative_visible_action && quest_presented_during_route &&
                   chapter_checkpoint_reached)
            : alpha_denied_replay
                ? (slice.access_attempted && slice.access_denied &&
                   !slice.gate_open && !slice.gate_crossed &&
                   !slice.terminal_session)
            : alpha_memory_replay
                ? (slice.body_hidden && slice.body_discovered &&
                   slice.cleaner_response && cleaner_relationship_established &&
                   cleaner_helped_coverup && replay_save_ok && replay_load_ok)
            : denied_replay
                ? (slice.access_attempted && slice.access_denied && !slice.gate_open &&
                   !slice.terminal_session && !chapter_checkpoint_reached)
                : terminal_denied_replay
                    ? (slice.terminal_attempted && slice.terminal_denied &&
                       !slice.terminal_session && !slice.gate_open)
                    : health_death_replay
                        ? (slice.player_died && slice.player_recovered &&
                           replay_save_ok && replay_load_ok &&
                           services.player->Health() > 0 &&
                           !services.player->Dead())
                        : badge_only_replay
                            ? (badge_held_by_player && !slice.access_attempted &&
                               !slice.access_denied && !slice.gate_open &&
                               !chapter_checkpoint_reached)
                            : false;
        for (const auto& runtime : services.ai->Npcs()) {
            if (runtime.instance.cognition == CognitionTier::Full) {
                ++full_npcs;
            } else if (runtime.instance.cognition == CognitionTier::SemiHuman) {
                ++semi_npcs;
            }
        }
        std::fprintf(stderr, "REPLAY_PROCESS_EXIT_OK=%s\n",
                     replay_process_ok ? "YES" : "NO");
        std::fprintf(stderr, "REPLAY_INPUT_CONSUMED=%s KEYBOARD_EVENTS=%zu MOUSE_EVENTS=%zu\n",
                     replay_input_consumed ? "YES" : "NO",
                     replay_keyboard_events, replay_mouse_events);
        std::fprintf(stderr, "REPLAY_EXPECTED_STATE_REACHED=%s\n",
                     expected_state_reached ? "YES" : "NO");
        std::fprintf(stderr, "CHAPTER_CHECKPOINT_REACHED=%s\n",
                     chapter_checkpoint_reached ? "YES" : "NO");
        std::fprintf(stderr, "REPLAY_RESULT=%s\n",
                     replay_process_ok && replay_input_consumed && expected_state_reached
                         ? "PASS" : "FAIL");
        std::fprintf(stderr, "REPLAY_EXIT_CODE=%d\n", result);
        std::fprintf(stderr, "REPLAY_ROUTE=");
        for (size_t i = 0; i < replay_route.size(); ++i) {
            if (i != 0) std::fputc('>', stderr);
            std::fputs(replay_route[i].c_str(), stderr);
        }
        std::fputc('\n', stderr);
        std::fprintf(stderr,
                     "NPC_COUNT=%zu FULL_NPC_COUNT=%zu SEMI_NPC_COUNT=%zu "
                     "AUTONOMOUS_LOOPS=%zu DISCOVERY_RESPONSES=%zu "
                     "SAVE_ATTEMPTED=%s SAVE_OK=%s LOAD_ATTEMPTED=%s LOAD_OK=%s "
                     "EVENT_JOURNAL=%zu SYSTEMIC_EVENTS=%zu MEMORIES=%zu\n",
                     services.ai->Npcs().size(), full_npcs, semi_npcs,
                     services.ai->AutonomousLoopCount(),
                     services.ai->DiscoveryResponseCount(),
                     replay_save_attempted ? "YES" : "NO",
                     replay_save_ok ? "YES" : "NO",
                     replay_load_attempted ? "YES" : "NO",
                     replay_load_ok ? "YES" : "NO",
                     events.JournalCount(), services.systemic->EventCount(),
                     services.systemic->MemoryCount());
        std::fprintf(stderr,
                     "SLICE_SHOT_HIT=%s NONLETHAL_HIT=%s BODY_CREATED=%s "
                     "BODY_HIDDEN=%s BODY_DISCOVERED=%s ACCESS_ATTEMPTED=%s "
                     "ACCESS_DENIED=%s GATE_OPEN=%s GATE_CROSSED=%s "
                     "PLAYER_HEALTH=%u PLAYER_DEAD=%s\n",
                     slice.shot_hit ? "YES" : "NO",
                     slice.nonlethal_hit ? "YES" : "NO",
                     slice.body_created ? "YES" : "NO",
                     slice.body_hidden ? "YES" : "NO",
                     slice.body_discovered ? "YES" : "NO",
                     slice.access_attempted ? "YES" : "NO",
                     slice.access_denied ? "YES" : "NO",
                     slice.gate_open ? "YES" : "NO",
                     slice.gate_crossed ? "YES" : "NO",
                     static_cast<unsigned>(services.player->Health()),
                     services.player->Dead() ? "YES" : "NO");
        std::fprintf(stderr, "BADGE_HELD_BY_PLAYER=%s\n",
                     badge_held_by_player ? "YES" : "NO");
        std::fprintf(stderr, "CLEANER_RELATIONSHIP=%s CLEANER_HELP_COVERUP=%s\n",
                     cleaner_relationship_established ? "YES" : "NO",
                     cleaner_helped_coverup ? "YES" : "NO");
        for (const auto& runtime : services.ai->Npcs()) {
            if (runtime.instance.id == slice.cleaner_npc ||
                runtime.instance.id == slice.guard_npc) {
                std::fprintf(stderr, "NPC_RUNTIME id_%llu room_%llu state_%u "
                                    "pos_%.3f_%.3f health_%u\n",
                             static_cast<unsigned long long>(runtime.instance.id.GetValue()),
                             static_cast<unsigned long long>(runtime.room.GetValue()),
                             static_cast<unsigned>(runtime.instance.state),
                             runtime.instance.position.x, runtime.instance.position.y,
                             static_cast<unsigned>(runtime.instance.health));
            }
        }
        std::fprintf(stderr, "TERMINAL_ATTEMPTED=%s TERMINAL_DENIED=%s "
                            "CLEANER_RESPONSE=%s PLAYER_DIED=%s "
                            "PLAYER_RECOVERED=%s\n",
                     slice.terminal_attempted ? "YES" : "NO",
                     slice.terminal_denied ? "YES" : "NO",
                     slice.cleaner_response ? "YES" : "NO",
                     slice.player_died ? "YES" : "NO",
                     slice.player_recovered ? "YES" : "NO");
        std::fprintf(stderr, "NARRATOR_TYPOGRAPHY_ACTIVE=%s\n",
                     render->NarratorTypographyActive() ? "YES" : "NO");
        std::fprintf(stderr, "NARRATIVE_VISIBLE_ACTION=%s\n",
                     narrative_visible_action ? "YES" : "NO");
        std::fprintf(stderr, "QUEST_PRESENTED_DURING_ROUTE=%s\n",
                     quest_presented_during_route ? "YES" : "NO");
        std::fprintf(stderr, "QUEST_PRESENTATION_VISIBLE=%s OBJECTIVE=%s\n",
                     quest_presentation_visible ? "YES" : "NO",
                     render->ObjectiveText().c_str());
        const BodyRecord* body = services.systemic->GetBody(slice.body);
        if (body != nullptr) {
            const ItemRecord* badge = slice.badge.IsValid()
                                          ? services.systemic->GetItem(slice.badge)
                                          : nullptr;
            std::fprintf(stderr,
                         "BODY_STATE=disposition_%u drag_%u searched_%s "
                         "BADGE_HOLDER=%llu TERMINAL_SESSION=%s\n",
                         static_cast<unsigned>(body->disposition),
                         static_cast<unsigned>(body->drag_status),
                         body->searched ? "YES" : "NO",
                         static_cast<unsigned long long>(
                         badge != nullptr && badge->current_holder.IsValid()
                                 ? badge->current_holder.GetValue()
                                 : 0),
                         slice.terminal_session ? "YES" : "NO");
        }
        const Vec3& player_position = services.player->Locomotion().position;
        std::fprintf(stderr, "PLAYER_STATE=room_%s pos_%.3f_%.3f_%.3f\n",
                     services.player->CurrentRoom().c_str(), player_position.x,
                     player_position.y, player_position.z);
    }

    if (config.smoke && config.save_after_smoke) {
        // Smoke save: real determinism sections through the atomic writer.
        // Includes player locomotion, world facts/infra, narrative storylet
        // runtime, RNG, and event journal (F-15 closure).
        std::vector<SaveSection> sections;
        std::vector<uint8_t> rng_bytes, events_bytes, player_bytes,
            world_bytes, ai_bytes, narrative_bytes, systemic_bytes;
        {
            Serializer s(rng_bytes);
            sim_rng.Save(s);
        }
        {
            Serializer s(events_bytes);
            events.Save(s);
        }
        {
            player_bytes = serialize_player();
        }
        {
            Serializer s(world_bytes);
            services.world->SaveState(s);
        }
        {
            Serializer s(ai_bytes);
            services.ai->Save(s);
        }
        {
            Serializer s(narrative_bytes);
            services.narrative->SaveState(s);
        }
        sections.push_back({SaveSectionId::Player, std::move(player_bytes)});
        sections.push_back({SaveSectionId::World, std::move(world_bytes)});
        sections.push_back({SaveSectionId::Rng, std::move(rng_bytes)});
        sections.push_back({SaveSectionId::Events, std::move(events_bytes)});
        sections.push_back({SaveSectionId::Ai, std::move(ai_bytes)});
        sections.push_back({SaveSectionId::Narrative, std::move(narrative_bytes)});
        systemic_bytes = services.systemic->Serialize();
        sections.push_back({SaveSectionId::Systemic, std::move(systemic_bytes)});
        // Ensure the runtime saves directory exists (best-effort).
        std::error_code ec;
        const std::filesystem::path save_dir = user_data_root / "saves";
        std::filesystem::create_directories(save_dir, ec);
        if (ec) {
            std::fprintf(stderr, "smoke save setup failed: user data unavailable\n");
            return 3;
        }
        SaveManager save;
        const std::string save_path = (save_dir / "smoke").string();
        const auto res = save.SaveWorld(save_path, sections);
        if (res.IsError()) {
            std::fprintf(stderr,
                         "smoke save write failed (real error): %s\n",
                         res.Error().message.c_str());
            return 3;
        }

        // Runtime save/load proof: load the real saved file, find the
        // Systemic section, and restore into a fresh SystemicWorld.
        const auto loaded_save = save.LoadWorld(save_path);
        if (loaded_save.IsError()) {
            std::fprintf(stderr, "smoke save reload failed: %s\n",
                         loaded_save.Error().message.c_str());
            return 4;
        }
        bool found_player = false;
        bool found_world = false;
        bool found_rng = false;
        bool found_events = false;
        bool found_ai = false;
        bool found_narrative = false;
        bool found_systemic = false;
        for (const auto& sec : loaded_save.Value()) {
            if (sec.id == SaveSectionId::Player) found_player = true;
            if (sec.id == SaveSectionId::World) {
                found_world = true;
                Deserializer d(sec.data.data(), sec.data.size());
                WorldModule restored_world;
                if (!restored_world.LoadState(d) || d.HasError() || !d.AtEnd()) {
                    std::fprintf(stderr, "world section restore failed\n");
                    return 5;
                }
            }
            if (sec.id == SaveSectionId::Rng) {
                found_rng = true;
                Deserializer d(sec.data.data(), sec.data.size());
                DeterministicRNG restored_rng;
                restored_rng.Load(d);
                if (d.HasError() || !d.AtEnd() ||
                    (restored_rng.GetState0() == 0 && restored_rng.GetState1() == 0)) {
                    std::fprintf(stderr, "rng section restore failed\n");
                    return 5;
                }
            }
            if (sec.id == SaveSectionId::Events) {
                found_events = true;
                Deserializer d(sec.data.data(), sec.data.size());
                EventBus restored_events;
                restored_events.Load(d);
                if (d.HasError() || !d.AtEnd()) {
                    std::fprintf(stderr, "event section restore failed\n");
                    return 5;
                }
            }
            if (sec.id == SaveSectionId::Ai) {
                found_ai = true;
                Deserializer d(sec.data.data(), sec.data.size());
                AutonomousNpcSystem restored_ai;
                restored_ai.Attach(services.systemic.get(), &events, &sim_rng);
                for (const auto& runtime : services.ai->Npcs()) {
                    if (!restored_ai.AddNpc(runtime.instance, runtime.room)) {
                        std::fprintf(stderr, "ai section setup failed\n");
                        return 5;
                    }
                }
                if (!restored_ai.Load(d) || d.HasError() || !d.AtEnd()) {
                    std::fprintf(stderr, "ai section restore failed\n");
                    return 5;
                }
            }
            if (sec.id == SaveSectionId::Narrative) {
                found_narrative = true;
                Deserializer d(sec.data.data(), sec.data.size());
                NarrativeModule restored_narrative;
                if (!restored_narrative.LoadState(d) || d.HasError() || !d.AtEnd()) {
                    std::fprintf(stderr, "narrative section restore failed\n");
                    return 5;
                }
            }
            if (sec.id == SaveSectionId::Systemic) {
                found_systemic = true;
                const auto restored = SystemicWorld::Deserialize(
                    sec.data.data(), sec.data.size());
                if (restored.IsError()) {
                    std::fprintf(stderr, "systemic section restore failed\n");
                    return 5;
                }
                const std::vector<uint8_t> restored_bytes = restored.Value().Serialize();
                if (restored_bytes != sec.data) {
                    std::fprintf(stderr, "systemic save/load byte mismatch\n");
                    return 6;
                }
            }
        }
        if (!found_player || !found_world || !found_rng || !found_events ||
            !found_ai || !found_narrative || !found_systemic) {
            std::fprintf(stderr, "smoke save section missing\n");
            return 7;
        }
    }
    events.Unregister(audio_consumer);
    if (audio) audio->Shutdown();
    return result;
}

} // namespace writeover
