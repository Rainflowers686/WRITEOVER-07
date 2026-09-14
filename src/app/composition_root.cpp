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
#include "src/app/tower_campaign_runtime.h"
#include "src/app/campaign_panel.h"
#include "src/app/player_product.h"
#include "src/app/presentation_pulse.h"
#include "src/app/product_save.h"
#include "src/app/player_save.h"
#include "src/app/terminal_surface.h"
#include "src/app/runtime_time_gate.h"
#include "src/app/presentation_text.h"
#include "src/app/player_perception.h"
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

// A small downward bias keeps the first-person weapon, route markings and
// functional room groupings in the opening read.  The player can still look
// freely; this only establishes a grounded production composition when a room
// is entered or restarted.
constexpr float kOpeningPitch = -0.08f;

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
    bool MovementDemonstrated() const { return movement_demonstrated_; }

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
        const bool surface_ready = !surface_ready_source_ || surface_ready_source_();
        if (time_gate_ != nullptr) {
            time_gate_->SetSurfacePaused(!surface_ready);
            time_gate_->ObserveSchedulerFrame(clock.FrameCount());
        }
        const uint64_t frame = time_gate_ != nullptr
                                   ? time_gate_->GameFrame()
                                   : clock.FrameCount();
        current_frame_ = frame;
        if (!surface_ready) {
            suppressed_after_overlay_ = input_.action_down;
            overlay_last_tick_ = true;
            return;
        }

        // A bounded application overlay (tower directory, case file or final
        // decision) owns its small input transaction before normal movement
        // and pause handling. It does not add a second input system.
        if (input_overlay_callback_ && input_overlay_callback_(input_)) {
            suppressed_after_overlay_ = input_.action_down;
            overlay_last_tick_ = true;
            return;
        }
        ProductInputLease gameplay_input(input_, suppressed_after_overlay_, overlay_last_tick_);
        overlay_last_tick_ = false;

        if (input_.action_pressed[static_cast<size_t>(GameAction::Pause)]) {
            paused_ = !paused_;
            if (time_gate_ != nullptr) time_gate_->SetPaused(paused_);
            if (pause_callback_) pause_callback_();
        }
        if (paused_) {
            // Q is the existing lean-left binding during gameplay.  While
            // paused it is an intentional, discoverable quit chord shown by
            // the pause prompt; this avoids adding a second action enum or
            // stealing a live gameplay binding.
            if (input_.action_pressed[static_cast<size_t>(GameAction::LeanLeft)] &&
                quit_callback_) {
                quit_callback_();
            }
            return;
        }
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
        const Vec3 position_before_motion = locomotion_.position;
        IntegrateLocomotion(locomotion_, move, sprint, *world_query_,
                            SimClock::kFixedDeltaTime);
        const float moved_x = locomotion_.position.x - position_before_motion.x;
        const float moved_y = locomotion_.position.y - position_before_motion.y;
        if (std::isfinite(moved_x) && std::isfinite(moved_y) &&
            (moved_x * moved_x + moved_y * moved_y) > 0.000001f) {
            movement_demonstrated_ = true;
        }
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
        if (input_.action_pressed[static_cast<size_t>(GameAction::Help)]) {
            if (help_callback_) {
                help_callback_();
            } else if (narrator_intrusion_callback_) {
                narrator_intrusion_callback_();
            }
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
    void SetHelpCallback(std::function<void()> cb) { help_callback_ = std::move(cb); }
    void SetInputOverlayCallback(std::function<bool(const InputState&)> cb) {
        input_overlay_callback_ = std::move(cb);
    }
    void SetSaveCallback(std::function<void()> cb) { save_callback_ = std::move(cb); }
    void RequestSave() {
        if (save_callback_) save_callback_();
    }
    void SetLoadCallback(std::function<void()> cb) { load_callback_ = std::move(cb); }
    void RequestLoad() { if (load_callback_) load_callback_(); }
    void SetPauseCallback(std::function<void()> cb) { pause_callback_ = std::move(cb); }
    void SetQuitCallback(std::function<void()> cb) { quit_callback_ = std::move(cb); }
    void SetTimeGate(RuntimeTimeGate* gate) { time_gate_ = gate; }
    void SetSurfaceReadySource(std::function<bool()> source) { surface_ready_source_ = std::move(source); }
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
    std::function<void()> help_callback_;
    std::function<bool(const InputState&)> input_overlay_callback_;
    std::function<void()> save_callback_;
    std::function<void()> load_callback_;
    std::function<void()> pause_callback_;
    std::function<void()> quit_callback_;
    std::function<void()> melee_callback_;
    std::function<void(const FireRequest&, const WeaponDef&)> fire_callback_;
    std::function<Vec3()> recovery_spawn_source_;
    bool paused_ = false;
    uint16_t health_ = 100;
    std::array<bool, kGameActionCount> suppressed_after_overlay_{};
    bool overlay_last_tick_ = false;
    bool dead_ = false;
    bool movement_demonstrated_ = false;
    uint64_t current_frame_ = 0;
    RuntimeTimeGate* time_gate_ = nullptr;
    std::function<bool()> surface_ready_source_;
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
    void SetActiveScene(const std::string& scene) {
        if (active_scene_ == scene) return;
        active_scene_ = scene;
        // Short local observations belong to their scene, not a backlog
        // that resumes over a later reveal. Durable fired/causality state
        // remains untouched; load restores its saved queue after room setup.
        queue_ = DialogueQueue{};
    }
    std::string Text(const std::string& id) const { return ResolveTextId(id); }
    void SetPresentationBusySource(std::function<bool()> source) {
        presentation_busy_source_ = std::move(source);
    }
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
        // Do not consume once-only dialogue while another line owns the HUD.
        // Queue/save formats and world conditions remain unchanged.
        queue_.Advance(static_cast<uint32_t>(frame));
        if (queue_.Count() > 0 ||
            (presentation_busy_source_ && presentation_busy_source_())) return;
        const std::set<std::string> scene_flags{"scene:" + active_scene_};
        const Storylet* s = engine_.SelectEligible(
            *facts_, {}, {}, scene_flags, difficulty_, frame);
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
                                                  ctx_.settings && ctx_.settings->text_duration == 0 ? 300u :
                                                  ctx_.settings && ctx_.settings->text_duration == 2 ? 900u : 480u, NarratorSpeakerId(),
                                                  value.persona, 0});
                    } else if constexpr (std::is_same_v<Action, DialogAction>) {
                        last_presented_text_ = resolved_text[value.text_id];
                        ++presented_action_count_;
                        queue_.Push(SubtitleLine{last_presented_text_,
                                                  static_cast<uint32_t>(frame),
                                                  ctx_.settings && ctx_.settings->text_duration == 0 ? 300u :
                                                  ctx_.settings && ctx_.settings->text_duration == 2 ? 900u : 480u, NpcId{}, 0, 0});
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
    std::string active_scene_;
    std::function<bool()> pause_source_;
    std::function<uint64_t()> game_frame_source_;
    std::function<void(const WorldCommand&)> world_command_sink_;
    uint8_t difficulty_ = 1;
    std::map<std::string, std::string> text_resources_;
    std::function<bool()> presentation_busy_source_;
    bool text_resolution_error_reported_ = false;
    std::string last_presented_text_;
    size_t presented_action_count_ = 0;
};

// Shared presentation/interaction placement for chapter exit bays. The scene
// entity remains the approach-zone authority; the existing grid supplies the
// supporting wall. No additional layout file, physics or save state is added.
bool SceneDoorWallPosition(const SceneEntity& entity, const GridCell* cells,
                           int width, int height, Vec3& position) {
    if (entity.kind != SceneEntityKind::Door || cells == nullptr) return false;
    position = entity.position;
    if (entity.link_id != 0) {
        const int col = static_cast<int>(std::floor(position.x));
        const int row = static_cast<int>(std::floor(position.y));
        if (col < 0 || row < 0 || col >= width || row >= height ||
            (cells[row * width + col].flags & CellFlag_Solid) == 0) return false;
        position.x = static_cast<float>(col) - 0.035f;
        return true;
    }
    // Act II doors are authored on the inner face of the perimeter wall
    // (x/y == 1 or width/height - 1).  Do not ray-march from an anchor that
    // is already on that face: starting inside a solid boundary cell sends
    // east/south doors one cell outside the room, while the opposite sides
    // end up almost a cell inside it.  Keeping the authored plane intact is
    // what makes the frame/panel pair a real wall opening from both sides.
    const float max_x = static_cast<float>(width - 1);
    const float max_y = static_cast<float>(height - 1);
    const bool on_perimeter_face =
        std::fabs(position.x - 1.0f) <= 0.12f ||
        std::fabs(position.x - max_x) <= 0.12f ||
        std::fabs(position.y - 1.0f) <= 0.12f ||
        std::fabs(position.y - max_y) <= 0.12f;
    if (on_perimeter_face) return true;
    RayConfig mount;
    mount.origin_xy = {position.x, position.y};
    mount.yaw = entity.yaw;
    mount.max_distance = 8.0f;
    const RayResult wall = CastColumnRay(mount, cells, width, height);
    if (!wall.hit_full_occlusion) return false;
    const float offset = std::max(0.0f, wall.full_occlusion_distance - 0.035f);
    position.x += std::cos(entity.yaw) * offset;
    position.y += std::sin(entity.yaw) * offset;
    return true;
}

class RenderModule final : public IRenderModule {
public:
    RenderModule(std::unique_ptr<ITerminalBackend> backend, int& w, int& h,
                 int requested_w, int requested_h)
        : backend_(std::move(backend)),
          width_(w),
        height_(h), requested_width_(requested_w), requested_height_(requested_h) {
        body_.assign(static_cast<size_t>(w) * h, CharCell{});
        backend_->Init(w, h);
    }

    bool RefreshSurface() {
        const auto surface = FitTerminalSurface(requested_width_, requested_height_, backend_->GetCaps());
        if (surface.width != width_ || surface.height != height_) {
            width_ = surface.width; height_ = surface.height;
            body_.assign(static_cast<size_t>(width_) * height_, CharCell{});
        }
        return surface.Usable();
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
    void SetPresentationSource(std::function<std::string(const std::string&)> source) {
        presentation_source_ = std::move(source);
    }
    std::string Present(const std::string& value) const {
        return presentation_source_ ? presentation_source_(value) : value;
    }
    void SetProductSource(PlayerProductRuntime* product) { product_ = product; }
    void SetPanelScrollSource(std::function<size_t()> source) { panel_scroll_source_ = std::move(source); }
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
    void SetChapterClosureSource(std::function<bool()> source) {
        chapter_closure_source_ = std::move(source);
    }
    void SetClosureTextSource(std::function<std::string(size_t)> source) {
        closure_text_source_ = std::move(source);
    }
    void SetCampaignPanelSource(std::function<std::vector<std::string>()> source) {
        campaign_panel_source_ = std::move(source);
    }
    void SetObjectivePresentationPrefix(std::string prefix) {
        objective_presentation_prefix_ = std::move(prefix);
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
    // Critical player-facing feedback must not be replaced by a lower-value
    // notification arriving later in the same event window. This is a
    // bounded priority seam, not a second message system: equal priorities
    // remain last-writer-wins so existing interaction text stays responsive.
    void SetSubtitleOnce(const std::string& text, uint64_t frames,
                         uint8_t priority = 50) {
        const uint64_t frame = game_frame_source_ ? game_frame_source_() : 0;
        if (product_ != nullptr && frames < 10000 && !text.empty() && text != "Overlay closed.") {
            const bool speech = text.find("Guard:") != std::string::npos ||
                text.find("Cleaner:") != std::string::npos || text.find("Doctor:") != std::string::npos ||
                text.find("Technician:") != std::string::npos || text.find("Liaison:") != std::string::npos ||
                text.find("Operator:") != std::string::npos || text.find("Archivist:") != std::string::npos ||
                text.find("Dr. Vale:") != std::string::npos || text.find("Security:") != std::string::npos ||
                text.find("Clerk:") != std::string::npos || text.find("Analyst:") != std::string::npos;
            const bool danger = text.rfind("UNDER FIRE", 0) == 0 || text.rfind("YOU ARE DOWN", 0) == 0;
            product_->feed.Publish(danger ? PerceptionCategory::Threat : speech ? PerceptionCategory::Dialogue : PerceptionCategory::Progress,
                settings_ ? ProductControlText(text, *settings_) : text, text.substr(0, 80), scene_id_, frame,
                true, true, settings_ ? (settings_->text_duration == 0 ? 300 : settings_->text_duration == 2 ? 900 : 540) : 540);
        }
        if (frame < subtitle_override_until_ &&
            priority < subtitle_override_priority_) {
            return;
        }
        subtitle_override_ = settings_ ? ProductControlText(text, *settings_) : text;
        const uint64_t adjusted_frames = settings_ && frames < 10000
            ? (settings_->text_duration == 0 ? frames * 2 / 3 : settings_->text_duration == 2 ? frames * 2 : frames)
            : frames;
        subtitle_override_until_ = frame + adjusted_frames;
        subtitle_override_priority_ = priority;
    }
    void ResetTransientPresentation() {
        subtitle_override_.clear();
        subtitle_override_until_ = 0;
        subtitle_override_priority_ = 0;
        subtitle_.clear();
        last_perceived_subtitle_.clear();
        last_traced_subtitle_.clear();
        narrator_intrusion_.Reset();
        shot_flash_.Reset(); hit_flash_.Reset(); explosion_.Reset();
    }
    void TriggerNarratorIntrusion(uint64_t frames) {
        narrator_intrusion_.Trigger(game_frame_source_ ? game_frame_source_() : 0, frames);
    }
    bool NarratorTypographyActive() const {
        return narrator_intrusion_.Active(game_frame_source_ ? game_frame_source_() : 0);
    }
    void SetTextSource(std::function<std::string(const std::string&)> source) {
        text_source_ = std::move(source);
    }
    bool PresentationBusy() const {
        const uint64_t frame = game_frame_source_ ? game_frame_source_() : 0;
        if (npcs_ != nullptr) {
            for (const auto& npc : *npcs_) {
                if (npc.room == active_room_ && npc.instance.role == Role::Guard &&
                    npc.instance.state == NPCState::Combat) return true;
            }
        }
        return frame < subtitle_override_until_ || frame < scene_enter_frame_ + 300 ||
            (scene_id_ == "room_service_medical" && frame < scene_enter_frame_ + 780);
    }
    void SetPresentationTrace(bool enabled) { presentation_trace_ = enabled; }
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
        const uint64_t frame = game_frame_source_ ? game_frame_source_() : 0;
        shot_flash_.Trigger(frame, 4);
        if (feedback.target_was_npc) hit_flash_.Trigger(frame, 6);
        if (feedback.target_died) explosion_.Trigger(frame, 10);
    }
    void TriggerExplosion(uint64_t frames = 10) {
        explosion_.Extend(game_frame_source_ ? game_frame_source_() : 0, frames);
    }
    const std::vector<CharCell>& Cells() const { return body_; }

    void SetGridData(const GridCell* cells, int w, int h) {
        grid_cells_ = cells;
        grid_w_ = w;
        grid_h_ = h;
    }

    void RenderFrame(uint64_t frame_index, float alpha) override {
        (void)alpha;
        if (width_ < 48 || height_ < 18) {
            DrawCampaignPanel(body_.data(), width_, height_, {"RESIZE"}, 0,
                              Present("RESIZE TERMINAL: minimum 48 x 18"));
            backend_->Submit(body_.data(), width_, height_);
            return;
        }
        const uint64_t game_frame = game_frame_source_
                                        ? game_frame_source_()
                                        : frame_index;
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
            // An incapacitated actor has one visual authority: the
            // room-local BodyRecord.  Keep this outside the scene-specific
            // NPC branches so a security body (or a later bounded room actor)
            // cannot become invisible merely because the player changed rooms.
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
                    // Body art is a distinct floor pose.  Its visual authority
                    // is BodyRecord::position, not an incapacitated actor's
                    // standing sprite.
                    sprites.push_back(make_world_sprite(
                        0xB000000000000000ull | body.id.GetValue(),
                        body.position, 0.45f, body_kind, 0.0f));
                }
            }
            if (scene_entities_ != nullptr) {
                for (const auto& entity : *scene_entities_) {
                    if (entity.room != scene_id_) continue;
                    if (entity.kind == SceneEntityKind::DoorReader) {
                        // A reader is a small control at hand height, not a
                        // second full-size door at the same interaction point.
                        Vec3 position = entity.position;
                        position.x -= 0.24f;
                        position.z = 1.05f;
                        sprites.push_back(make_world_sprite(entity.stable_id,
                            position, 0.38f, CharacterSpriteKind::Terminal, entity.yaw));
                        continue;
                    }
                    if (entity.kind == SceneEntityKind::Door) {
                        Vec3 position;
                        if (!SceneDoorWallPosition(entity, grid_cells_, grid_w_,
                                                   grid_h_, position)) continue;
                        sprites.push_back(make_world_sprite(entity.stable_id,
                            position, entity.height, entity.visual, entity.yaw));
                        continue;
                    }
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
            if (combat_ != nullptr && combat_->last_shot_frame > 0 &&
                game_frame >= combat_->last_shot_frame &&
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
            DrawVisualEffects(game_frame);
        }
        const uint64_t scene_frame = game_frame >= scene_enter_frame_
                                         ? game_frame - scene_enter_frame_ : 0;
        subtitle_.clear();
        if (scene_frame < 300 && text_source_) {
            subtitle_ = text_source_("text_intro_" + scene_id_);
        }
        // A later room can acknowledge an actual discovery response from
        // the durable ledger. No timer invents a rescue or a security report.
        if (scene_id_ == "room_service_medical" && scene_frame < 780 &&
            scene_frame >= 300 && systemic_ != nullptr && text_source_) {
            for (auto it = systemic_->SystemEvents().rbegin();
                 it != systemic_->SystemEvents().rend(); ++it) {
                if (it->type == SystemicEventType::MedicalCall) {
                    subtitle_ = text_source_("text_medical_after_care");
                    break;
                }
                if (it->type == SystemicEventType::Report &&
                    std::find(it->tags.begin(), it->tags.end(),
                              "report_security") != it->tags.end()) {
                    subtitle_ = text_source_("text_medical_after_report");
                    break;
                }
            }
        }
        if (subtitle_.empty() && game_frame >= subtitle_override_until_ &&
            systemic_ != nullptr && text_source_ &&
            (dialogue_ == nullptr || dialogue_->Count() == 0)) {
            for (auto it = systemic_->SystemEvents().rbegin();
                 it != systemic_->SystemEvents().rend(); ++it) {
                if (it->location != active_room_ || game_frame < it->frame ||
                    game_frame - it->frame < 120 || game_frame - it->frame > 1200 ||
                    it->id == last_discovery_speech_) continue;
                const char* id = it->type == SystemicEventType::HelpCoverUp
                    ? "text_cleaner_coverup"
                    : it->type == SystemicEventType::MedicalCall
                        ? "text_cleaner_medical"
                        : it->type == SystemicEventType::Report &&
                            std::find(it->tags.begin(), it->tags.end(),
                                      "report_security") != it->tags.end()
                            ? "text_cleaner_report" : nullptr;
                if (id == nullptr) continue;
                SetSubtitleOnce(text_source_(id), 480, 25);
                last_discovery_speech_ = it->id;
                break;
            }
        }
        // Incidental speech is event- or interaction-driven. Proximity and
        // internal NPC states must not create a permanent subtitle ticker.
        if (dialogue_ != nullptr) {
            const auto active_lines = dialogue_->ActiveLines(
                static_cast<uint32_t>(std::min<uint64_t>(
                    game_frame, std::numeric_limits<uint32_t>::max())));
            if (!active_lines.empty()) subtitle_ = active_lines.back().text;
        }
        if (game_frame < subtitle_override_until_) {
            subtitle_ = subtitle_override_;
        }
        if (product_ && !subtitle_.empty() && game_frame >= subtitle_override_until_ &&
            subtitle_ != last_perceived_subtitle_) {
            product_->feed.Publish(PerceptionCategory::Dialogue, subtitle_, "spoken:" + subtitle_.substr(0, 70),
                scene_id_, game_frame, true, false);
        }
        last_perceived_subtitle_ = subtitle_;
        if (debug_overlay_) {
            subtitle_ = "F3 DEBUG | pos " + std::to_string(player_pos_.x) + "," +
                        std::to_string(player_pos_.y) + " yaw " + std::to_string(player_yaw_);
        }
        if (presentation_trace_ && subtitle_ != last_traced_subtitle_) {
            std::fprintf(stderr, "SUBTITLE_TRACE frame=%llu room=%s text=%s\n",
                static_cast<unsigned long long>(game_frame), scene_id_.c_str(),
                subtitle_.c_str());
            last_traced_subtitle_ = subtitle_;
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
        if (!objective_.empty() &&
            (objective_presentation_prefix_.empty() ||
             objective_.rfind(objective_presentation_prefix_, 0) == 0)) {
            objective_was_presented_ = true;
        }
        interaction_prompt_ = interaction_prompt_source_
                                  ? interaction_prompt_source_()
                                  : std::string{};
        if (settings_) interaction_prompt_ = ProductControlText(interaction_prompt_, *settings_);
        hud.interaction_prompt = interaction_prompt_.empty()
                                     ? nullptr
                                     : interaction_prompt_.c_str();
        hud.grid_width = grid_w_;
        hud.grid_height = grid_h_;
        hud.developer_overlay = debug_overlay_;
        auto campaign_panel = campaign_panel_source_
            ? campaign_panel_source_() : std::vector<std::string>{};
        if (product_ != nullptr && product_->Active() && settings_) campaign_panel = product_->Rows(*settings_,
            [this](const std::string& value) { return Present(value); });
        if (settings_) for (auto& line : campaign_panel) line = ProductControlText(line, *settings_);
        for (auto& line : campaign_panel) line = Present(line);
        const std::string visible_objective = Present(objective_);
        const std::string visible_prompt = Present(interaction_prompt_);
        const std::string visible_subtitle = Present(subtitle_);
        const std::string visible_weapon = Present(hud.weapon_name);
        const std::string visible_health = Present("Health");
        hud.objective = visible_objective.c_str();
        hud.interaction_prompt = visible_prompt.c_str();
        hud.weapon_name = visible_weapon.c_str();
        hud.health_label = visible_health.c_str();
        hud.subtitle = campaign_panel.empty() && (settings_ == nullptr || settings_->subtitles)
                           ? visible_subtitle.c_str() : nullptr;
        if (campaign_panel.empty()) hud_.Draw(body_.data(), width_, height_, hud);
        if (chapter_closure_source_ && chapter_closure_source_() && text_source_ &&
            width_ >= 48 && height_ >= 24) {
            // The durable checkpoint owns this quiet epilogue, including after
            // load. It does not pause input, invent a cutscene or run a timer.
            const std::array<const char*, 5> lines{{"text_closure_title",
                "text_closure_departure", "text_closure_status", "", ""}};
            for (size_t row = 0; row < lines.size(); ++row) {
                const std::string text = closure_text_source_
                    ? closure_text_source_(row) : text_source_(lines[row]);
                if (text.empty()) continue;
                const std::string clipped = text::Clip(Present(text), width_ - 8);
                const int count = text::Columns(clipped);
                const int left = (width_ - count) / 2;
                const int y = height_ / 5 + static_cast<int>(row) * 2;
                for (int x = -2; x < count + 2; ++x) {
                    CharCell& cell = body_[static_cast<size_t>(y) * width_ + left + x];
                    cell.code_point = U' ';
                    cell.fg_r = row == 2 ? 209 : 232;
                    cell.fg_g = row == 2 ? 167 : 228;
                    cell.fg_b = row == 2 ? 100 : 209;
                    cell.bg_r = 8; cell.bg_g = 13; cell.bg_b = 19;
                    cell.flags = row == 0 ? 0x01 : 0;
                }
                text::DrawRow(body_.data() + static_cast<size_t>(y) * width_, width_, left,
                              count, clipped, body_[static_cast<size_t>(y) * width_ + left]);
            }
        }
        if (narrator_intrusion_.Active(game_frame)) {
            const bool reduce_flicker = settings_ != nullptr && settings_->reduce_flicker;
            const bool reduce_shake = settings_ != nullptr && settings_->reduce_camera_shake;
            for (int yy = 0; yy < height_; ++yy) {
                for (int xx = 0; xx < width_; ++xx) {
                    CharCell& c = body_[static_cast<size_t>(yy) * width_ + xx];
                    c.bg_r = 5; c.bg_g = 7; c.bg_b = 15;
                    c.fg_r = 64; c.fg_g = 42; c.fg_b = 66;
                }
            }
            const uint64_t elapsed = narrator_intrusion_.Elapsed(game_frame);
            DrawNarratorTypography(elapsed, reduce_flicker, reduce_shake);
        }
        if (product_ != nullptr && campaign_panel.empty() && settings_) {
            const auto feed_rows = product_->feed.Visible(game_frame, settings_->sensory_verbosity, subtitle_, objective_);
            for (size_t row = 0; row < feed_rows.size() && height_ >= 18; ++row) {
                const size_t available = width_ > 4 ? static_cast<size_t>(width_ - 4) : 0;
                const std::string translated = Present(feed_rows[row]);
                const std::string line = available >= 3 && text::Columns(translated) > static_cast<int>(available)
                    ? text::Clip(translated, static_cast<int>(available) - 3) + "..." : translated;
                CharCell style;
                style.fg_r = 218; style.fg_g = 210; style.fg_b = 178;
                style.bg_r = 8; style.bg_g = 13; style.bg_b = 19;
                text::DrawRow(body_.data() + (row + 7) * static_cast<size_t>(width_), width_, 2,
                              static_cast<int>(available), line, style);
            }
        }
        DrawCampaignPanel(body_.data(), width_, height_, campaign_panel,
            product_ && product_->Active() ? product_->scroll : panel_scroll_source_ ? panel_scroll_source_() : 0,
            Present("RESIZE TERMINAL: minimum 48 x 18"), Present("SCROLL TO READ MORE"));
        backend_->Submit(body_.data(), width_, height_);
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
                std::max<uint64_t>(1, narrator_intrusion_.Duration()));
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

    void DrawVisualEffects(uint64_t frame_index) {
        const bool reduce_flicker = settings_ != nullptr && settings_->reduce_flicker;
        const bool reduce_shake = settings_ != nullptr && settings_->reduce_camera_shake;
        DrawCharacterEffects(body_.data(), width_, height_, frame_index,
                             shot_flash_.Active(frame_index),
                             hit_flash_.Active(frame_index),
                             explosion_.Active(frame_index),
                             reduce_flicker, reduce_shake);
    }

    std::unique_ptr<ITerminalBackend> backend_;
    int& width_;
    int& height_;
    const int requested_width_;
    const int requested_height_;
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
    std::function<std::string(const std::string&)> text_source_;
    std::function<std::string(const std::string&)> presentation_source_;
    EventId last_discovery_speech_;
    bool presentation_trace_ = false;
    std::string last_traced_subtitle_;
    std::string last_perceived_subtitle_;
    uint64_t subtitle_override_until_ = 0;
    uint8_t subtitle_override_priority_ = 0;
    PresentationPulse narrator_intrusion_;
    PresentationPulse shot_flash_;
    PresentationPulse hit_flash_;
    PresentationPulse explosion_;
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
    std::function<bool()> chapter_closure_source_;
    std::function<std::string(size_t)> closure_text_source_;
    std::function<std::vector<std::string>()> campaign_panel_source_;
    PlayerProductRuntime* product_ = nullptr;
    std::function<size_t()> panel_scroll_source_;
    std::function<std::string()> interaction_prompt_source_;
    std::string objective_presentation_prefix_;
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
    if (g.world->HasLoadedRoom()) {
        // The authored room spawn is a complete first-impression transform,
        // not only a position.  Leaving yaw at PlayerModule's default made
        // room-specific opening compositions (especially Records and Power)
        // start 90 degrees away from their focal work area.
        g.player->Locomotion().position = g.world->LoadedRoom().spawn_point;
        g.player->Locomotion().yaw = g.world->LoadedRoom().spawn_yaw;
        g.player->Locomotion().pitch = kOpeningPitch;
    } else {
        g.player->Locomotion().position = Vec3{1.5f, 6.0f, 0.0f};
        g.player->Locomotion().yaw = 0.0f;
        g.player->Locomotion().pitch = kOpeningPitch;
    }
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
    PresentationText presentation;
    if (!presentation.Load(data_root / "text")) {
        std::fprintf(stderr, "Missing or inconsistent bilingual presentation resources.\n");
        return 8;
    }
    PlayerProductRuntime product;
    PlayerPerceptionObserver perception_observer;
    std::string last_product_objective;
    bool new_game_requested = false;
    ProductSaveRole save_role = ProductSaveRole::Manual;
    std::string requested_load;
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
            if (const auto* shot = std::get_if<EventWeaponFire>(&event.payload)) {
                audio->PlaySfx(AudioId::New(shot->slot == WeaponSlot::Stunner ? 8 : 1),
                               shot->slot == WeaponSlot::Stunner ? 0.62f : 0.85f);
            } else if (std::holds_alternative<EventDamage>(event.payload)) {
                audio->PlaySfx(AudioId::New(2), 0.8f);
            } else if (std::holds_alternative<EventDoorChange>(event.payload)) {
                audio->PlaySfx(AudioId::New(3), 0.7f);
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
                                                  terminal_h, config.terminal_w, config.terminal_h);
    services.player->SetSurfaceReadySource([&render] { return render->RefreshSurface(); });
    render->SetSettingsSource(&settings);
    render->SetPresentationSource([&presentation, &settings](const std::string& value) {
        return presentation.Present(value, settings.language);
    });
    render->SetProductSource(&product);
    render->SetPresentationTrace(config.smoke || !config.replay_path.empty());
    render->SetPauseSource([&time_gate] { return time_gate.Paused(); });
    render->SetGameFrameSource([&time_gate] { return time_gate.GameFrame(); });
    render->SetSceneId(config.room_id.empty() ? "room_b1_revival" : config.room_id);
    services.narrative->SetActiveScene(
        config.room_id.empty() ? "room_b1_revival" : config.room_id);
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
    render->SetTextSource([&services](const std::string& id) {
        if (id == "text_intro_room_1f_security") {
            const auto fact = [&](const char* name) {
                WorldFact value;
                return services.world->Facts().Get(RuntimeFactId(name), value) &&
                    std::holds_alternative<bool>(value.value) && std::get<bool>(value.value);
            };
            if (fact("fact_b1_loud_action")) {
                if (fact("fact_b1_camera_disabled"))
                    return services.narrative->Text("text_security_camera_offline");
                if (services.systemic->AlertLevel() >= FacilityAlertLevel::Suspicious)
                    return services.narrative->Text("text_security_camera_online");
            }
        }
        return services.narrative->Text(id);
    });
    services.narrative->SetPresentationBusySource([&render] {
        return render->PresentationBusy();
    });

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
        TerminalId calibration_terminal;
        TerminalId medical_terminal;
        ObservationSourceId camera;
        QuestId opening_quest = QuestId::New(9005);
        QuestId chapter_quest = QuestId::New(9006);
        RoomId b1_room;
        NpcId security_guard_npc;
        NpcId records_operator_npc;
        NpcId power_technician_npc;
        NpcId transit_guard_npc;
        NpcId observation_analyst_npc;
        ItemId badge;
        ItemId cash;
        TerminalId dispatch_terminal;
        TerminalId records_terminal;
        TerminalId power_terminal;
        TerminalId power_backup_terminal;
        TerminalId observation_terminal;
        TerminalId transit_terminal;
        TerminalId arrival_directory_terminal;
        TerminalId records_subject_terminal;
        TerminalId operations_control_terminal;
        TerminalId network_observation_terminal;
        TerminalId transfer_gate_terminal;
        TerminalId executive_archive_terminal;
        TerminalId authority_decision_terminal;
        TerminalId roof_epilogue_terminal;
        ObservationSourceId transit_camera;
        bool terminal_session = false;
        bool bribe_done = false;
        bool schedule_found = false;
        bool body_created = false;
        bool body_hidden = false;
        bool body_discovered = false;
        bool cleaner_response = false;
        bool shot_hit = false;
        bool nonlethal_hit = false;
        bool badge_revoked = false;
        bool terminal_attempted = false;
        bool terminal_denied = false;
        bool player_died = false;
        bool player_recovered = false;
        bool player_restarted = false;
        bool access_attempted = false;
        bool access_denied = false;
        bool transition_denied = false;
        bool gate_unlocked = false;
        bool gate_open = false;
        bool gate_crossed = false;
        bool elevator_entry_attempted = false;
        bool elevator_entry_denied = false;
        bool act2_dispatch_reviewed = false;
        bool act2_records_consulted = false;
        bool act2_archive_released = false;
        bool act2_archive_terminal_accessed = false;
        bool act2_observation_found = false;
        bool act2_camera_looped = false;
        bool act2_power_helped = false;
        bool act2_power_rerouted = false;
        bool act2_utility_noise = false;
        bool act2_transit_guard_down = false;
        bool act2_transit_guard_bypassed = false;
        bool act2_transit_alerted = false;
        bool act2_transit_controlled = false;
        bool act2_checkpoint_reached = false;
        bool act2_access_denied = false;
        bool normal_quit_requested = false;
    } slice;
    slice.cart = ContainerId::New(cart_entity->systemic_id);
    slice.terminal = TerminalId::New(terminal_entity->systemic_id);
    slice.calibration_terminal = TerminalId::New(9006);
    slice.medical_terminal = TerminalId::New(9007);
    slice.camera = ObservationSourceId::New(camera_entity->systemic_id);
    slice.dispatch_terminal = TerminalId::New(9201);
    slice.records_terminal = TerminalId::New(9202);
    slice.power_terminal = TerminalId::New(9203);
    slice.power_backup_terminal = TerminalId::New(9204);
    slice.observation_terminal = TerminalId::New(9205);
    slice.transit_terminal = TerminalId::New(9206);
    slice.transit_camera = ObservationSourceId::New(9207);
    slice.arrival_directory_terminal = TerminalId::New(9303);
    slice.records_subject_terminal = TerminalId::New(9304);
    slice.operations_control_terminal = TerminalId::New(9305);
    slice.network_observation_terminal = TerminalId::New(9306);
    slice.transfer_gate_terminal = TerminalId::New(9307);
    slice.executive_archive_terminal = TerminalId::New(9308);
    slice.authority_decision_terminal = TerminalId::New(9309);
    slice.roof_epilogue_terminal = TerminalId::New(9310);
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
                                        ? "YOU ARE DOWN. Pause offers recovery points or a fresh game."
                                        : "UNDER FIRE / Break sight. Find cover.",
                                    services.player->Dead() ? 240 : 90,
                                    services.player->Dead() ? 100 : 80);
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
        TerminalRecord calibration_terminal;
        calibration_terminal.id = slice.calibration_terminal;
        calibration_terminal.room =
            RoomId::New(StableContentId("room_01_calibration"));
        calibration_terminal.credential_requirement = 2;
        calibration_terminal.access_scope.push_back("MEDICAL_ROUTE");
        TerminalRecord medical_terminal;
        medical_terminal.id = slice.medical_terminal;
        medical_terminal.room =
            RoomId::New(StableContentId("room_service_medical"));
        medical_terminal.credential_requirement = 2;
        medical_terminal.access_scope.push_back("MEDICAL_INTAKE");
        const RoomId act2_concourse_room =
            RoomId::New(StableContentId("room_act2_service_concourse"));
        const RoomId act2_records_room =
            RoomId::New(StableContentId("room_act2_records_archive"));
        const RoomId act2_power_room =
            RoomId::New(StableContentId("room_act2_power_utility"));
        const RoomId act2_transit_room =
            RoomId::New(StableContentId("room_act2_transit_control"));
        const RoomId act2_observation_room =
            RoomId::New(StableContentId("room_act2_observation_gallery"));
        const RoomId arrival_room =
            RoomId::New(StableContentId("room_1f_arrival_lobby"));
        const RoomId records_core_room =
            RoomId::New(StableContentId("room_8f_records_core"));
        const RoomId operations_room =
            RoomId::New(StableContentId("room_12f_operations_control"));
        const RoomId network_room =
            RoomId::New(StableContentId("room_18f_network_node"));
        const RoomId transfer_room =
            RoomId::New(StableContentId("room_24f_security_transfer"));
        const RoomId executive_room =
            RoomId::New(StableContentId("room_30f_executive_archive"));
        const RoomId authority_room =
            RoomId::New(StableContentId("room_36f_authority_core"));
        const RoomId roof_room =
            RoomId::New(StableContentId("room_roof_exit"));
        TerminalRecord dispatch_terminal;
        dispatch_terminal.id = slice.dispatch_terminal;
        dispatch_terminal.room = act2_concourse_room;
        dispatch_terminal.credential_requirement = 0;
        dispatch_terminal.access_scope.push_back("ACT2_DISPATCH");
        TerminalRecord records_terminal;
        records_terminal.id = slice.records_terminal;
        records_terminal.room = act2_records_room;
        records_terminal.credential_requirement = 2;
        records_terminal.access_scope.push_back("ACT2_RECORDS");
        TerminalRecord power_terminal;
        power_terminal.id = slice.power_terminal;
        power_terminal.room = act2_power_room;
        power_terminal.credential_requirement = 0;
        power_terminal.access_scope.push_back("ACT2_POWER");
        TerminalRecord power_backup_terminal;
        power_backup_terminal.id = slice.power_backup_terminal;
        power_backup_terminal.room = act2_power_room;
        power_backup_terminal.credential_requirement = 0;
        power_backup_terminal.access_scope.push_back("ACT2_POWER_BACKUP");
        TerminalRecord observation_terminal;
        observation_terminal.id = slice.observation_terminal;
        observation_terminal.room = act2_observation_room;
        observation_terminal.credential_requirement = 0;
        observation_terminal.access_scope.push_back("ACT2_OBSERVATION");
        TerminalRecord transit_terminal;
        transit_terminal.id = slice.transit_terminal;
        transit_terminal.room = act2_transit_room;
        transit_terminal.credential_requirement = 0;
        transit_terminal.access_scope.push_back("ACT2_TRANSIT");
        TerminalRecord arrival_directory_terminal;
        arrival_directory_terminal.id = slice.arrival_directory_terminal;
        arrival_directory_terminal.room = arrival_room;
        arrival_directory_terminal.access_scope.push_back("TOWER_DIRECTORY");
        TerminalRecord records_subject_terminal;
        records_subject_terminal.id = slice.records_subject_terminal;
        records_subject_terminal.room = records_core_room;
        records_subject_terminal.access_scope.push_back("SUBJECT_RECORD");
        TerminalRecord operations_control_terminal;
        operations_control_terminal.id = slice.operations_control_terminal;
        operations_control_terminal.room = operations_room;
        operations_control_terminal.access_scope.push_back("OPERATIONS_CONTROL");
        TerminalRecord network_observation_terminal;
        network_observation_terminal.id = slice.network_observation_terminal;
        network_observation_terminal.room = network_room;
        network_observation_terminal.access_scope.push_back("NETWORK_OBSERVATION");
        TerminalRecord transfer_gate_terminal;
        transfer_gate_terminal.id = slice.transfer_gate_terminal;
        transfer_gate_terminal.room = transfer_room;
        transfer_gate_terminal.access_scope.push_back("SECURITY_TRANSFER");
        TerminalRecord executive_archive_terminal;
        executive_archive_terminal.id = slice.executive_archive_terminal;
        executive_archive_terminal.room = executive_room;
        executive_archive_terminal.access_scope.push_back("EXECUTIVE_ARCHIVE");
        TerminalRecord authority_decision_terminal;
        authority_decision_terminal.id = slice.authority_decision_terminal;
        authority_decision_terminal.room = authority_room;
        authority_decision_terminal.access_scope.push_back("AUTHORITY_CORE");
        TerminalRecord roof_epilogue_terminal;
        roof_epilogue_terminal.id = slice.roof_epilogue_terminal;
        roof_epilogue_terminal.room = roof_room;
        roof_epilogue_terminal.access_scope.push_back("ROOF_EPILOGUE");
        ObservationSource camera;
        camera.id = slice.camera;
        camera.type = ObservationSourceType::Camera;
        camera.room = slice.b1_room;
        camera.online = true;
        camera.network_segment = "B1_SECURITY_LOOP";
        camera.provenance = "B1 ceiling camera 04";
        ObservationSource transit_camera;
        transit_camera.id = slice.transit_camera;
        transit_camera.type = ObservationSourceType::Camera;
        transit_camera.room = act2_transit_room;
        transit_camera.online = true;
        transit_camera.network_segment = "ACT2_TRANSIT_LOOP";
        transit_camera.provenance = "Transit camera 02";
        QuestRecord opening;
        opening.id = slice.opening_quest;
        opening.title = "Calibration route";
        opening.presentation_objective = "Reach the security checkpoint";
        opening.status = QuestStatus::Offered;
        QuestRecord chapter;
        chapter.id = slice.chapter_quest;
        chapter.title = "Chapter One: service route";
        chapter.presentation_objective = "Reach the elevator checkpoint";
        chapter.status = QuestStatus::Offered;
        const bool setup_ok = services.systemic->AddContainer(cart) &&
                              services.systemic->AddTerminal(terminal) &&
                              services.systemic->AddTerminal(calibration_terminal) &&
                              services.systemic->AddTerminal(medical_terminal) &&
                              services.systemic->AddTerminal(dispatch_terminal) &&
                              services.systemic->AddTerminal(records_terminal) &&
                              services.systemic->AddTerminal(power_terminal) &&
                              services.systemic->AddTerminal(power_backup_terminal) &&
                              services.systemic->AddTerminal(observation_terminal) &&
                              services.systemic->AddTerminal(transit_terminal) &&
                              services.systemic->AddObservationSource(camera) &&
                              services.systemic->AddObservationSource(transit_camera) &&
                              services.systemic->AddTerminal(arrival_directory_terminal) &&
                              services.systemic->AddTerminal(records_subject_terminal) &&
                              services.systemic->AddTerminal(operations_control_terminal) &&
                              services.systemic->AddTerminal(network_observation_terminal) &&
                              services.systemic->AddTerminal(transfer_gate_terminal) &&
                              services.systemic->AddTerminal(executive_archive_terminal) &&
                              services.systemic->AddTerminal(authority_decision_terminal) &&
                              services.systemic->AddTerminal(roof_epilogue_terminal) &&
                              services.systemic->AddQuest(opening) &&
                              services.systemic->AddQuest(chapter);
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
        (void)services.systemic->TransitionQuest(
            slice.chapter_quest, QuestStatus::Accepted, 0,
            "Complete the Chapter One service route");
        (void)services.systemic->TransitionQuest(
            slice.chapter_quest, QuestStatus::Active, 0,
            "Reach the elevator checkpoint");
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
             "fact_alpha_bribe_accepted", "fact_chapter_calibration_entered",
             "fact_chapter_calibration_accessed", "fact_chapter_medical_reached",
             "fact_chapter_medical_assessed", "fact_chapter_quiet_route",
             "fact_chapter_aggressive_route", "fact_chapter_staff_route",
             "fact_chapter_security_reached", "fact_chapter_security_checkpoint",
             "fact_chapter_checkpoint_reached", "fact_r1_guard_dead",
             "fact_act2_concourse_entered", "fact_act2_dispatch_reviewed",
             "fact_act2_records_consulted", "fact_act2_archive_released",
             "fact_act2_archive_terminal_accessed", "fact_act2_observation_found",
             "fact_act2_camera_looped", "fact_act2_power_helped",
             "fact_act2_power_rerouted", "fact_act2_utility_noise",
             "fact_act2_transit_guard_down", "fact_act2_transit_guard_bypassed",
             "fact_act2_transit_alerted", "fact_act2_transit_controlled",
             "fact_act2_checkpoint_reached", "fact_act2_access_denied",
             "fact_act3_arrival_reached", "fact_act3_hub_reviewed",
             "fact_act3_records_accessed", "fact_act3_authority_lead",
             "fact_act3_operations_accessed", "fact_act3_operations_cooperated",
             "fact_act3_network_discovered", "fact_act3_upper_access",
             "fact_act3_force_route", "fact_act4_transfer_reached",
             "fact_act4_guard_down", "fact_act4_guard_bypassed",
             "fact_act4_security_alerted", "fact_act4_archive_opened",
             "fact_act4_authority_ready", "fact_pre_final_checkpoint",
             "fact_ending_amend", "fact_ending_disclose", "fact_ending_breach",
             "fact_campaign_completed", "fact_roof_reached",
             "fact_elevator_records_unlocked", "fact_elevator_operations_unlocked",
             "fact_elevator_network_unlocked", "fact_elevator_transfer_unlocked",
             "fact_elevator_executive_unlocked", "fact_elevator_authority_unlocked",
             "fact_elevator_roof_unlocked"}) {
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
            if (actor->role == Role::Guard &&
                profile.spawn_room ==
                    RoomId::New(StableContentId("room_1f_security"))) {
                slice.security_guard_npc = profile.id;
            }
            if (profile.id == NpcId::New(StableContentId("records_operator"))) {
                slice.records_operator_npc = profile.id;
            } else if (profile.id == NpcId::New(StableContentId("power_technician"))) {
                slice.power_technician_npc = profile.id;
            } else if (profile.id == NpcId::New(StableContentId("security_response"))) {
                slice.transit_guard_npc = profile.id;
            } else if (profile.id == NpcId::New(StableContentId("observation_analyst"))) {
                slice.observation_analyst_npc = profile.id;
            }
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
            if (services.player->Dead()) return false;
            // The filed Records route is an authored, non-combat way through
            // Transit.  Keep the existing target gate as the single seam:
            // before the player makes a loud move, Transit Security can be
            // addressed and bypassed instead of turning a quiet route into a
            // mandatory firefight.  Any shot sets fact_b1_loud_action, which
            // immediately restores normal perception and combat semantics.
            if (services.player->CurrentRoom() == "room_act2_transit_control") {
                WorldFact quiet_route;
                WorldFact archive_route;
                WorldFact loud_action;
                WorldFact aggressive_route;
                const bool quiet_route_ready =
                    services.world->Facts().Get(
                        RuntimeFactId("fact_chapter_quiet_route"), quiet_route) &&
                    std::holds_alternative<bool>(quiet_route.value) &&
                    std::get<bool>(quiet_route.value);
                const bool archive_route_ready =
                    (services.world->Facts().Get(
                         RuntimeFactId("fact_act2_archive_released"),
                         archive_route) &&
                     std::holds_alternative<bool>(archive_route.value) &&
                     std::get<bool>(archive_route.value)) ||
                    (services.world->Facts().Get(
                         RuntimeFactId("fact_act2_archive_terminal_accessed"),
                         archive_route) &&
                     std::holds_alternative<bool>(archive_route.value) &&
                     std::get<bool>(archive_route.value));
                const bool loud =
                    services.world->Facts().Get(
                        RuntimeFactId("fact_b1_loud_action"), loud_action) &&
                    std::holds_alternative<bool>(loud_action.value) &&
                    std::get<bool>(loud_action.value);
                const bool aggressive =
                    services.world->Facts().Get(
                        RuntimeFactId("fact_chapter_aggressive_route"),
                        aggressive_route) &&
                    std::holds_alternative<bool>(aggressive_route.value) &&
                    std::get<bool>(aggressive_route.value);
                if (quiet_route_ready && archive_route_ready && !loud &&
                    !aggressive) {
                    return false;
                }
            }
            return true;
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
            (!feedback.target_stunned && !feedback.target_died)) {
            return;
        }
        const bool is_primary_b1_body = feedback.npc == slice.guard_npc;
        if (feedback.npc == slice.transit_guard_npc) {
            slice.act2_transit_guard_down = true;
            services.world->SetBooleanFact(
                RuntimeFactId("fact_act2_transit_guard_down"), slice.player, true);
        }
        if (feedback.npc == NpcId::New(StableContentId("transfer_guard"))) {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_act4_guard_down"), slice.player, true);
            services.world->SetBooleanFact(
                RuntimeFactId("fact_act3_force_route"), slice.player, true);
            services.world->SetBooleanFact(
                RuntimeFactId("fact_act4_security_alerted"), slice.player, true);
            services.world->SetBooleanFact(
                RuntimeFactId("fact_elevator_executive_unlocked"), slice.player, true);
            services.systemic->SetAlert(
                FacilityAlertLevel::Suspicious,
                {services.world->LoadedRoom().id}, services.player->CurrentFrame());
        }
        const EntityId body_id = is_primary_b1_body
            ? slice.body
            : EntityId::New(0xC000000000000000ull |
                            (feedback.npc.GetValue() & 0x3FFFFFFFFFFFFFFFull));
        if (services.systemic->GetBody(body_id) != nullptr) return;
        const RuntimeNpc* target = nullptr;
        for (const auto& runtime : services.ai->Npcs()) {
            if (runtime.instance.id == feedback.npc) {
                target = &runtime;
                break;
            }
        }
        if (target == nullptr) return;
        BodyRecord body;
        body.id = body_id;
        body.npc = feedback.npc;
        body.status = feedback.target_died ? BodyStatus::Dead : BodyStatus::Unconscious;
        body.disposition = BodyDisposition::Exposed;
        body.position = target->instance.position;
        body.room = target->room;
        if (!services.systemic->AddBody(body)) return;
        const EntityId target_entity = EntityId::New(feedback.npc.GetValue());
        if (is_primary_b1_body && slice.badge.IsValid() &&
            services.systemic->ItemHeldBy(slice.badge, target_entity)) {
            (void)services.systemic->TransferItem(slice.badge, body_id);
        }
        slice.body_created = slice.body_created || is_primary_b1_body;
        slice.nonlethal_hit = slice.nonlethal_hit || feedback.target_stunned;
        if (is_primary_b1_body && feedback.target_died) {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_r1_guard_dead"), slice.player, true);
        }
        render->SetSubtitleOnce(feedback.target_stunned
                                    ? "Target down. Search the body before moving on."
                                    : "Target dead. The scene is now evidence.",
                                180, 80);
    };
    services.ai->SetShotFeedbackCallback([&](const ShotFeedback& feedback) {
        render->TriggerShotFeedback(feedback);
        sync_body_from_feedback(feedback);
    });
    const EventBus::ConsumerId speech_consumer = events.Register(
        [&](const WorldEvent& event) {
            const auto* speech = std::get_if<EventNpcSpeak>(&event.payload);
            if (speech == nullptr) return;
            // Full cognition actors share this sight-entry event. It is not
            // a Security line id: resolve the actual speaker's role.
            if (speech->line != StringId::New(0xB1003)) return;
            const auto speaker = std::find_if(services.ai->Npcs().begin(),
                services.ai->Npcs().end(), [&](const RuntimeNpc& npc) {
                    return npc.instance.id == speech->npc;
                });
            if (speaker == services.ai->Npcs().end() ||
                speaker->room != services.world->LoadedRoom().id) return;
            const char* text_id = speaker->instance.role == Role::Doctor
                ? "text_doctor_greeting"
                : speaker->instance.role == Role::Cleaner
                    ? "text_cleaner_greeting"
                    : speaker->instance.role == Role::Technician
                        ? "text_technician_greeting"
                        : speaker->instance.faction == Faction::Security
                            ? "text_security_challenge" : nullptr;
            const std::string text = text_id != nullptr
                ? services.narrative->Text(text_id) : std::string{};
            if (text.empty()) return;
            // Speech is useful context, but it must not erase damage, death,
            // access, or body-state feedback that arrived in the same window.
            render->SetSubtitleOnce(text, 300, 20);
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
        // A loud action is a real world consequence only while the authored
        // B1 camera is online. The camera-offline counterfactual remains a
        // durable blind spot, while the online case raises the existing
        // facility alert state consumed by the later Security objective.
        const ObservationSource* camera_source =
            services.systemic->GetObservationSource(slice.camera);
        const bool camera_observes_current_room =
            camera_source != nullptr && camera_source->online &&
            camera_source->room == services.world->LoadedRoom().id;
        bool b1_terminal_session_active = false;
        if (camera_observes_current_room &&
            services.player->CurrentRoom() == "room_b1_revival") {
            for (const auto& session : services.systemic->TerminalSessions()) {
                if (session.terminal == slice.terminal &&
                    session.user == slice.player && session.active) {
                    b1_terminal_session_active = true;
                    break;
                }
            }
        }
        const ItemRecord* badge_record = slice.badge.IsValid()
                                              ? services.systemic->GetItem(slice.badge)
                                              : nullptr;
        const bool badge_held_by_player =
            badge_record != nullptr &&
            services.systemic->ItemHeldBy(slice.badge, slice.player);
        if (camera_observes_current_room && b1_terminal_session_active &&
            badge_held_by_player && badge_record != nullptr &&
            !badge_record->revoked &&
            services.systemic->RevokeCredential(
                slice.badge, services.player->CurrentFrame())) {
            slice.badge_revoked = true;
            render->SetSubtitleOnce(
                "CAMERA ALERT: your badge was revoked. Reload before the checkpoint.",
                220, 55);
        }
        if (camera_observes_current_room &&
            services.systemic->AlertLevel() < FacilityAlertLevel::Suspicious) {
            services.systemic->SetAlert(FacilityAlertLevel::Suspicious,
                                        {services.world->LoadedRoom().id},
                                        services.player->CurrentFrame());
            if (audio) audio->PlaySfx(AudioId::New(6), 0.50f);
            render->SetSubtitleOnce(
                "The active camera caught the disturbance.", 120, 45);
        }
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
        // Room links author a horizontal destination view.  Reset the
        // vertical look so a prior B1 interaction (which may leave the player
        // looking at a body or floor) cannot make the first target in the new
        // room impossible to focus.
        services.player->Locomotion().pitch = kOpeningPitch;
        services.player->SetCurrentRoom(id);
        services.narrative->SetActiveScene(id);
        if (id == "room_01_calibration") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_chapter_calibration_entered"),
                slice.player, true);
        } else if (id == "room_service_medical") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_alpha_medical_reached"),
                slice.player, true);
            services.world->SetBooleanFact(
                RuntimeFactId("fact_chapter_medical_reached"),
                slice.player, true);
        } else if (id == "room_1f_security") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_alpha_security_reached"),
                slice.player, true);
            services.world->SetBooleanFact(
                RuntimeFactId("fact_chapter_security_reached"),
                slice.player, true);
        } else if (id == "room_act2_service_concourse") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_act2_concourse_entered"),
                slice.player, true);
        } else if (id == "room_1f_arrival_lobby") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_act3_arrival_reached"),
                slice.player, true);
            WorldFact act2_checkpoint;
            const bool act2_complete =
                services.world->Facts().Get(
                    RuntimeFactId("fact_act2_checkpoint_reached"),
                    act2_checkpoint) &&
                std::holds_alternative<bool>(act2_checkpoint.value) &&
                std::get<bool>(act2_checkpoint.value);
            if (act2_complete) {
                services.world->SetBooleanFact(
                    RuntimeFactId("fact_elevator_records_unlocked"),
                    slice.player, true);
            }
        } else if (id == "room_8f_records_core") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_elevator_records_unlocked"),
                slice.player, true);
        } else if (id == "room_12f_operations_control") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_elevator_operations_unlocked"),
                slice.player, true);
        } else if (id == "room_18f_network_node") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_elevator_network_unlocked"),
                slice.player, true);
        } else if (id == "room_24f_security_transfer") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_elevator_transfer_unlocked"),
                slice.player, true);
            services.world->SetBooleanFact(
                RuntimeFactId("fact_act4_transfer_reached"),
                slice.player, true);
        } else if (id == "room_30f_executive_archive") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_elevator_executive_unlocked"),
                slice.player, true);
        } else if (id == "room_36f_authority_core") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_elevator_authority_unlocked"),
                slice.player, true);
        } else if (id == "room_roof_exit") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_roof_reached"),
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

    // Chapter links are authored SceneTransition records.  The policy is
    // assigned after the camera helper below so it can use the same durable
    // facts as objectives and interactions.
    std::function<bool(const SceneTransition&)> transition_allowed;

    auto serialize_player = [&]() {
        return SerializePlayerSave(
            {services.player->CurrentRoom(), services.player->Locomotion(),
             services.player->Combat(), services.player->Health(), services.player->Dead()},
            static_cast<uint32_t>(services.player->CurrentFrame()));
    };

    services.player->SetSaveCallback([&] {
        const ProductSaveRole role = save_role;
        save_role = ProductSaveRole::Manual;
        replay_save_attempted = true;
        replay_save_ok = false;
        if (services.player->Dead()) {
            render->SetSubtitleOnce("Cannot save while dead. Load or start a new game from Pause.", 240);
            return;
        }
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
        const auto written = WriteProductSaveRoles(save_dir, role, sections);
        replay_save_ok = written.primary_saved && written.resume_saved;
        product.continue_available = ProductSaveEnvelopeValid(save_dir / ProductResumeName(save_dir));
        product.checkpoint_available = ProductSaveEnvelopeValid(save_dir / "pvs_checkpoint");
        product.pre_final_available = ProductSaveEnvelopeValid(save_dir / "pvs_pre_final");
        render->SetSubtitleOnce(!replay_save_ok ? (written.primary_saved
            ? "Recovery slot written; Continue update failed. Check user-data access."
            : "Save failed. Previous recovery files are retained.") :
            role == ProductSaveRole::Manual ? "Manual save recorded." :
            role == ProductSaveRole::PreFinal ? "Pre-final checkpoint secured. The ending will not overwrite it." :
            role == ProductSaveRole::Completion ? "Ending saved. Pause offers Replay Final Choice." : "Checkpoint saved.", 240);
    });
    services.player->SetLoadCallback([&] {
        replay_load_attempted = true;
        replay_load_ok = false;
        SaveManager save;
        const std::string load_name = requested_load.empty()
            ? ProductResumeName(user_data_root / "saves") : requested_load;
        requested_load.clear();
        const auto loaded = save.LoadWorld(
            (user_data_root / "saves" / load_name).string());
        if (loaded.IsError()) {
            // A first death can legitimately happen before the player has made
            // a manual save.  Keep the dead state authoritative while the
            // load attempt is pending, then provide a deterministic authored
            // room restart instead of leaving the player in input limbo.
            std::error_code save_error;
            const bool save_exists = std::filesystem::exists(user_data_root / "saves" / (load_name + ".wo07"), save_error);
            if (!save_exists && !save_error && services.player->Dead() && services.world->HasLoadedRoom()) {
                const Room& room = services.world->LoadedRoom();
                services.player->Locomotion().position = room.spawn_point;
                services.player->Locomotion().velocity = Vec3{};
                services.player->Locomotion().yaw = room.spawn_yaw;
                services.player->Locomotion().pitch = kOpeningPitch;
                services.player->Locomotion().contact.grounded = true;
                services.player->SetHealthState(100, false);
                slice.player_restarted = true;
                render->SetSubtitleOnce(
                    "No save found. Recovered at the room entrance; world progression is retained.", 240);
            } else {
                render->SetSubtitleOnce("Load failed.", 120);
            }
            return;
        }
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
            PlayerSaveData staged;
            if (!ParsePlayerSave(player_section->data,
                    static_cast<uint32_t>(services.player->CurrentFrame()), staged)) {
                fail_load("Load failed: invalid player state."); return;
            }
            restored_room = std::move(staged.room);
            restored_loco = staged.locomotion;
            restored_combat = staged.combat;
            restored_health = staged.health;
            restored_dead = staged.dead;
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
        WorldFact restored_b1_checkpoint;
        if (services.world->Facts().Get(
                RuntimeFactId("fact_b1_checkpoint_reached"),
                restored_b1_checkpoint) &&
            std::holds_alternative<bool>(restored_b1_checkpoint.value)) {
            slice.gate_crossed =
                std::get<bool>(restored_b1_checkpoint.value) || slice.gate_crossed;
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
        render->ResetTransientPresentation();
        product.feed.Clear();
        product.ending_rows.clear();
        perception_observer.Reset();
        last_product_objective.clear();
        product.notice.clear();
        product.boot_context = false;
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
    constexpr float kPlayerInteractionDistance = 3.0f;
    const auto camera_looks_at = [&](const Vec3& target, float radius,
                                     float height) {
        return CameraRayHitsTarget(
            services.player->Locomotion(), terminal_w, terminal_h,
            settings.fov, services.world->Query(), target, radius, height,
            kPlayerInteractionDistance);
    };
    const auto fact_is_true = [&](std::string_view name) {
        WorldFact fact;
        return services.world->Facts().Get(RuntimeFactId(name), fact) &&
               std::holds_alternative<bool>(fact.value) &&
               std::get<bool>(fact.value);
    };
    TowerCampaignRuntime campaign(fact_is_true);
    enum class CampaignOverlay {
        None,
        Directory,
        CaseFile,
        Ending,
    };
    CampaignOverlay campaign_overlay = CampaignOverlay::None;
    size_t campaign_scroll = 0;
    render->SetPanelScrollSource([&] { return campaign_scroll; });
    size_t directory_selection = 0;
    std::vector<size_t> directory_options;
    std::vector<TowerCampaignRuntime::EndingOption> ending_options;
    TowerCampaignRuntime::Ending selected_ending =
        TowerCampaignRuntime::Ending::Amend;
    render->SetCampaignPanelSource([&]() -> std::vector<std::string> {
        if (campaign_overlay == CampaignOverlay::Directory)
            return campaign.DirectoryRows(services.player->CurrentRoom(), directory_selection);
        if (campaign_overlay == CampaignOverlay::CaseFile) {
            std::istringstream text(campaign.CaseFile(services.player->CurrentRoom(),
                PlayerKnownEvidence(*services.systemic, slice.player)));
            std::vector<std::string> rows;
            for (std::string line; std::getline(text, line);) rows.push_back(line);
            if (!rows.empty()) rows.pop_back();
            rows.insert(rows.begin() + 1, "LOCATION / " + product.location);
            for (const auto& item : services.systemic->Items()) {
                if (item.type == ItemType::Badge && services.systemic->ItemHeldBy(item.id, slice.player)) {
                    rows.push_back(item.revoked ? "CREDENTIAL / held badge REVOKED" : "CREDENTIAL / held badge VALID; readers still check access");
                }
            }
            rows.push_back(ProductBinding(settings, GameAction::MoveLeft) + "/" + ProductBinding(settings, GameAction::MoveRight) +
                " SCROLL  " + ProductBinding(settings, GameAction::Pause) + " CLOSE");
            return rows;
        }
        if (campaign_overlay == CampaignOverlay::Ending) {
            std::vector<std::string> rows{"FINAL DECISION / SUBJECT 07",
                ending_options.empty() ? "" : "> SELECTED: " +
                    ending_options[std::min(directory_selection, ending_options.size() - 1)].title};
            if (!ending_options.empty()) {
                rows.push_back(ending_options[std::min(directory_selection,
                    ending_options.size() - 1)].summary);
            }
            rows.push_back("F records this choice and takes you to Roof.");
            for (size_t i = 0; i < ending_options.size(); ++i) {
                rows.push_back(std::string(i == directory_selection ? "> " : "  ") +
                    ending_options[i].title);
                rows.push_back("  " + ending_options[i].summary);
                rows.push_back("");
            }
            rows.push_back("These resolutions reflect your filed route and evidence.");
            rows.push_back("F commits this resolution and takes you to Roof.");
            rows.push_back("W/S SELECT   F CONFIRM   ESC CLOSE");
            return rows;
        }
        return {};
    });
    const auto set_campaign_fact = [&](const char* name) {
        services.world->SetBooleanFact(RuntimeFactId(name), slice.player, true);
    };
    const auto player_has_valid_badge = [&] {
        return slice.badge.IsValid() &&
               services.systemic->ItemHeldBy(slice.badge, slice.player) &&
               services.systemic->ReaderAcceptsItem(slice.badge, 2);
    };
    const auto player_holds_revoked_badge = [&] {
        const ItemRecord* badge = slice.badge.IsValid()
                                      ? services.systemic->GetItem(slice.badge)
                                      : nullptr;
        return badge != nullptr && badge->revoked &&
               services.systemic->ItemHeldBy(slice.badge, slice.player);
    };
    const auto terminal_session_active = [&](TerminalId terminal_id) {
        for (const auto& session : services.systemic->TerminalSessions()) {
            if (session.terminal == terminal_id && session.user == slice.player &&
                session.active) {
                return true;
            }
        }
        return false;
    };
    const auto security_guard_disabled = [&] {
        if (!slice.security_guard_npc.IsValid()) return false;
        for (const auto& runtime : services.ai->Npcs()) {
            if (runtime.instance.id == slice.security_guard_npc &&
                (runtime.instance.state == NPCState::Stunned ||
                 runtime.instance.state == NPCState::Dead)) {
                return true;
            }
        }
        return false;
    };
    transition_allowed = [&](const SceneTransition& link) {
        if (link.id == "b1_to_calibration") {
            return player_has_valid_badge() &&
                   (terminal_session_active(slice.terminal) ||
                    fact_is_true("fact_b1_loud_action"));
        }
        if (link.id == "calibration_to_medical") {
            return fact_is_true("fact_chapter_calibration_accessed");
        }
        if (link.id == "medical_to_staff") {
            return fact_is_true("fact_chapter_medical_assessed") &&
                   fact_is_true("fact_chapter_quiet_route");
        }
        if (link.id == "medical_to_security") {
            return fact_is_true("fact_chapter_medical_assessed");
        }
        if (link.id == "medical_to_calibration") {
            return fact_is_true("fact_chapter_calibration_accessed");
        }
        if (link.id == "medical_to_elevator") {
            return fact_is_true("fact_chapter_staff_route") ||
                   fact_is_true("fact_chapter_security_checkpoint");
        }
        if (link.id == "security_to_elevator") {
            return fact_is_true("fact_chapter_security_checkpoint") ||
                   security_guard_disabled();
        }
        if (link.id == "staff_to_elevator") {
            return fact_is_true("fact_chapter_staff_route");
        }
        if (link.id == "elevator_to_act2_concourse") {
            return fact_is_true("fact_chapter_checkpoint_reached");
        }
        if (link.id == "act2_concourse_to_records" ||
            link.id == "act2_concourse_to_power") {
            return fact_is_true("fact_act2_dispatch_reviewed");
        }
        if (link.id == "act2_concourse_to_transit") {
            return fact_is_true("fact_act2_archive_released") ||
                   fact_is_true("fact_act2_archive_terminal_accessed") ||
                   fact_is_true("fact_act2_power_rerouted") ||
                   fact_is_true("fact_act2_transit_guard_down") ||
                   fact_is_true("fact_act2_checkpoint_reached");
        }
        if (link.id == "act2_power_to_transit") {
            return fact_is_true("fact_act2_power_rerouted") ||
                   fact_is_true("fact_act2_checkpoint_reached");
        }
        if (link.id == "act2_records_to_observation") {
            return fact_is_true("fact_act2_records_consulted") ||
                   fact_is_true("fact_act2_archive_terminal_accessed");
        }
        if (link.id == "act2_concourse_to_elevator" ||
            link.id == "act2_records_to_concourse" ||
            link.id == "act2_power_to_concourse" ||
            link.id == "act2_transit_to_concourse" ||
            link.id == "act2_transit_to_power" ||
            link.id == "act2_observation_to_records") {
            return true;
        }
        if (link.id == "act2_transit_to_arrival") {
            return fact_is_true("fact_act2_checkpoint_reached") &&
                   fact_is_true("fact_act2_transit_controlled");
        }
        if (link.id == "arrival_to_act2_transit") {
            return fact_is_true("fact_act2_checkpoint_reached");
        }
        if (link.id == "records_to_arrival" ||
            link.id == "operations_to_arrival" ||
            link.id == "network_to_arrival" ||
            link.id == "transfer_to_arrival" ||
            link.id == "executive_to_arrival" ||
            link.id == "authority_to_arrival" ||
            link.id == "roof_to_arrival") {
            return true;
        }
        // Backtracking links are intentionally safe once the destination
        // room exists. They do not create a second progression path.
        if (link.id == "security_to_medical_south" ||
            link.id == "security_to_medical_east" ||
            link.id == "staff_to_medical") {
            return true;
        }
        return true;
    };
    const auto enter_scene_transition = [&](const char* transition_id) {
        const SceneTransition* link = scene_runtime.FindTransition(transition_id);
        if (link == nullptr) return false;
        if (transition_allowed && !transition_allowed(*link)) {
            slice.transition_denied = true;
            render->SetSubtitleOnce(link->unavailable_message, 120);
            return true;
        }
        const bool security_guard_bypass =
            link->id == "security_to_elevator" && security_guard_disabled();
        if (!switch_room(link->destination_room, link->destination_spawn,
                         link->destination_yaw)) {
            render->SetSubtitleOnce(link->unavailable_message, 120);
            return true;
        }
        if (link->id == "b1_to_calibration") {
            slice.gate_crossed = true;
            services.world->SetBooleanFact(
                RuntimeFactId("fact_r1_checkpoint_reached"), slice.player, true);
            services.world->SetBooleanFact(
                RuntimeFactId("fact_b1_checkpoint_reached"), slice.player, true);
            const BodyRecord* route_body =
                services.systemic->GetBody(slice.body);
            // The chapter route records the player's durable approach, not the
            // body's transient post-discovery disposition. A cleaner may later
            // reopen a hidden cart body; that consequence must not rewrite a
            // previously quiet/non-lethal route into the loud branch.
            const bool quiet_route =
                route_body != nullptr &&
                fact_is_true("fact_b1_body_hidden") &&
                fact_is_true("fact_b1_nonlethal_action") &&
                !fact_is_true("fact_b1_loud_action");
            services.world->SetBooleanFact(
                RuntimeFactId("fact_chapter_quiet_route"),
                slice.player, quiet_route);
            services.world->SetBooleanFact(
                RuntimeFactId("fact_chapter_aggressive_route"),
                slice.player, !quiet_route);
            (void)services.systemic->TransitionQuest(
                slice.opening_quest, QuestStatus::Completed,
                services.player->CurrentFrame(),
                "B1 service checkpoint reached");
        }
        if (security_guard_bypass) {
            // The disabled-guard authorization is an intentional alternate
            // security route. Make the same durable checkpoint fact visible to
            // the elevator completion gate before entering its terminal room.
            services.world->SetBooleanFact(
                RuntimeFactId("fact_chapter_security_checkpoint"),
                slice.player, true);
        }
        if (link->id == "medical_to_staff") {
            services.world->SetBooleanFact(
                RuntimeFactId("fact_chapter_staff_route"), slice.player, true);
        }
        if (link->id == "b1_to_calibration" || link->id == "elevator_to_act2_concourse" ||
            link->id == "act2_transit_to_arrival") {
            save_role = ProductSaveRole::Checkpoint;
            services.player->RequestSave();
        }
        return true;
    };
    const auto focused_scene_entity = [&](const char* entity_id) {
        const SceneEntity* entity = scene_runtime.FindEntity(entity_id);
        if (entity == nullptr || entity->room != services.player->CurrentRoom()) return false;
        // Preserve the authored approach interaction, while making the actual
        // visible wall door usable when approached from a different angle.
        if (camera_looks_at(entity->position, entity->radius, entity->height)) return true;
        const Grid& grid = services.world->LoadedRoom().grid;
        Vec3 mounted;
        return entity->link_id == 0 && SceneDoorWallPosition(*entity,
            grid.Data().data(), grid.Width(), grid.Height(), mounted) &&
            camera_looks_at(mounted, entity->radius, entity->height);
    };
    const auto use_chapter_terminal = [&](TerminalId terminal_id,
                                           std::string_view action,
                                           std::string_view success_text) {
        const TerminalRecord* terminal = services.systemic->GetTerminal(terminal_id);
        if (terminal == nullptr || terminal->room != services.world->LoadedRoom().id ||
            !terminal->powered || !player_has_valid_badge() ||
            !services.systemic->ReaderAcceptsItem(
                slice.badge, terminal->credential_requirement)) {
            return false;
        }
        if (terminal_session_active(terminal_id)) return true;
        const uint64_t frame = services.player->CurrentFrame();
        TerminalSession session;
        session.terminal = terminal_id;
        session.user = slice.player;
        session.method = TerminalAccessMethod::Credential;
        session.started_frame = frame;
        session.active = true;
        TerminalAuditLog audit;
        audit.terminal = terminal_id;
        audit.user = slice.player;
        audit.method = TerminalAccessMethod::Credential;
        audit.frame = frame;
        audit.action = std::string(action);
        audit.unauthorized = false;
        if (!services.systemic->AddTerminalSession(session) ||
            !services.systemic->AddTerminalAudit(audit)) {
            return false;
        }
        render->SetSubtitleOnce(std::string(success_text), 180);
        return true;
    };
    const auto use_act2_terminal = [&](TerminalId terminal_id,
                                       std::string_view action,
                                       std::string_view success_text) {
        const TerminalRecord* terminal = services.systemic->GetTerminal(terminal_id);
        if (terminal == nullptr || terminal->room != services.world->LoadedRoom().id ||
            !terminal->powered) {
            return false;
        }
        if (terminal_session_active(terminal_id)) return true;
        const uint64_t frame = services.player->CurrentFrame();
        TerminalSession session;
        session.terminal = terminal_id;
        session.user = slice.player;
        session.method = TerminalAccessMethod::PhysicalServicePort;
        session.started_frame = frame;
        session.active = true;
        TerminalAuditLog audit;
        audit.terminal = terminal_id;
        audit.user = slice.player;
        audit.method = TerminalAccessMethod::PhysicalServicePort;
        audit.frame = frame;
        audit.action = std::string(action);
        audit.unauthorized = false;
        if (!services.systemic->AddTerminalSession(session) ||
            !services.systemic->AddTerminalAudit(audit)) {
            return false;
        }
        render->SetSubtitleOnce(std::string(success_text), 180);
        return true;
    };
    const auto focused_npc = [&](NpcId npc_id, float radius = 0.52f) {
        if (!npc_id.IsValid()) return false;
        for (const auto& runtime : services.ai->Npcs()) {
            if (runtime.instance.id != npc_id ||
                runtime.room != services.world->LoadedRoom().id ||
                runtime.instance.state == NPCState::Dead ||
                runtime.instance.state == NPCState::Stunned) {
                continue;
            }
            if (camera_looks_at(runtime.instance.position, radius, 1.85f)) {
                return true;
            }
        }
        return false;
    };
    // Alpha-01 presentation is intentionally a pair of small sources rather
    // than a new UI or quest framework. They read the same systemic facts and
    // authored scene records that the interaction callback below mutates.
    render->SetObjectiveSource([&] {
        const std::string& room = services.player->CurrentRoom();
        const bool has_badge = slice.badge.IsValid() &&
            services.systemic->ItemHeldBy(slice.badge, slice.player);
        if (room == "room_b1_revival") {
            if (player_holds_revoked_badge()) {
                return std::string("B1: Badge revoked; reload before the checkpoint");
            }
            if (const BodyRecord* body = services.systemic->GetBody(slice.body);
                body != nullptr && body->disposition == BodyDisposition::Exposed) {
                if (!body->searched) return std::string("B1: Search the downed guard");
                if (!has_badge) return std::string("B1: Take the access badge");
            }
            if (!has_badge) return std::string("B1: Find a way through the service route");
            if (!slice.terminal_session &&
                !fact_is_true("fact_b1_loud_action")) {
                return std::string("B1: Use the calibration terminal");
            }
            if (!slice.gate_open) return std::string("B1: Use the service reader");
            if (!slice.gate_crossed) return std::string("B1: Cross the open service door");
            return std::string("B1 complete: continue to calibration");
        }
        if (room == "room_01_calibration") {
            return fact_is_true("fact_chapter_calibration_accessed")
                       ? std::string("Calibration: enter Medical service")
                       : std::string("Calibration: use the route terminal");
        }
        if (room == "room_service_medical") {
            if (!fact_is_true("fact_chapter_medical_assessed")) {
                return std::string("Medical: access the intake terminal");
            }
            if (fact_is_true("fact_chapter_security_checkpoint")) {
                return std::string("Security: use the elevator checkpoint");
            }
            if (fact_is_true("fact_chapter_quiet_route")) {
                return std::string("Medical: take the quiet staff passage");
            }
            return std::string("Medical: report to the Security checkpoint");
        }
        if (room == "room_1f_security") {
            if (static_cast<uint8_t>(services.systemic->AlertLevel()) >=
                static_cast<uint8_t>(FacilityAlertLevel::Suspicious)) {
                return std::string("Security: response elevated; reach the checkpoint");
            }
            return fact_is_true("fact_chapter_security_checkpoint") ||
                           security_guard_disabled()
                        ? std::string("Security: use the elevator access door")
                       : std::string("Security: pass the checkpoint");
        }
        if (room == "room_elevator_lobby") {
            if (!fact_is_true("fact_chapter_checkpoint_reached")) {
                return std::string("Elevator: verify the chapter checkpoint");
            }
            return fact_is_true("fact_act2_concourse_entered")
                       ? std::string("Act II-A: return to the transfer zone")
                       : std::string("Act II-A: use the transfer control");
        }
        if (room == "room_act2_service_concourse") {
            if (!fact_is_true("fact_act2_dispatch_reviewed")) {
                return std::string("Act II-A: review the dispatch board");
            }
            if (!fact_is_true("fact_act2_archive_released") &&
                !fact_is_true("fact_act2_archive_terminal_accessed") &&
                !fact_is_true("fact_act2_power_rerouted")) {
                return std::string("Act II-A: choose a Records or Power route");
            }
            if (!fact_is_true("fact_act2_transit_controlled")) {
                return std::string("Act II-A: reach Transit Control");
            }
            return std::string("Act II-A: use the deep transfer door");
        }
        if (room == "room_act2_records_archive") {
            if (!fact_is_true("fact_act2_records_consulted") &&
                !fact_is_true("fact_act2_archive_terminal_accessed")) {
                return std::string("Records: secure a route into Transit Control");
            }
            return std::string("Records: optional observation gallery or return");
        }
        if (room == "room_act2_power_utility") {
            return fact_is_true("fact_act2_power_rerouted")
                       ? std::string("Power: take the maintenance bypass to Transit Control")
                       : std::string("Power: restore a maintenance bypass");
        }
        if (room == "room_act2_observation_gallery") {
            return fact_is_true("fact_act2_camera_looped")
                       ? std::string("Observation: return to Records")
                       : std::string("Observation: loop the transit camera");
        }
        if (room == "room_act2_transit_control") {
            if (!fact_is_true("fact_act2_transit_controlled")) {
                return std::string("Transit: clear the checkpoint and use control");
            }
            return std::string("Transit: use the deep transfer door");
        }
        if (room == "room_restroom_staff") {
            return std::string("Staff route: use the elevator service door");
        }
        if (room == "room_1f_arrival_lobby" ||
            room == "room_8f_records_core" ||
            room == "room_12f_operations_control" ||
            room == "room_18f_network_node" ||
            room == "room_24f_security_transfer" ||
            room == "room_30f_executive_archive" ||
            room == "room_36f_authority_core" ||
            room == "room_roof_exit") {
            return campaign.Objective(room);
        }
        return std::string("Explore the marked service route");
    });
    render->SetChapterClosureSource([&] {
        const bool chapter_closure =
            services.player->CurrentRoom() == "room_elevator_lobby" &&
            fact_is_true("fact_chapter_checkpoint_reached") &&
            !fact_is_true("fact_act2_concourse_entered");
        const bool campaign_closure =
            services.player->CurrentRoom() == "room_roof_exit" &&
            fact_is_true("fact_campaign_completed");
        return chapter_closure || campaign_closure;
    });
    render->SetClosureTextSource([&](size_t row) {
        if (services.player->CurrentRoom() == "room_roof_exit" &&
            fact_is_true("fact_campaign_completed")) {
            if (row == 0) {
                return std::string("WRITEOVER-07 / CAMPAIGN COMPLETE");
            }
            if (row == 1) {
                const std::string ending = fact_is_true("fact_ending_disclose")
                    ? "ENDING / DISCLOSE - THE RECORD LEFT THE FACILITY"
                    : fact_is_true("fact_ending_breach")
                        ? "ENDING / BREACH - THE EXIT WAS FORCED"
                        : "ENDING / AMEND - THE RECORD WAS RECONCILED";
                return ending;
            }
            if (row == 2) return services.narrative->Text(
                fact_is_true("fact_ending_disclose") ? "text_roof_disclose"
                : fact_is_true("fact_ending_breach") ? "text_roof_breach" : "text_roof_amend");
            if (row == 3) return ProductBinding(settings, GameAction::Pause) + " PAUSE / ENDING SUMMARY / REPLAY FINAL CHOICE / QUIT";
            return ProductBinding(settings, GameAction::Help) + " CASE FILE | LIFT DOOR: RETURN";
        }
        if (row == 0) return services.narrative->Text("text_closure_title");
        if (row == 1) return services.narrative->Text("text_closure_departure");
        if (row == 2) return services.narrative->Text("text_closure_status");
        return std::string{};
    });
    render->SetObjectivePresentationPrefix("B1:");
    bool interaction_demonstrated = false;
    render->SetInteractionPromptSource([&] {
        const std::string& room = services.player->CurrentRoom();
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
                    if (player_holds_revoked_badge()) {
                        return std::string("[F] USE TERMINAL (BADGE REVOKED)");
                    }
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
            if (services.player->CurrentFrame() < 360 &&
                !services.player->MovementDemonstrated() &&
                !interaction_demonstrated) {
                return std::string("WASD MOVE | MOUSE LOOK | F INTERACT | LMB FIRE");
            }
            return std::string{};
        }
        if (room == "room_01_calibration") {
            const SceneEntity* terminal = scene_runtime.FindEntity("calibration_terminal");
            if (terminal != nullptr && focused_scene_entity("calibration_terminal")) {
                return terminal_session_active(slice.calibration_terminal)
                           ? std::string("[F] REVIEW CALIBRATION ROUTE")
                           : std::string("[F] USE CALIBRATION TERMINAL");
            }
            if (focused_scene_entity("calibration_service_door")) {
                return std::string("[F] ENTER MEDICAL SERVICE");
            }
            return services.player->CurrentFrame() < 360 &&
                           !services.player->MovementDemonstrated() &&
                           !interaction_demonstrated
                       ? std::string("WASD MOVE | MOUSE LOOK | F INTERACT")
                       : std::string{};
        }
        if (room == "room_1f_security") {
            for (const auto& runtime : services.ai->Npcs()) {
                if (runtime.room == services.world->LoadedRoom().id &&
                    runtime.instance.role == Role::Guard &&
                    focused_scene_entity("security_service_door") == false &&
                    runtime.instance.state != NPCState::Dead &&
                    runtime.instance.state != NPCState::Stunned &&
                    camera_looks_at(runtime.instance.position, 0.90f, 1.80f)) {
                    return std::string("[F] ADDRESS SECURITY");
                }
            }
            if (focused_scene_entity("security_service_door")) {
                return std::string("[F] ENTER ELEVATOR ACCESS");
            }
            if (focused_scene_entity("security_medical_south_door") ||
                focused_scene_entity("security_medical_east_door")) {
                return std::string("[F] RETURN TO MEDICAL");
            }
            return std::string{};
        }
        if (room == "room_service_medical") {
            if (focused_scene_entity("medical_terminal")) {
                return terminal_session_active(slice.medical_terminal)
                           ? std::string("[F] REVIEW MEDICAL INTAKE")
                           : std::string("[F] USE MEDICAL TERMINAL");
            }
            if (focused_scene_entity("medical_calibration_door")) {
                return std::string("[F] RETURN TO CALIBRATION");
            }
            if (focused_scene_entity("medical_security_door")) {
                return std::string("[F] ENTER SECURITY");
            }
            if (focused_scene_entity("medical_staff_door")) {
                return std::string("[F] ENTER STAFF PASSAGE");
            }
            if (focused_scene_entity("medical_elevator_door")) {
                return std::string("[F] ENTER ELEVATOR LOBBY");
            }
            return std::string{};
        }
        if (room == "room_restroom_staff") {
            if (focused_scene_entity("staff_medical_door")) {
                return std::string("[F] RETURN TO MEDICAL");
            }
            if (focused_scene_entity("staff_elevator_door")) {
                return std::string("[F] ENTER ELEVATOR LOBBY");
            }
            return std::string{};
        }
        if (room == "room_elevator_lobby" &&
            focused_scene_entity("elevator_restricted_door")) {
            if (!fact_is_true("fact_chapter_checkpoint_reached")) {
                return std::string("[F] VERIFY ELEVATOR CHECKPOINT");
            }
            return std::string("[F] ENTER ACT II-A TRANSFER");
        }
        if (room == "room_act2_service_concourse") {
            if (focused_scene_entity("act2_dispatch_terminal")) {
                return terminal_session_active(slice.dispatch_terminal)
                           ? std::string("[F] REVIEW DISPATCH BOARD")
                           : std::string("[F] REVIEW DISPATCH BOARD");
            }
            if (focused_scene_entity("act2_concourse_records_door")) {
                return std::string("[F] ENTER RECORDS ARCHIVE");
            }
            if (focused_scene_entity("act2_concourse_power_door")) {
                return std::string("[F] ENTER POWER UTILITY");
            }
            if (focused_scene_entity("act2_concourse_transit_door")) {
                return std::string("[F] ENTER TRANSIT CONTROL");
            }
            if (focused_scene_entity("act2_concourse_elevator_door")) {
                return std::string("[F] RETURN TO ELEVATOR");
            }
            return std::string{};
        }
        if (room == "room_act2_records_archive") {
            if (focused_scene_entity("act2_records_terminal")) {
                return terminal_session_active(slice.records_terminal)
                           ? std::string("[F] REVIEW ARCHIVE MANIFEST")
                           : std::string("[F] QUERY ARCHIVE MANIFEST");
            }
            if (focused_npc(slice.records_operator_npc)) {
                return std::string("[F] SPEAK WITH RECORDS OPERATOR");
            }
            if (focused_scene_entity("act2_records_concourse_door")) {
                return std::string("[F] RETURN TO TRANSFER ZONE");
            }
            if (focused_scene_entity("act2_records_observation_door")) {
                return std::string("[F] ENTER OBSERVATION GALLERY");
            }
            return std::string{};
        }
        if (room == "room_act2_power_utility") {
            if (focused_scene_entity("act2_power_relay_terminal")) {
                return terminal_session_active(slice.power_terminal)
                           ? std::string("[F] REVIEW RELAY BYPASS")
                           : std::string("[F] REROUTE RESERVE FEED");
            }
            if (focused_scene_entity("act2_power_backup_terminal")) {
                return terminal_session_active(slice.power_backup_terminal)
                           ? std::string("[F] REVIEW BACKUP FEED")
                           : std::string("[F] FORCE BACKUP FEED");
            }
            if (focused_npc(slice.power_technician_npc)) {
                return std::string("[F] SPEAK WITH TECHNICIAN");
            }
            if (focused_scene_entity("act2_power_concourse_door")) {
                return std::string("[F] RETURN TO TRANSFER ZONE");
            }
            if (focused_scene_entity("act2_power_transit_door")) {
                return std::string("[F] TAKE MAINTENANCE BYPASS");
            }
            return std::string{};
        }
        if (room == "room_act2_observation_gallery") {
            if (focused_scene_entity("act2_gallery_records_terminal")) {
                return terminal_session_active(slice.observation_terminal)
                           ? std::string("[F] REVIEW CAMERA LOOP")
                           : std::string("[F] LOOP TRANSIT CAMERA");
            }
            if (focused_npc(slice.observation_analyst_npc)) {
                return std::string("[F] SPEAK WITH ANALYST");
            }
            if (focused_scene_entity("act2_gallery_records_door")) {
                return std::string("[F] RETURN TO RECORDS ARCHIVE");
            }
            return std::string{};
        }
        if (room == "room_act2_transit_control") {
            if (focused_npc(slice.transit_guard_npc)) {
                return std::string("[F] ADDRESS TRANSIT SECURITY");
            }
            if (focused_scene_entity("act2_transit_control_terminal")) {
                return terminal_session_active(slice.transit_terminal)
                           ? std::string("[F] REVIEW TRANSIT CONTROL")
                           : std::string("[F] USE CONTROL TERMINAL");
            }
            if (focused_scene_entity("act2_transit_exit_door")) {
                return std::string("[F] ENTER DEEP TRANSFER");
            }
            if (focused_scene_entity("act2_transit_concourse_door")) {
                return std::string("[F] RETURN TO TRANSFER ZONE");
            }
            if (focused_scene_entity("act2_transit_power_door")) {
                return std::string("[F] RETURN TO POWER UTILITY");
            }
        }
        if (room == "room_1f_arrival_lobby") {
            if (focused_scene_entity("arrival_tower_directory") ||
                focused_scene_entity("arrival_lift_directory")) {
                return std::string("[F] OPEN LIFT DIRECTORY");
            }
            if (focused_npc(NpcId::New(StableContentId("arrival_clerk")))) {
                return std::string("[F] SPEAK WITH ARRIVAL CLERK");
            }
            if (focused_scene_entity("arrival_lift_door")) {
                return std::string("[F] RETURN TO DEEP TRANSFER");
            }
        }
        if (room == "room_8f_records_core") {
            if (focused_scene_entity("records_subject_terminal")) {
                return terminal_session_active(slice.records_subject_terminal)
                           ? std::string("[F] REVIEW SUBJECT RECORD")
                           : std::string("[F] QUERY SUBJECT RECORD");
            }
            if (focused_npc(NpcId::New(StableContentId("records_archivist")))) {
                return std::string("[F] SPEAK WITH ARCHIVIST");
            }
            if (focused_scene_entity("records_return_door")) {
                return std::string("[F] RETURN TO ARRIVAL");
            }
        }
        if (room == "room_12f_operations_control") {
            if (focused_scene_entity("operations_control_terminal")) {
                return terminal_session_active(slice.operations_control_terminal)
                           ? std::string("[F] REVIEW OPERATIONS ROUTE")
                           : std::string("[F] RECONCILE OPERATIONS");
            }
            if (focused_npc(NpcId::New(StableContentId("operations_operator")))) {
                return std::string("[F] SPEAK WITH OPERATOR");
            }
            if (focused_scene_entity("operations_return_door")) {
                return std::string("[F] RETURN TO ARRIVAL");
            }
        }
        if (room == "room_18f_network_node") {
            if (focused_scene_entity("network_observation_terminal")) {
                return terminal_session_active(slice.network_observation_terminal)
                           ? std::string("[F] REVIEW UNLISTED FEED")
                           : std::string("[F] OPEN UNLISTED FEED");
            }
            if (focused_npc(NpcId::New(StableContentId("network_analyst")))) {
                return std::string("[F] SPEAK WITH ANALYST");
            }
            if (focused_scene_entity("network_return_door")) {
                return std::string("[F] RETURN TO ARRIVAL");
            }
        }
        if (room == "room_24f_security_transfer") {
            if (focused_scene_entity("transfer_gate_terminal")) {
                return terminal_session_active(slice.transfer_gate_terminal)
                           ? std::string("[F] REVIEW TRANSFER CLEARANCE")
                           : std::string("[F] PRESENT TRANSFER CLEARANCE");
            }
            if (focused_npc(NpcId::New(StableContentId("transfer_guard")))) {
                return std::string("[F] ADDRESS TRANSFER GUARD");
            }
            if (focused_scene_entity("transfer_return_door")) {
                return std::string("[F] RETURN TO ARRIVAL");
            }
        }
        if (room == "room_30f_executive_archive") {
            if (focused_scene_entity("executive_archive_terminal")) {
                return terminal_session_active(slice.executive_archive_terminal)
                           ? std::string("[F] REVIEW EXECUTIVE RECORD")
                           : std::string("[F] OPEN EXECUTIVE RECORD");
            }
            if (focused_npc(NpcId::New(StableContentId("executive_liaison")))) {
                return std::string("[F] SPEAK WITH LIAISON");
            }
            if (focused_scene_entity("executive_return_door")) {
                return std::string("[F] RETURN TO ARRIVAL");
            }
        }
        if (room == "room_36f_authority_core") {
            if (focused_scene_entity("authority_decision_terminal")) {
                if (fact_is_true("fact_campaign_completed")) {
                    return std::string("[F] REVIEW RECORDED DECISION");
                }
                return fact_is_true("fact_pre_final_checkpoint")
                           ? std::string("[F] OPEN FINAL DECISION")
                           : std::string("[F] ESTABLISH FINAL CHECKPOINT");
            }
            if (focused_npc(NpcId::New(StableContentId("authority_presence")))) {
                return std::string("[F] ADDRESS AUTHORITY");
            }
            if (focused_scene_entity("authority_return_door")) {
                return std::string("[F] RETURN TO ARRIVAL");
            }
        }
        if (room == "room_roof_exit" && focused_scene_entity("roof_epilogue_marker")) {
            return std::string("[F] READ FINAL STATUS");
        }
        return std::string{};
    });
    const auto close_campaign_overlay = [&] {
        campaign_overlay = CampaignOverlay::None;
        directory_options.clear();
        ending_options.clear();
        campaign_scroll = 0;
        time_gate.SetPaused(false);
    };
    const auto open_campaign_directory = [&] {
        product.Close();
        campaign_scroll = 0;
        directory_options = campaign.SelectableDestinations(
            services.player->CurrentRoom());
        directory_selection = 0;
        if (directory_options.empty()) {
            render->SetSubtitleOnce("LIFT DIRECTORY / NO VALID STOPS", 120, 90);
            return;
        }
        campaign_overlay = CampaignOverlay::Directory;
        time_gate.SetPaused(true);
    };
    const auto open_case_file = [&] {
        product.Close();
        campaign_scroll = 0;
        campaign_overlay = CampaignOverlay::CaseFile;
        time_gate.SetPaused(true);
    };
    const auto open_final_decision = [&] {
        product.Close();
        campaign_scroll = 0;
        if (fact_is_true("fact_campaign_completed")) {
            render->SetSubtitleOnce("This decision is already recorded. Your case file retains the outcome.", 240, 100);
            return;
        }
        ending_options = campaign.EligibleEndings();
        if (ending_options.empty()) {
            render->SetSubtitleOnce(
                "AUTHORITY / No eligible resolution. Records, operations and transfer remain incomplete.",
                240, 100);
            return;
        }
        campaign_overlay = CampaignOverlay::Ending;
        directory_selection = 0;
        time_gate.SetPaused(true);
    };
    services.player->SetInputOverlayCallback([&](const InputState& input) {
        const auto pressed = [&](GameAction action) {
            return input.action_pressed[static_cast<size_t>(action)];
        };
        product.dead = services.player->Dead();
        product.completed = fact_is_true("fact_campaign_completed");
        if (product.completed && product.ending_rows.empty()) {
            const std::string ending = fact_is_true("fact_ending_disclose") ? "DISCLOSE" :
                fact_is_true("fact_ending_breach") ? "BREACH" : "AMEND";
            product.ending_rows = {"CAMPAIGN COMPLETE / " + ending,
                ending == "DISCLOSE" ? "The record leaves the building. Authority no longer owns the only copy." :
                ending == "BREACH" ? "You forced an exit. The institution retains the account you refused to sign." :
                "Your correction is filed. The institution accepts a different version of Subject 07.",
                "EVIDENCE KNOWN / " + std::to_string(PlayerKnownEvidence(*services.systemic, slice.player)),
                fact_is_true("fact_act3_network_discovered") ? "You recovered the unlisted network feed." : "No unlisted network feed was recovered.",
                fact_is_true("fact_act3_operations_cooperated") ? "Operations supplied a cooperative route." : "Operations did not supply a cooperative route.",
                "You may explore the roof or return to the Case File.",
                "Pause > Replay Final Choice loads the separate pre-final save; it does not rewrite this completion.",
                "New Game rebuilds all live state. Existing recovery files remain until their next save."};
        }
        if (!product.completed) product.ending_rows.clear();
        const std::string current_room = services.player->CurrentRoom();
        product.location = "Facility";
        for (const auto& destination : campaign.Destinations()) {
            if (destination.room_id == current_room) product.location = destination.display_name;
        }
        const std::pair<const char*, const char*> local_names[] = {
            {"room_b1_revival", "B1 / Revival"}, {"room_01_calibration", "Calibration"},
            {"room_1f_security", "Security Checkpoint"}, {"room_service_medical", "Medical"},
            {"room_restroom_staff", "Staff Quarters"}, {"room_elevator_lobby", "Service Lift"},
            {"room_act2_service_concourse", "Service Concourse"}, {"room_act2_records_archive", "Records Annex"},
            {"room_act2_power_utility", "Utility Relay"}, {"room_act2_observation_gallery", "Observation"},
            {"room_act2_transit_control", "Transit Checkpoint"}};
        for (const auto& name : local_names) if (current_room == name.first) product.location = name.second;
        if (!input.has_focus) {
            if (!product.Active() && campaign_overlay == CampaignOverlay::None) {
                product.boot_context = false;
                product.Open(ProductPage::Pause);
                product.notice = "Focus lost. Resume when ready.";
            }
            time_gate.SetPaused(true);
            return true;
        }
        if (!time_gate.Paused()) {
            perception_observer.Observe(product, services.ai->Npcs(), scene_runtime,
                *services.systemic, services.world->Query(), services.world->LoadedRoom().id,
                services.player->CurrentRoom(), services.player->Locomotion(), services.player->CurrentFrame(), settings);
            const std::string& objective = render->ObjectiveText();
            if (!objective.empty() && objective != last_product_objective) {
                product.feed.Publish(PerceptionCategory::Progress, "Objective: " + objective,
                    "objective", services.player->CurrentRoom(), services.player->CurrentFrame());
                last_product_objective = objective;
            }
        }
        const auto load_product_save = [&] {
            const bool was_dead = services.player->Dead();
            services.player->RequestLoad();
            if (replay_load_ok || (was_dead && !services.player->Dead())) {
                product.Close(); campaign_overlay = CampaignOverlay::None;
                time_gate.SetPaused(false);
            } else product.notice = "Load rejected. No live state or recovery file was replaced.";
        };
        // World-mode load remains a normal player action. Only modal/death
        // recovery owns the transaction and fences held menu keys.
        if (pressed(GameAction::LoadGame) && (product.Active() ||
            campaign_overlay != CampaignOverlay::None || services.player->Dead())) {
            load_product_save(); return true;
        }
        if (product.Active()) {
            const ProductCommand command = product.Handle(input, settings);
            switch (command) {
            case ProductCommand::Resume: time_gate.SetPaused(false); break;
            case ProductCommand::Continue:
            case ProductCommand::Load:
            case ProductCommand::Checkpoint:
            case ProductCommand::PreFinal:
                requested_load = command == ProductCommand::Checkpoint ? "pvs_checkpoint" :
                    command == ProductCommand::PreFinal ? "pvs_pre_final" : "";
                load_product_save();
                break;
            case ProductCommand::Save:
                services.player->RequestSave();
                product.notice = replay_save_ok ? "Manual save recorded." : "Save failed; check user-data access.";
                break;
            case ProductCommand::NewGame:
                std::fprintf(stderr, "PRODUCT_NEW_GAME_REQUEST previous_completed=%s previous_health=%u previous_evidence=%zu\n",
                    product.completed ? "YES" : "NO", services.player->Health(), PlayerKnownEvidence(*services.systemic, slice.player));
                new_game_requested = true; engine.RequestStop(); break;
            case ProductCommand::Quit:
                slice.normal_quit_requested = true; engine.RequestStop(); break;
            case ProductCommand::CaseFile: open_case_file(); break;
            case ProductCommand::SettingsChanged: {
                SettingsRegistry registry;
                const auto saved = registry.Save((user_data_root / "settings.cfg").string(), settings);
                product.notice = saved.IsOk() ? "Preference saved." : "Preference active; persistence failed.";
                if (audio) audio->SetVolume(settings.master_volume / 100.0f, settings.sfx_volume / 100.0f, settings.narrator_volume / 100.0f);
                break;
            }
            default: break;
            }
            return true;
        }
        if (campaign_overlay == CampaignOverlay::None) {
            if (pressed(GameAction::Pause) || services.player->Dead()) {
                product.boot_context = false;
                product.Open(ProductPage::Pause);
                time_gate.SetPaused(true);
                return true;
            }
            if (pressed(GameAction::AimDownSights)) {
                product.boot_context = false;
                product.inspect_rows = InspectVisibleTarget(services.ai->Npcs(), scene_runtime,
                    *services.systemic, services.world->LoadedRoom().id, services.player->CurrentRoom(),
                    services.player->Locomotion().EyePosition(), camera_looks_at, focused_scene_entity);
                product.Open(ProductPage::Inspect);
                time_gate.SetPaused(true);
                return true;
            }
            if (pressed(GameAction::Help)) { open_case_file(); return true; }
            return false;
        }
        if (pressed(GameAction::MoveLeft)) campaign_scroll = campaign_scroll > 0 ? campaign_scroll - 1 : 0;
        if (pressed(GameAction::MoveRight)) ++campaign_scroll;
        if (pressed(GameAction::Pause)) {
            close_campaign_overlay();
            return true;
        }
        if (campaign_overlay == CampaignOverlay::CaseFile) {
            if (pressed(GameAction::Interact) || pressed(GameAction::Help)) {
                close_campaign_overlay();
            }
            return true;
        }
        if (campaign_overlay == CampaignOverlay::Directory) {
            if (directory_options.empty()) {
                close_campaign_overlay();
                return true;
            }
            if (pressed(GameAction::MoveForward)) {
                directory_selection = directory_selection == 0
                    ? directory_options.size() - 1 : directory_selection - 1;
            } else if (pressed(GameAction::MoveBackward)) {
                directory_selection = (directory_selection + 1) % directory_options.size();
            } else if (pressed(GameAction::Interact)) {
                const auto& destination =
                    campaign.Destinations()[directory_options[directory_selection]];
                if (destination.room_id == services.player->CurrentRoom()) {
                    close_campaign_overlay();
                    return true;
                }
                const bool moved = switch_room(
                    destination.room_id, Vec3{2.5f, destination.room_id == "room_roof_exit" ? 32.0f : 5.0f, 0.0f}, 0.0f);
                if (!moved) {
                    render->SetSubtitleOnce("Lift destination unavailable.", 150, 100);
                } else {
                    campaign_overlay = CampaignOverlay::None;
                    directory_options.clear();
                    time_gate.SetPaused(false);
                    render->SetSubtitleOnce(
                        "Lift arrival recorded. " + destination.display_name,
                        180, 95);
                }
            }
            return true;
        }
        if (campaign_overlay == CampaignOverlay::Ending) {
            if (ending_options.empty()) {
                close_campaign_overlay();
                return true;
            }
            if (pressed(GameAction::MoveForward)) {
                directory_selection = directory_selection == 0
                    ? ending_options.size() - 1 : directory_selection - 1;
            } else if (pressed(GameAction::MoveBackward)) {
                directory_selection = (directory_selection + 1) % ending_options.size();
            }
            if (!pressed(GameAction::MoveForward) && !pressed(GameAction::MoveBackward) &&
                pressed(GameAction::Interact)) {
                const auto option = ending_options[directory_selection];
                if (!switch_room("room_roof_exit", Vec3{2.5f, 32.0f, 0.0f}, 0.0f)) {
                    render->SetSubtitleOnce("Roof unavailable. No decision was recorded.", 240, 105);
                    return true;
                }
                selected_ending = option.ending;
                set_campaign_fact(TowerCampaignRuntime::EndingFact(selected_ending));
                set_campaign_fact("fact_campaign_completed");
                set_campaign_fact("fact_elevator_roof_unlocked");
                // RequestSave invokes the callback immediately. Persist the
                // destination and its facts, not a half-completed Authority state.
                save_role = ProductSaveRole::Completion;
                services.player->RequestSave();
                campaign_overlay = CampaignOverlay::None;
                ending_options.clear();
                time_gate.SetPaused(false);
                {
                    render->SetSubtitleOnce(
                        std::string("Decision recorded: ") +
                            TowerCampaignRuntime::EndingLabel(selected_ending) +
                            ". You are outside the authorization chain.",
                        240, 105);
                }
            }
            return true;
        }
        return true;
    });
    services.player->SetHelpCallback(open_case_file);
    const auto add_campaign_knowledge = [&](uint64_t numeric_id,
                                             KnowledgeAssetType type,
                                             float confidence) {
        const KnowledgeAssetId id = KnowledgeAssetId::New(numeric_id);
        if (services.systemic->GetKnowledgeAsset(id) != nullptr) return;
        KnowledgeAssetRecord asset;
        asset.id = id;
        asset.type = type;
        asset.source = ResourceId::New(4);
        asset.confidence = confidence;
        asset.known_by.push_back(slice.player);
        (void)services.systemic->AddKnowledgeAsset(asset);
    };
    services.player->SetInteractCallback([&] {
        interaction_demonstrated = true;
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
                if (!hit || hit_distance > kPlayerInteractionDistance ||
                    hit_distance >= focused_distance) {
                    return;
                }
                // Keep nearest-target ordering above, but share visibility
                // with the HUD/chapter interactions. When the eye overlaps
                // a reader proxy, its forward exit can lie behind the closed
                // door; that exit must not make the nearby reader invisible.
                if (!camera_looks_at(candidate.position, candidate.radius,
                                     candidate.height)) return;
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
                const bool badge_held = slice.badge.IsValid() &&
                    services.systemic->ItemHeldBy(slice.badge, player);
                if (!player_holds_valid_badge()) {
                    slice.access_denied = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_b1_gate_denied"), player, true);
                    render->SetSubtitleOnce(
                        badge_held && player_holds_revoked_badge()
                            ? "ACCESS DENIED. The held badge is revoked; reload before the checkpoint."
                            : "ACCESS DENIED. The reader needs your held badge.",
                        180);
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
                    if (!enter_scene_transition("b1_to_calibration")) {
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
                if (player_holds_revoked_badge()) {
                    slice.badge_revoked = true;
                    slice.terminal_denied = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_b1_gate_denied"), player, true);
                    render->SetSubtitleOnce(
                        "TERMINAL: badge revoked; the active session no longer authorizes access.",
                        180);
                } else if (!slice.terminal_session && player_holds_valid_badge()) {
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
                            services.narrative->Text("text_cleaner_agreement"),
                            420);
                    }
                } else {
                    for (const auto& npc : services.ai->Npcs()) {
                        if (npc.instance.id != focused.npc) continue;
                        const char* id = npc.instance.role == Role::Technician
                            ? "text_technician_greeting"
                            : npc.instance.role == Role::Doctor
                                ? "text_doctor_greeting"
                                : npc.instance.role == Role::Cleaner
                                    ? "text_cleaner_greeting"
                                    : npc.instance.faction == Faction::Security
                                        ? "text_security_greeting" : nullptr;
                        // A missing authored line is silence, not permission to
                        // borrow Security's voice. This keeps the opening
                        // quiet for staff, research, and other non-security
                        // actors whose interaction contract is not yet authored.
                        if (id != nullptr) {
                            render->SetSubtitleOnce(services.narrative->Text(id), 420);
                        }
                        break;
                    }
                }
                return;
            }
            render->SetSubtitleOnce("B1: maintenance, camera loop, terminal, or checkpoint.", 100);
            return;
        }

        if (services.player->CurrentRoom() == "room_01_calibration") {
            if (focused_scene_entity("calibration_terminal")) {
                if (terminal_session_active(slice.calibration_terminal)) {
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_chapter_calibration_accessed"),
                        player, true);
                    render->SetSubtitleOnce(
                        "CALIBRATION: route already verified. Proceed to Medical.",
                        160);
                } else if (use_chapter_terminal(
                               slice.calibration_terminal,
                               "read_medical_route",
                               "CALIBRATION: route verified. Service access is open.")) {
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_chapter_calibration_accessed"),
                        player, true);
                } else {
                    render->SetSubtitleOnce(
                        "CALIBRATION: held credential required.", 160);
                }
                return;
            }
            if (focused_scene_entity("calibration_service_door")) {
                (void)enter_scene_transition("calibration_to_medical");
                return;
            }
            render->SetSubtitleOnce(services.narrative->Text("text_calibration_note"), 360);
            return;
        }

        if (services.player->CurrentRoom() == "room_1f_security") {
            const bool has_badge = slice.badge.IsValid() &&
                services.systemic->ItemHeldBy(slice.badge, player) &&
                services.systemic->ReaderAcceptsItem(slice.badge, 2);
            const bool bribe_done = slice.bribe_done || [&] {
                WorldFact fact;
                return services.world->Facts().Get(
                           RuntimeFactId("fact_alpha_bribe_accepted"), fact) &&
                       std::holds_alternative<bool>(fact.value) &&
                       std::get<bool>(fact.value);
            }();
            if (interaction_guard_available &&
                camera_looks_at(interaction_guard_position, 0.90f, 1.80f)) {
                if (interaction_guard_npc == slice.security_guard_npc &&
                    security_guard_disabled()) {
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_chapter_security_checkpoint"),
                        player, true);
                    render->SetSubtitleOnce(
                        "SECURITY: the checkpoint is open while the guard is down.",
                        180);
                } else if (has_badge) {
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_chapter_security_checkpoint"),
                        player, true);
                    render->SetSubtitleOnce(services.narrative->Text("text_security_granted"), 360);
                } else if (!bribe_done && slice.cash.IsValid() &&
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
                        services.world->SetBooleanFact(
                            RuntimeFactId("fact_chapter_security_checkpoint"),
                            player, true);
                        render->SetSubtitleOnce(services.narrative->Text("text_security_bribe"), 360);
                    }
                } else if (bribe_done) {
                    render->SetSubtitleOnce(services.narrative->Text("text_security_bribe_repeat"), 300);
                } else {
                    render->SetSubtitleOnce("ACCESS DENIED. No valid credential.", 180);
                }
                return;
            }
            if (focused_scene_entity("security_service_door")) {
                (void)enter_scene_transition("security_to_elevator");
                return;
            }
            if (focused_scene_entity("security_medical_south_door")) {
                (void)enter_scene_transition("security_to_medical_south");
                return;
            }
            if (focused_scene_entity("security_medical_east_door")) {
                (void)enter_scene_transition("security_to_medical_east");
                return;
            }
            if (p.x < 8.0f && p.y < 5.0f) {
                bool schedule_found = slice.schedule_found;
                if (!schedule_found) {
                    for (const auto& asset : services.systemic->Knowledge()) {
                        if (asset.type == KnowledgeAssetType::ShiftSchedule &&
                            std::find(asset.known_by.begin(), asset.known_by.end(),
                                      player) != asset.known_by.end()) {
                            schedule_found = true;
                            break;
                        }
                    }
                }
                if (!schedule_found) {
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
            if (focused_scene_entity("medical_terminal")) {
                if (terminal_session_active(slice.medical_terminal)) {
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_chapter_medical_assessed"),
                        player, true);
                    render->SetSubtitleOnce(
                        "MEDICAL: intake already recorded. Choose a service route.",
                        160);
                } else if (use_chapter_terminal(
                               slice.medical_terminal,
                               "complete_medical_intake",
                               "MEDICAL: intake recorded. Choose a service route.")) {
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_chapter_medical_assessed"),
                        player, true);
                } else {
                    render->SetSubtitleOnce(
                        "MEDICAL: held credential required for intake.", 160);
                }
                return;
            }
            if (focused_scene_entity("medical_calibration_door")) {
                (void)enter_scene_transition("medical_to_calibration");
                return;
            }
            if (focused_scene_entity("medical_security_door")) {
                (void)enter_scene_transition("medical_to_security");
                return;
            }
            if (focused_scene_entity("medical_staff_door")) {
                (void)enter_scene_transition("medical_to_staff");
                return;
            }
            if (focused_scene_entity("medical_elevator_door")) {
                (void)enter_scene_transition("medical_to_elevator");
                return;
            }
            render->SetSubtitleOnce(services.narrative->Text("text_medical_note"), 360);
            return;
        }

        if (services.player->CurrentRoom() == "room_restroom_staff") {
            if (focused_scene_entity("staff_medical_door")) {
                (void)enter_scene_transition("staff_to_medical");
                return;
            }
            if (focused_scene_entity("staff_elevator_door")) {
                (void)enter_scene_transition("staff_to_elevator");
                return;
            }
            render->SetSubtitleOnce(services.narrative->Text("text_staff_note"), 360);
            return;
        }

        if (services.player->CurrentRoom() == "room_act2_service_concourse") {
            if (focused_scene_entity("act2_dispatch_terminal")) {
                const bool already_reviewed =
                    fact_is_true("fact_act2_dispatch_reviewed");
                if (use_act2_terminal(slice.dispatch_terminal,
                                       "review_dispatch_board",
                                       services.narrative->Text("text_act2_dispatch"))) {
                    slice.act2_dispatch_reviewed = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_dispatch_reviewed"), player, true);
                    if (already_reviewed) {
                        render->SetSubtitleOnce(
                            "DISPATCH: route board already reviewed. Choose Records or Power.",
                            150);
                    }
                } else {
                    render->SetSubtitleOnce("DISPATCH: control board unavailable.", 140);
                }
                return;
            }
            if (focused_scene_entity("act2_concourse_records_door")) {
                (void)enter_scene_transition("act2_concourse_to_records");
                return;
            }
            if (focused_scene_entity("act2_concourse_power_door")) {
                (void)enter_scene_transition("act2_concourse_to_power");
                return;
            }
            if (focused_scene_entity("act2_concourse_transit_door")) {
                (void)enter_scene_transition("act2_concourse_to_transit");
                return;
            }
            if (focused_scene_entity("act2_concourse_elevator_door")) {
                (void)enter_scene_transition("act2_concourse_to_elevator");
                return;
            }
            render->SetSubtitleOnce(services.narrative->Text("text_act2_concourse"), 300);
            return;
        }

        if (services.player->CurrentRoom() == "room_act2_records_archive") {
            if (focused_npc(slice.records_operator_npc)) {
                const bool already_consulted =
                    fact_is_true("fact_act2_records_consulted");
                RelationshipRecord relationship;
                relationship.a = EntityId::New(slice.records_operator_npc.GetValue());
                relationship.b = player;
                relationship.trust = 0.72f;
                relationship.respect = 0.58f;
                relationship.suspicion =
                    fact_is_true("fact_chapter_aggressive_route") ? 0.28f : 0.10f;
                relationship.debt = 0.24f;
                const bool relationship_saved =
                    services.systemic->SetRelationship(relationship);
                slice.act2_records_consulted = true;
                slice.act2_archive_released = true;
                services.world->SetBooleanFact(
                    RuntimeFactId("fact_act2_records_consulted"), player, true);
                services.world->SetBooleanFact(
                    RuntimeFactId("fact_act2_archive_released"), player, true);
                const KnowledgeAssetId route_asset = KnowledgeAssetId::New(9301);
                if (services.systemic->GetKnowledgeAsset(route_asset) == nullptr) {
                    KnowledgeAssetRecord asset;
                    asset.id = route_asset;
                    asset.type = KnowledgeAssetType::AccessProcedure;
                    asset.source = ResourceId::New(2);
                    asset.confidence = 0.82f;
                    asset.known_by.push_back(player);
                    (void)services.systemic->AddKnowledgeAsset(asset);
                }
                const bool aggressive = fact_is_true("fact_chapter_aggressive_route") ||
                                        fact_is_true("fact_b1_loud_action");
                render->SetSubtitleOnce(
                    services.narrative->Text(already_consulted
                        ? "text_act2_records_operator_repeat"
                        : aggressive ? "text_act2_records_aggressive"
                                     : "text_act2_records_operator"),
                    relationship_saved ? 360 : 240);
                return;
            }
            if (focused_scene_entity("act2_records_terminal")) {
                if (!player_has_valid_badge()) {
                    slice.act2_access_denied = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_access_denied"), player, true);
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_transit_denied"), 180);
                } else if (use_act2_terminal(
                               slice.records_terminal, "query_archive_manifest",
                               services.narrative->Text("text_act2_records_terminal"))) {
                    slice.act2_archive_terminal_accessed = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_archive_terminal_accessed"), player, true);
                    if (terminal_session_active(slice.records_terminal)) {
                        render->SetSubtitleOnce(
                            services.narrative->Text("text_act2_records_terminal"), 180);
                    }
                } else {
                    render->SetSubtitleOnce("ARCHIVE: credentialed query rejected.", 160);
                }
                return;
            }
            if (focused_scene_entity("act2_records_concourse_door")) {
                (void)enter_scene_transition("act2_records_to_concourse");
                return;
            }
            if (focused_scene_entity("act2_records_observation_door")) {
                (void)enter_scene_transition("act2_records_to_observation");
                return;
            }
            render->SetSubtitleOnce("RECORDS: ask the operator or query the manifest.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_act2_power_utility") {
            if (focused_npc(slice.power_technician_npc)) {
                const bool already_helped = fact_is_true("fact_act2_power_helped");
                RelationshipRecord relationship;
                relationship.a = EntityId::New(slice.power_technician_npc.GetValue());
                relationship.b = player;
                relationship.trust = 0.66f;
                relationship.respect = 0.62f;
                relationship.debt = 0.34f;
                const bool relationship_saved =
                    services.systemic->SetRelationship(relationship);
                slice.act2_power_helped = true;
                services.world->SetBooleanFact(
                    RuntimeFactId("fact_act2_power_helped"), player, true);
                render->SetSubtitleOnce(
                    services.narrative->Text(already_helped
                        ? "text_act2_power_helped"
                        : "text_act2_power_tech"),
                    relationship_saved ? 360 : 240);
                return;
            }
            const auto reroute_power = [&](TerminalId terminal_id,
                                           std::string_view action,
                                           std::string_view success_text,
                                           bool forced) {
                const bool was_rerouted = fact_is_true("fact_act2_power_rerouted");
                if (!use_act2_terminal(terminal_id, action, success_text)) {
                    render->SetSubtitleOnce("UTILITY: relay handshake rejected.", 160);
                    return;
                }
                slice.act2_power_rerouted = true;
                services.world->SetBooleanFact(
                    RuntimeFactId("fact_act2_power_rerouted"), player, true);
                const bool helped = fact_is_true("fact_act2_power_helped");
                if (forced || !helped) {
                    slice.act2_utility_noise = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_utility_noise"), player, true);
                }
                if (!was_rerouted) {
                    SystemicEvent reroute;
                    reroute.id = EventId::New(10000 + services.systemic->EventCount());
                    reroute.type = SystemicEventType::InfrastructureChange;
                    reroute.actor = player;
                    reroute.location = services.world->LoadedRoom().id;
                    reroute.frame = frame;
                    reroute.severity = forced || !helped ? 55 : 18;
                    reroute.legality = forced || !helped
                        ? LegalityClass::MinorOffense : LegalityClass::Legal;
                    reroute.outcome = OutcomeType::Success;
                    reroute.method = std::string(action);
                    reroute.tags.push_back("maintenance_bypass");
                    reroute.tags.push_back(forced ? "forced" : "operator_assisted");
                    (void)services.systemic->AddSystemicEvent(reroute);
                }
                if (was_rerouted) {
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_power_repeat"), 180);
                } else if (forced || !helped) {
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_transit_alert"), 220);
                } else {
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_power_rerouted"), 180);
                }
            };
            if (focused_scene_entity("act2_power_relay_terminal")) {
                reroute_power(slice.power_terminal, "reroute_reserve_feed",
                              services.narrative->Text("text_act2_power_rerouted"), false);
                return;
            }
            if (focused_scene_entity("act2_power_backup_terminal")) {
                reroute_power(slice.power_backup_terminal, "force_backup_feed",
                              services.narrative->Text("text_act2_power_rerouted"), true);
                return;
            }
            if (focused_scene_entity("act2_power_concourse_door")) {
                (void)enter_scene_transition("act2_power_to_concourse");
                return;
            }
            if (focused_scene_entity("act2_power_transit_door")) {
                (void)enter_scene_transition("act2_power_to_transit");
                return;
            }
            render->SetSubtitleOnce("UTILITY: the relay needs a human decision.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_act2_observation_gallery") {
            if (focused_npc(slice.observation_analyst_npc)) {
                const bool already_found = fact_is_true("fact_act2_observation_found");
                RelationshipRecord relationship;
                relationship.a = EntityId::New(slice.observation_analyst_npc.GetValue());
                relationship.b = player;
                relationship.trust = 0.63f;
                relationship.respect = 0.55f;
                relationship.suspicion = 0.08f;
                const bool relationship_saved =
                    services.systemic->SetRelationship(relationship);
                slice.act2_observation_found = true;
                services.world->SetBooleanFact(
                    RuntimeFactId("fact_act2_observation_found"), player, true);
                const KnowledgeAssetId camera_asset = KnowledgeAssetId::New(9302);
                if (services.systemic->GetKnowledgeAsset(camera_asset) == nullptr) {
                    KnowledgeAssetRecord asset;
                    asset.id = camera_asset;
                    asset.type = KnowledgeAssetType::CameraBlindSpot;
                    asset.source = ResourceId::New(3);
                    asset.confidence = 0.76f;
                    asset.known_by.push_back(player);
                    (void)services.systemic->AddKnowledgeAsset(asset);
                }
                render->SetSubtitleOnce(
                    already_found
                        ? "Analyst: the loop is still holding. That is the interesting part."
                        : services.narrative->Text("text_act2_observation_analyst"),
                    relationship_saved ? 360 : 240);
                return;
            }
            if (focused_scene_entity("act2_gallery_records_terminal")) {
                const bool was_looped = fact_is_true("fact_act2_camera_looped");
                if (use_act2_terminal(
                        slice.observation_terminal, "loop_transit_camera",
                        services.narrative->Text("text_act2_observation_looped"))) {
                    slice.act2_observation_found = true;
                    slice.act2_camera_looped = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_observation_found"), player, true);
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_camera_looped"), player, true);
                    if (!was_looped) {
                        services.systemic->SetObservationSourceOnline(
                            slice.transit_camera, false);
                        SystemicEvent loop;
                        loop.id = EventId::New(10000 + services.systemic->EventCount());
                        loop.type = SystemicEventType::Vandalism;
                        loop.actor = player;
                        loop.location = services.world->LoadedRoom().id;
                        loop.frame = frame;
                        loop.severity = 28;
                        loop.legality = LegalityClass::Unauthorized;
                        loop.outcome = OutcomeType::Success;
                        loop.method = "camera_loop";
                        loop.tags.push_back("temporary_blind_spot");
                        (void)services.systemic->AddSystemicEvent(loop);
                    } else {
                        render->SetSubtitleOnce(
                            services.narrative->Text("text_act2_observation_looped"), 180);
                    }
                } else {
                    render->SetSubtitleOnce("OBSERVATION: control port unavailable.", 160);
                }
                return;
            }
            if (focused_scene_entity("act2_gallery_records_door")) {
                (void)enter_scene_transition("act2_observation_to_records");
                return;
            }
            render->SetSubtitleOnce("OBSERVATION: ask the analyst or use the control port.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_act2_transit_control") {
            if (focused_npc(slice.transit_guard_npc)) {
                const bool archive_route =
                    fact_is_true("fact_act2_archive_released") ||
                    fact_is_true("fact_act2_archive_terminal_accessed");
                const bool power_route = fact_is_true("fact_act2_power_rerouted");
                const bool quiet_camera = fact_is_true("fact_act2_camera_looped");
                if (fact_is_true("fact_act2_transit_guard_down")) {
                    slice.act2_transit_guard_bypassed = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_transit_guard_bypassed"), player, true);
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_transit_guard_down"), 220);
                } else if (archive_route) {
                    slice.act2_transit_guard_bypassed = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_transit_guard_bypassed"), player, true);
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_transit_guard_quiet"), 240);
                } else if (power_route) {
                    slice.act2_transit_guard_bypassed = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_transit_guard_bypassed"), player, true);
                    const bool prior_alert = fact_is_true("fact_act2_transit_alerted");
                    const bool carryover_loud =
                        fact_is_true("fact_chapter_aggressive_route") ||
                        fact_is_true("fact_b1_loud_action");
                    if (!quiet_camera && (slice.act2_utility_noise || carryover_loud)) {
                        slice.act2_transit_alerted = true;
                        services.world->SetBooleanFact(
                            RuntimeFactId("fact_act2_transit_alerted"), player, true);
                        services.systemic->SetAlert(
                            FacilityAlertLevel::Suspicious,
                            {services.world->LoadedRoom().id}, frame);
                        render->SetSubtitleOnce(
                            services.narrative->Text("text_act2_transit_guard_alert"),
                            prior_alert ? 160 : 260);
                    } else {
                        render->SetSubtitleOnce(
                            services.narrative->Text("text_act2_transit_guard_quiet"), 240);
                    }
                } else {
                    slice.act2_access_denied = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_access_denied"), player, true);
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_transit_denied"), 200);
                }
                return;
            }
            if (focused_scene_entity("act2_transit_control_terminal")) {
                const bool authorized =
                    fact_is_true("fact_act2_transit_guard_bypassed") ||
                    fact_is_true("fact_act2_transit_guard_down");
                if (!authorized) {
                    slice.act2_access_denied = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_access_denied"), player, true);
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_transit_denied"), 200);
                } else if (use_act2_terminal(
                               slice.transit_terminal, "accept_transit_handshake",
                               services.narrative->Text("text_act2_transit_terminal"))) {
                    slice.act2_transit_controlled = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_transit_controlled"), player, true);
                    if (slice.act2_transit_alerted ||
                        fact_is_true("fact_act2_transit_alerted")) {
                        render->SetSubtitleOnce(
                            services.narrative->Text("text_act2_transit_alert"), 220);
                    }
                } else {
                    render->SetSubtitleOnce("TRANSIT: control terminal unavailable.", 160);
                }
                return;
            }
            if (focused_scene_entity("act2_transit_exit_door")) {
                if (!fact_is_true("fact_act2_transit_controlled")) {
                    slice.act2_access_denied = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_access_denied"), player, true);
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_transit_denied"), 200);
                } else if (!fact_is_true("fact_act2_checkpoint_reached")) {
                    slice.act2_checkpoint_reached = true;
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_act2_checkpoint_reached"), player, true);
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_act2_checkpoint"), 360);
                    if (audio) audio->PlaySfx(AudioId::New(7), 0.55f);
                } else {
                    (void)enter_scene_transition("act2_transit_to_arrival");
                }
                return;
            }
            if (focused_scene_entity("act2_transit_concourse_door")) {
                (void)enter_scene_transition("act2_transit_to_concourse");
                return;
            }
            if (focused_scene_entity("act2_transit_power_door")) {
                (void)enter_scene_transition("act2_transit_to_power");
                return;
            }
            render->SetSubtitleOnce(services.narrative->Text("text_act2_transit_guard"), 200);
            return;
        }

        if (services.player->CurrentRoom() == "room_elevator_lobby") {
            if (focused_scene_entity("elevator_restricted_door")) {
                slice.elevator_entry_attempted = true;
                const bool route_authorized =
                    fact_is_true("fact_chapter_staff_route") ||
                    fact_is_true("fact_chapter_security_checkpoint");
                if (!route_authorized) {
                    slice.elevator_entry_denied = true;
                    render->SetSubtitleOnce(
                        "ELEVATOR: the checkpoint record is incomplete.", 160);
                } else if (!fact_is_true("fact_chapter_checkpoint_reached")) {
                    services.world->SetBooleanFact(
                        RuntimeFactId("fact_chapter_checkpoint_reached"),
                        player, true);
                    (void)services.systemic->TransitionQuest(
                        slice.chapter_quest, QuestStatus::Completed, frame,
                        "Chapter One elevator checkpoint reached");
                    render->SetSubtitleOnce(
                        services.narrative->Text("text_elevator_complete"), 360);
                    if (audio) audio->PlaySfx(AudioId::New(7), 0.55f);
                } else {
                    (void)enter_scene_transition("elevator_to_act2_concourse");
                }
                return;
            }
            render->SetSubtitleOnce("ELEVATOR: the restricted door awaits verification.", 180);
        }

        if (services.player->CurrentRoom() == "room_1f_arrival_lobby") {
            if (focused_scene_entity("arrival_tower_directory") ||
                focused_scene_entity("arrival_lift_directory")) {
                open_campaign_directory();
                return;
            }
            if (focused_npc(NpcId::New(StableContentId("arrival_clerk")))) {
                set_campaign_fact("fact_act3_hub_reviewed");
                set_campaign_fact("fact_elevator_records_unlocked");
                render->SetSubtitleOnce(
                    "Clerk: Records is the first stop. Operations will open after you bring back a route.",
                    240, 85);
                return;
            }
            if (focused_scene_entity("arrival_lift_door")) {
                (void)enter_scene_transition("arrival_to_act2_transit");
                return;
            }
            render->SetSubtitleOnce("ARRIVAL: use the directory or speak with the clerk.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_8f_records_core") {
            if (focused_scene_entity("records_subject_terminal")) {
                if (use_act2_terminal(
                        slice.records_subject_terminal, "query_subject_07",
                        "RECORDS / SUBJECT 07: release denied before revival. Review owner: Authority, 36F.")) {
                    set_campaign_fact("fact_act3_records_accessed");
                    set_campaign_fact("fact_act3_authority_lead");
                    set_campaign_fact("fact_elevator_operations_unlocked");
                    add_campaign_knowledge(9401, KnowledgeAssetType::AccessProcedure, 0.86f);
                } else {
                    render->SetSubtitleOnce("RECORDS: the subject terminal is offline.", 160);
                }
                return;
            }
            if (focused_npc(NpcId::New(StableContentId("records_archivist")))) {
                set_campaign_fact("fact_act3_records_accessed");
                set_campaign_fact("fact_act3_authority_lead");
                set_campaign_fact("fact_elevator_operations_unlocked");
                add_campaign_knowledge(9401, KnowledgeAssetType::AccessProcedure, 0.82f);
                render->SetSubtitleOnce(
                    "Archivist: Your release was denied before you woke. Not by Medical. The order came from 36F.",
                    240, 85);
                return;
            }
            if (focused_scene_entity("records_return_door")) {
                (void)enter_scene_transition("records_to_arrival");
                return;
            }
            render->SetSubtitleOnce("RECORDS: the terminal has the useful version of the file.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_12f_operations_control") {
            const bool forced = fact_is_true("fact_chapter_aggressive_route") ||
                                fact_is_true("fact_b1_loud_action") ||
                                services.systemic->AlertLevel() >= FacilityAlertLevel::Suspicious;
            if (focused_scene_entity("operations_control_terminal")) {
                if (use_act2_terminal(
                        slice.operations_control_terminal, "reconcile_operations",
                        "OPERATIONS: response map reconciled. A quiet route is available.")) {
                    set_campaign_fact("fact_act3_operations_accessed");
                    set_campaign_fact("fact_elevator_network_unlocked");
                    if (forced) {
                        set_campaign_fact("fact_act3_force_route");
                        set_campaign_fact("fact_act4_security_alerted");
                        // Force is a valid discovery-poor fallback: an
                        // alerted operator can expose the transfer stop even
                        // when the optional Network observation was skipped.
                        set_campaign_fact("fact_elevator_transfer_unlocked");
                        render->SetSubtitleOnce(
                            "OPERATIONS: route reconciled under alert. The next guard will know.",
                            220, 90);
                    } else {
                        set_campaign_fact("fact_act3_operations_cooperated");
                    }
                    add_campaign_knowledge(9402, KnowledgeAssetType::Route,
                                           forced ? 0.62f : 0.88f);
                } else {
                    render->SetSubtitleOnce("OPERATIONS: control handshake unavailable.", 160);
                }
                return;
            }
            if (focused_npc(NpcId::New(StableContentId("operations_operator")))) {
                set_campaign_fact("fact_act3_operations_accessed");
                set_campaign_fact("fact_elevator_network_unlocked");
                if (forced) {
                    set_campaign_fact("fact_act3_force_route");
                    set_campaign_fact("fact_act4_security_alerted");
                    // Preserve a complete force route without making the
                    // optional Network discovery a hard prerequisite.
                    set_campaign_fact("fact_elevator_transfer_unlocked");
                    render->SetSubtitleOnce(
                        "Operator: I can route you upward. I cannot make the cameras forget.",
                        220, 85);
                } else {
                    set_campaign_fact("fact_act3_operations_cooperated");
                    render->SetSubtitleOnce(
                    "Operator: I've filed your transfer. Network has the feed they left out of your record.",
                        220, 85);
                }
                add_campaign_knowledge(9402, KnowledgeAssetType::Route, 0.8f);
                return;
            }
            if (focused_scene_entity("operations_return_door")) {
                (void)enter_scene_transition("operations_to_arrival");
                return;
            }
            render->SetSubtitleOnce("OPERATIONS: reconcile the map before leaving.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_18f_network_node") {
            if (focused_scene_entity("network_observation_terminal")) {
                if (use_act2_terminal(
                        slice.network_observation_terminal, "open_unlisted_feed",
                        "NETWORK: unlisted observation feed opened. The record can leave the tower.")) {
                    set_campaign_fact("fact_act3_network_discovered");
                    set_campaign_fact("fact_elevator_transfer_unlocked");
                    add_campaign_knowledge(9403, KnowledgeAssetType::CameraBlindSpot, 0.91f);
                } else {
                    render->SetSubtitleOnce("NETWORK: the observation feed is not responding.", 160);
                }
                return;
            }
            if (focused_npc(NpcId::New(StableContentId("network_analyst")))) {
                set_campaign_fact("fact_act3_network_discovered");
                set_campaign_fact("fact_elevator_transfer_unlocked");
                add_campaign_knowledge(9403, KnowledgeAssetType::CameraBlindSpot, 0.86f);
                render->SetSubtitleOnce(
                    "Analyst: That feed was omitted, not lost. Decide who gets to see it.",
                    240, 85);
                return;
            }
            if (focused_scene_entity("network_return_door")) {
                (void)enter_scene_transition("network_to_arrival");
                return;
            }
            render->SetSubtitleOnce("NETWORK: the unlisted feed is the useful discovery.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_24f_security_transfer") {
            if (focused_scene_entity("transfer_gate_terminal")) {
                const bool guard_route = fact_is_true("fact_act4_guard_bypassed") ||
                    fact_is_true("fact_act4_guard_down") ||
                    fact_is_true("fact_act3_operations_cooperated") ||
                    fact_is_true("fact_act3_force_route");
                if (!guard_route) {
                    set_campaign_fact("fact_act4_security_alerted");
                    render->SetSubtitleOnce(
                        "TRANSFER: clearance denied. The guard still owns this gate.",
                        200, 95);
                } else if (use_act2_terminal(
                               slice.transfer_gate_terminal, "clear_upper_transfer",
                               "TRANSFER: upper security gate cleared.")) {
                    set_campaign_fact("fact_act4_guard_bypassed");
                    set_campaign_fact("fact_elevator_executive_unlocked");
                    set_campaign_fact("fact_act3_upper_access");
                    if (fact_is_true("fact_act3_force_route")) {
                        set_campaign_fact("fact_act4_security_alerted");
                        services.systemic->SetAlert(
                            FacilityAlertLevel::Suspicious,
                            {services.world->LoadedRoom().id}, frame);
                    }
                    add_campaign_knowledge(9404, KnowledgeAssetType::AccessProcedure, 0.78f);
                }
                return;
            }
            if (focused_npc(NpcId::New(StableContentId("transfer_guard")))) {
                const bool forced = fact_is_true("fact_chapter_aggressive_route") ||
                                    fact_is_true("fact_b1_loud_action") ||
                                    services.systemic->AlertLevel() >= FacilityAlertLevel::Suspicious;
                if (forced) {
                    set_campaign_fact("fact_act3_force_route");
                    set_campaign_fact("fact_act4_security_alerted");
                    set_campaign_fact("fact_elevator_executive_unlocked");
                    services.systemic->SetAlert(
                        FacilityAlertLevel::Suspicious,
                        {services.world->LoadedRoom().id}, frame);
                    render->SetSubtitleOnce(
                        "Transfer Guard: I have your route. You can still force the next door.",
                        220, 95);
                } else {
                    set_campaign_fact("fact_act4_guard_bypassed");
                    set_campaign_fact("fact_elevator_executive_unlocked");
                    set_campaign_fact("fact_act3_upper_access");
                    render->SetSubtitleOnce(
                        "Transfer Guard: Your route is filed. Do not make me revise it.",
                        220, 85);
                }
                return;
            }
            if (focused_scene_entity("transfer_return_door")) {
                (void)enter_scene_transition("transfer_to_arrival");
                return;
            }
            render->SetSubtitleOnce("TRANSFER: clear the guard or make the route loud.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_30f_executive_archive") {
            if (focused_scene_entity("executive_archive_terminal")) {
                const bool upper_access = fact_is_true("fact_act3_upper_access") ||
                    fact_is_true("fact_act3_force_route") ||
                    fact_is_true("fact_act4_guard_bypassed");
                if (!upper_access) {
                    render->SetSubtitleOnce("ARCHIVE: the executive record rejects this route.", 180, 95);
                } else if (use_act2_terminal(
                               slice.executive_archive_terminal, "open_executive_record",
                               "ORDER 07: hold subject unresolved until they certify their own account. Review authority transfers to Subject 07.")) {
                    set_campaign_fact("fact_act4_archive_opened");
                    set_campaign_fact("fact_act4_authority_ready");
                    set_campaign_fact("fact_elevator_authority_unlocked");
                    add_campaign_knowledge(9405, KnowledgeAssetType::Secret, 0.9f);
                }
                return;
            }
            if (focused_npc(NpcId::New(StableContentId("executive_liaison")))) {
                if (fact_is_true("fact_act3_upper_access") ||
                    fact_is_true("fact_act3_force_route")) {
                    set_campaign_fact("fact_act4_archive_opened");
                    set_campaign_fact("fact_act4_authority_ready");
                    set_campaign_fact("fact_elevator_authority_unlocked");
                    add_campaign_knowledge(9405, KnowledgeAssetType::Secret, 0.82f);
                    render->SetSubtitleOnce(
                        "Liaison: They need your signature to close the file. The order gives you authority to write it. Read it carefully.",
                        240, 85);
                } else {
                    render->SetSubtitleOnce("Liaison: The archive is not open to an unfiled route.", 180, 90);
                }
                return;
            }
            if (focused_scene_entity("executive_return_door")) {
                (void)enter_scene_transition("executive_to_arrival");
                return;
            }
            render->SetSubtitleOnce("ARCHIVE: the sealed record is the focal point.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_36f_authority_core") {
            if (focused_scene_entity("authority_decision_terminal")) {
                if (!fact_is_true("fact_act4_authority_ready")) {
                    render->SetSubtitleOnce("AUTHORITY: the executive record is not yet reconciled.", 180, 95);
                } else if (!fact_is_true("fact_pre_final_checkpoint")) {
                    set_campaign_fact("fact_pre_final_checkpoint");
                    save_role = ProductSaveRole::PreFinal;
                    services.player->RequestSave();
                    render->SetSubtitleOnce(
                        "PRE-FINAL CHECKPOINT / The decision will be durable after you confirm it.",
                        300, 105);
                } else {
                    open_final_decision();
                }
                return;
            }
            if (focused_npc(NpcId::New(StableContentId("authority_presence")))) {
                set_campaign_fact("fact_act4_authority_ready");
                set_campaign_fact("fact_elevator_authority_unlocked");
                render->SetSubtitleOnce(
                    "Authority: I can certify an account. I cannot make it what happened. The signature is yours.",
                    240, 90);
                return;
            }
            if (focused_scene_entity("authority_return_door")) {
                (void)enter_scene_transition("authority_to_arrival");
                return;
            }
            render->SetSubtitleOnce("AUTHORITY: use the decision terminal when ready.", 160);
            return;
        }

        if (services.player->CurrentRoom() == "room_roof_exit") {
            if (focused_scene_entity("roof_epilogue_marker")) {
                set_campaign_fact("fact_roof_reached");
                render->SetSubtitleOnce(services.narrative->Text(
                    fact_is_true("fact_ending_disclose") ? "text_roof_disclose"
                    : fact_is_true("fact_ending_breach") ? "text_roof_breach" : "text_roof_amend"), 360, 100);
                return;
            }
            render->SetSubtitleOnce("ROOF: the campaign is complete.", 180, 90);
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
        render->SetSubtitleOnce(
            services.player->Paused()
                ? "PAUSED - ESC resumes; Q quits"
                : "RESUMED",
            120);
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
    services.player->SetQuitCallback([&] {
        slice.normal_quit_requested = true;
        render->SetSubtitleOnce("QUIT REQUESTED - restoring terminal", 60);
        engine.RequestStop();
    });
    const auto save_directory = user_data_root / "saves";
    product.continue_available = ProductSaveEnvelopeValid(save_directory / ProductResumeName(save_directory));
    product.checkpoint_available = ProductSaveEnvelopeValid(save_directory / "pvs_checkpoint");
    product.pre_final_available = ProductSaveEnvelopeValid(save_directory / "pvs_pre_final");
    if (config.skip_boot) {
        const bool fresh = services.player->CurrentRoom() == "room_b1_revival" && !fact_is_true("fact_campaign_completed") &&
            services.player->Health() == 100 && !services.player->Dead() && PlayerKnownEvidence(*services.systemic, slice.player) == 0;
        std::fprintf(stderr, "PRODUCT_NEW_GAME_INITIAL_STATE=%s ROOM=%s HEALTH=%u EVIDENCE=%zu\n",
            fresh ? "PASS" : "FAIL", services.player->CurrentRoom().c_str(), services.player->Health(), PlayerKnownEvidence(*services.systemic, slice.player));
    }
    if (!config.skip_boot && !config.smoke && config.replay_path.empty() && config.room_id.empty() && !config.camera_override) {
        product.Open(settings.language.empty() ? ProductPage::Language : ProductPage::Boot);
        time_gate.SetPaused(true);
    } else product.boot_context = false;
    engine.SetRenderModule(render.get());

    const int result = engine.Run(config.max_frames);
    events.Unregister(player_damage_consumer);
    events.Unregister(speech_consumer);
    if (new_game_requested) return 10;

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
        const bool b1_checkpoint_reached = slice.gate_crossed &&
                                            services.player->CurrentRoom() ==
                                                "room_01_calibration";
        const bool chapter_checkpoint_reached =
            fact_is_true("fact_chapter_checkpoint_reached") &&
            services.player->CurrentRoom() == "room_elevator_lobby";
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
        const bool normal_quit_replay =
            config.replay_path.find("normal_quit") != std::string::npos;
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
        const bool chapter_systemic_replay =
            config.replay_path.find("chapter01_systemic") != std::string::npos;
        const bool chapter_aggressive_replay =
            config.replay_path.find("chapter01_aggressive") != std::string::npos;
        const bool chapter_denied_replay =
            config.replay_path.find("chapter01_denied_or_blocked") !=
            std::string::npos;
        const bool chapter_memory_replay =
            config.replay_path.find("chapter01_memory_consequence") !=
            std::string::npos;
        const bool chapter_mid_save_replay =
            config.replay_path.find("chapter01_mid_save_load") !=
            std::string::npos;
        const bool chapter_backtrack_replay =
            config.replay_path.find("chapter01_backtrack") != std::string::npos;
        const bool chapter_security_bypass_replay =
            config.replay_path.find("chapter01_security_bypass") != std::string::npos;
        const bool chapter_no_save_death_replay =
            config.replay_path.find("chapter01_no_save_death") != std::string::npos;
        const bool chapter_terminal_skip_replay =
            config.replay_path.find("chapter01_terminal_skip_denied") !=
            std::string::npos;
        const bool scenario_guard_other_room_replay =
            config.replay_path.find("scenario_guard_other_room") !=
            std::string::npos;
        const bool scenario_camera_offline_replay =
            config.replay_path.find("scenario_camera_offline") !=
            std::string::npos;
        const bool scenario_dead_body_hidden_replay =
            config.replay_path.find("scenario_dead_body_hidden") !=
            std::string::npos;
        const bool scenario_terminal_badge_revoked_replay =
            config.replay_path.find("scenario_terminal_active_badge_revoked") !=
            std::string::npos;
        const bool scenario_durable_history_replay =
            config.replay_path.find("scenario_player_restarted_with_durable_history") !=
            std::string::npos;
        const bool scenario_wall_blocked_los_replay =
            config.replay_path.find("scenario_wall_blocked_guard_los") !=
            std::string::npos;
        const bool scenario_elevator_without_route_replay =
            config.replay_path.find("scenario_elevator_without_route") !=
            std::string::npos;
        const bool campaign_amend_replay =
            config.replay_path.find("campaign_probe_amend") != std::string::npos;
        const bool campaign_disclose_replay =
            config.replay_path.find("campaign_probe_disclose") != std::string::npos;
        const bool campaign_breach_replay =
            config.replay_path.find("campaign_probe_breach") != std::string::npos ||
            config.replay_path.find("campaign_probe_discovery_poor") !=
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
        const bool durable_history_preserved =
            slice.player_restarted && cleaner_relationship_established;
        const ItemRecord* badge_record = slice.badge.IsValid()
                                              ? services.systemic->GetItem(slice.badge)
                                              : nullptr;
        const bool badge_revoked =
            (slice.badge_revoked ||
             (badge_record != nullptr && badge_record->revoked));
        const BodyRecord* body = services.systemic->GetBody(slice.body);
        const bool dead_body_hidden =
            body != nullptr && body->status == BodyStatus::Dead &&
            body->disposition == BodyDisposition::HiddenInContainer;
        const bool guard_line_of_sight_blocked = [&] {
            if (services.player->CurrentRoom() != "room_1f_security") return false;
            for (const auto& runtime : services.ai->Npcs()) {
                if (runtime.instance.id != slice.security_guard_npc ||
                    runtime.room != services.world->LoadedRoom().id ||
                    runtime.instance.state == NPCState::Dead ||
                    runtime.instance.state == NPCState::Stunned) {
                    continue;
                }
                const Vec3 guard_eye{
                    runtime.instance.position.x, runtime.instance.position.y,
                    runtime.instance.position.z +
                        GetPostureParams(Posture::Stand).eye_height};
                const Vec3 player_eye = services.player->Locomotion().EyePosition();
                return !services.world->Query().LineOfSight(
                    guard_eye, player_eye, player_eye.z);
            }
            return false;
        }();
        const bool narrative_visible_action =
            services.narrative->PresentedActionCount() > 0 &&
            !services.narrative->LastPresentedText().empty();
        const bool quest_presented_during_route = render->ObjectiveWasPresented();
        const bool quest_presentation_visible = render->ObjectiveVisible();
        const bool b1_loud_action = fact_is_true("fact_b1_loud_action");
        const bool b1_camera_offline = fact_is_true("fact_b1_camera_disabled");
        const char* camera_surveillance_response =
            !b1_loud_action ? "NO_LOUD_ACTION" :
            b1_camera_offline ? "BLIND_SPOT" :
            static_cast<uint8_t>(services.systemic->AlertLevel()) >=
                    static_cast<uint8_t>(FacilityAlertLevel::Suspicious)
                ? "SECURITY_ALERT" : "NO_ALERT";
        const QuestRecord* chapter_quest =
            services.systemic->GetQuest(slice.chapter_quest);
        const bool chapter_quest_completed =
            chapter_quest != nullptr && chapter_quest->status == QuestStatus::Completed;
        const auto visited_room = [&](std::string_view room) {
            return std::find(replay_route.begin(), replay_route.end(), room) !=
                   replay_route.end();
        };
        const bool backtracked_medical = [&] {
            for (size_t i = 1; i + 1 < replay_route.size(); ++i) {
                if (replay_route[i] == "room_service_medical" &&
                    replay_route[i - 1] == "room_01_calibration" &&
                    replay_route[i + 1] == "room_01_calibration") {
                    return true;
                }
            }
            return false;
        }();
        const bool chapter_route_complete =
            chapter_checkpoint_reached && chapter_quest_completed &&
            fact_is_true("fact_chapter_calibration_accessed") &&
            fact_is_true("fact_chapter_medical_assessed") &&
            narrative_visible_action && quest_presented_during_route &&
            visited_room("room_01_calibration") &&
            visited_room("room_service_medical") &&
            visited_room("room_elevator_lobby");
        const bool campaign_route_complete =
            fact_is_true("fact_campaign_completed") &&
            visited_room("room_1f_arrival_lobby") &&
            visited_room("room_8f_records_core") &&
            visited_room("room_12f_operations_control") &&
            (visited_room("room_18f_network_node") ||
             fact_is_true("fact_act3_force_route")) &&
            visited_room("room_24f_security_transfer") &&
            visited_room("room_30f_executive_archive") &&
            visited_room("room_36f_authority_core") &&
            visited_room("room_roof_exit") && replay_save_ok && replay_load_ok &&
            !slice.transition_denied && !services.player->Dead();
        const bool campaign_replay =
            config.replay_path.find("campaign_") != std::string::npos ||
            visited_room("room_roof_exit");
        const bool campaign_ending_reached =
            campaign_amend_replay
                ? fact_is_true("fact_ending_amend")
                : campaign_disclose_replay
                    ? fact_is_true("fact_ending_disclose")
                    : campaign_breach_replay
                        ? fact_is_true("fact_ending_breach")
                        : true;
        const bool campaign_end_screen_ready =
            services.player->CurrentRoom() == "room_roof_exit" &&
            fact_is_true("fact_campaign_completed") &&
            (fact_is_true("fact_ending_amend") ||
             fact_is_true("fact_ending_disclose") ||
             fact_is_true("fact_ending_breach"));
        const bool campaign_reload_replay =
            std::filesystem::path(config.replay_path).filename() == "campaign_completed_reload.txt";
        const int recorded_endings = static_cast<int>(fact_is_true("fact_ending_amend")) +
            static_cast<int>(fact_is_true("fact_ending_disclose")) +
            static_cast<int>(fact_is_true("fact_ending_breach"));
        const bool campaign_reload_complete = campaign_reload_replay && replay_load_ok &&
            campaign_end_screen_ready && fact_is_true("fact_roof_reached") &&
            recorded_endings == 1 && !services.player->Dead();
        const bool product_probe = config.replay_path.find("campaign_probe_amend_product") != std::string::npos;
        const bool product_probe_ok = product.Visited(ProductPage::Inspect) && product.Visited(ProductPage::History) &&
            product.Visited(ProductPage::Dialogue) && product.Visited(ProductPage::Ending) && product.preference_changes > 0 &&
            !product.Active() && replay_save_ok && replay_load_ok && product.feed.Size() < PerceptionFeed::kCapacity;
        if (product_probe) std::fprintf(stderr, "PRODUCT_INTEGRATED_PROBE=%s UI_PAGES=%u PREFERENCE_CHANGES=%zu HISTORY_SIZE=%zu\n",
            product_probe_ok ? "PASS" : "FAIL", product.visited_pages, product.preference_changes, product.feed.Size());
        const bool expected_state_reached = (product_probe ? product_probe_ok : true) && (normal_quit_replay
            ? slice.normal_quit_requested
            : campaign_reload_replay ? campaign_reload_complete
            : campaign_replay
                ? (campaign_route_complete && campaign_ending_reached &&
                   campaign_end_screen_ready)
            : scenario_guard_other_room_replay
                ? (services.player->CurrentRoom() == "room_service_medical" &&
                   services.player->Health() == 100 &&
                   services.ai->GuardAttackCount() == 0)
            : scenario_camera_offline_replay
                ? fact_is_true("fact_b1_camera_disabled")
            : scenario_dead_body_hidden_replay
                ? (dead_body_hidden && slice.body_hidden && !slice.nonlethal_hit &&
                   badge_held_by_player)
            : scenario_terminal_badge_revoked_replay
                ? (slice.terminal_session && slice.terminal_denied &&
                   badge_revoked && !slice.gate_open && !slice.gate_crossed)
            : scenario_durable_history_replay
                ? (slice.player_died && slice.player_restarted &&
                   durable_history_preserved && !services.player->Dead() &&
                   !replay_load_ok)
            : scenario_wall_blocked_los_replay
                ? (guard_line_of_sight_blocked &&
                   services.ai->GuardAttackCount() == 0 &&
                   services.player->Health() == 100)
            : scenario_elevator_without_route_replay
                ? (services.player->CurrentRoom() == "room_elevator_lobby" &&
                   slice.elevator_entry_attempted &&
                   slice.elevator_entry_denied &&
                   !fact_is_true("fact_chapter_checkpoint_reached"))
            : chapter_systemic_replay
            ? (chapter_route_complete && fact_is_true("fact_chapter_quiet_route") &&
               fact_is_true("fact_chapter_staff_route") &&
               visited_room("room_restroom_staff") && replay_save_ok && replay_load_ok)
            : chapter_aggressive_replay
                ? (chapter_route_complete &&
                   fact_is_true("fact_chapter_aggressive_route") &&
                   fact_is_true("fact_chapter_security_checkpoint") &&
                   visited_room("room_1f_security") && slice.shot_hit &&
                   !slice.nonlethal_hit && slice.body_created && badge_held_by_player &&
                   replay_save_ok && replay_load_ok)
            : chapter_denied_replay
                ? (slice.access_attempted && slice.access_denied &&
                   !slice.gate_open && !slice.gate_crossed &&
                   !chapter_checkpoint_reached)
            : chapter_memory_replay
                ? (chapter_route_complete &&
                   cleaner_relationship_established && cleaner_helped_coverup &&
                   slice.body_hidden && slice.body_discovered &&
                   slice.cleaner_response && replay_save_ok && replay_load_ok)
            : chapter_mid_save_replay
                ? (chapter_route_complete && replay_save_ok && replay_load_ok &&
                   visited_room("room_restroom_staff"))
            : chapter_backtrack_replay
                 ? (chapter_route_complete && fact_is_true("fact_chapter_quiet_route") &&
                    fact_is_true("fact_chapter_staff_route") && backtracked_medical)
             : chapter_security_bypass_replay
                 ? (chapter_route_complete &&
                    fact_is_true("fact_chapter_security_checkpoint") &&
                    visited_room("room_1f_security") &&
                    visited_room("room_elevator_lobby") &&
                    !slice.transition_denied)
             : chapter_no_save_death_replay
                 ? (slice.player_died && slice.player_restarted &&
                    !replay_load_ok && services.player->Health() > 0 &&
                    !services.player->Dead())
             : chapter_terminal_skip_replay
                 ? (slice.transition_denied && slice.gate_open &&
                    !slice.gate_crossed && badge_held_by_player &&
                    !slice.terminal_session &&
                    !visited_room("room_01_calibration"))
             : (success_replay || alpha_systemic_replay)
            ? (slice.shot_hit && slice.nonlethal_hit && slice.body_created &&
               slice.body_hidden && slice.body_discovered && slice.cleaner_response &&
               slice.terminal_session && slice.gate_open &&
               narrative_visible_action && quest_presented_during_route &&
               replay_save_ok && replay_load_ok && b1_checkpoint_reached)
            : alpha_aggressive_replay
                ? (slice.shot_hit && !slice.nonlethal_hit && slice.body_created &&
                   badge_held_by_player && slice.gate_open && slice.gate_crossed &&
                   narrative_visible_action && quest_presented_during_route &&
                   b1_checkpoint_reached)
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
                   !slice.terminal_session && !b1_checkpoint_reached)
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
                               !b1_checkpoint_reached)
                            : false);
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
        std::fprintf(stderr, "CAMPAIGN_COMPLETION_REACHED=%s\n",
                     campaign_route_complete || campaign_reload_complete ? "YES" : "NO");
        std::fprintf(stderr, "CAMPAIGN_END_SCREEN_READY=%s\n",
                     campaign_end_screen_ready ? "YES" : "NO");
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
                     "CAMPAIGN_FACTS records=%s operations=%s operations_cooperated=%s "
                     "network=%s force=%s transfer=%s guard_down=%s guard_bypassed=%s "
                     "alerted=%s archive=%s authority=%s pre_final=%s completed=%s "
                     "roof=%s amend=%s disclose=%s breach=%s\n",
                     fact_is_true("fact_act3_records_accessed") ? "YES" : "NO",
                     fact_is_true("fact_act3_operations_accessed") ? "YES" : "NO",
                     fact_is_true("fact_act3_operations_cooperated") ? "YES" : "NO",
                     fact_is_true("fact_act3_network_discovered") ? "YES" : "NO",
                     fact_is_true("fact_act3_force_route") ? "YES" : "NO",
                     fact_is_true("fact_act4_transfer_reached") ? "YES" : "NO",
                     fact_is_true("fact_act4_guard_down") ? "YES" : "NO",
                     fact_is_true("fact_act4_guard_bypassed") ? "YES" : "NO",
                     fact_is_true("fact_act4_security_alerted") ? "YES" : "NO",
                     fact_is_true("fact_act4_archive_opened") ? "YES" : "NO",
                     fact_is_true("fact_act4_authority_ready") ? "YES" : "NO",
                     fact_is_true("fact_pre_final_checkpoint") ? "YES" : "NO",
                     fact_is_true("fact_campaign_completed") ? "YES" : "NO",
                     fact_is_true("fact_roof_reached") ? "YES" : "NO",
                     fact_is_true("fact_ending_amend") ? "YES" : "NO",
                     fact_is_true("fact_ending_disclose") ? "YES" : "NO",
                     fact_is_true("fact_ending_breach") ? "YES" : "NO");
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
        std::fprintf(stderr,
                     "TRANSITION_DENIED=%s PLAYER_RESTARTED=%s "
                     "NORMAL_QUIT_REQUESTED=%s "
                     "FACT_R1_GUARD_DEAD=%s FACT_CHAPTER_SECURITY_CHECKPOINT=%s "
                     "FACT_B1_CAMERA_DISABLED=%s FACT_CHAPTER_MEDICAL_ASSESSED=%s "
                     "FACT_CHAPTER_QUIET_ROUTE=%s FACT_CHAPTER_AGGRESSIVE_ROUTE=%s "
                     "FACT_CHAPTER_STAFF_ROUTE=%s FACILITY_ALERT_LEVEL=%u "
                     "CAMERA_SURVEILLANCE_RESPONSE=%s "
                     "GUARD_ATTACKS=%zu BACKTRACK_MEDICAL=%s\n",
                     slice.transition_denied ? "YES" : "NO",
                     slice.player_restarted ? "YES" : "NO",
                     slice.normal_quit_requested ? "YES" : "NO",
                     fact_is_true("fact_r1_guard_dead") ? "YES" : "NO",
                     fact_is_true("fact_chapter_security_checkpoint") ? "YES" : "NO",
                     fact_is_true("fact_b1_camera_disabled") ? "YES" : "NO",
                     fact_is_true("fact_chapter_medical_assessed") ? "YES" : "NO",
                     fact_is_true("fact_chapter_quiet_route") ? "YES" : "NO",
                     fact_is_true("fact_chapter_aggressive_route") ? "YES" : "NO",
                     fact_is_true("fact_chapter_staff_route") ? "YES" : "NO",
                     static_cast<unsigned>(services.systemic->AlertLevel()),
                     camera_surveillance_response,
                     services.ai->GuardAttackCount(),
                     backtracked_medical ? "YES" : "NO");
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
                            "PLAYER_RECOVERED=%s PLAYER_RESTARTED=%s\n",
                     slice.terminal_attempted ? "YES" : "NO",
                     slice.terminal_denied ? "YES" : "NO",
                     slice.cleaner_response ? "YES" : "NO",
                     slice.player_died ? "YES" : "NO",
                     slice.player_recovered ? "YES" : "NO",
                     slice.player_restarted ? "YES" : "NO");
        std::fprintf(stderr, "NARRATOR_TYPOGRAPHY_ACTIVE=%s\n",
                     render->NarratorTypographyActive() ? "YES" : "NO");
        std::fprintf(stderr, "NARRATIVE_VISIBLE_ACTION=%s\n",
                     narrative_visible_action ? "YES" : "NO");
        std::fprintf(stderr, "NARRATIVE_LAST_TEXT=%s\n",
                     services.narrative->LastPresentedText().c_str());
        std::fprintf(stderr, "QUEST_PRESENTED_DURING_ROUTE=%s\n",
                     quest_presented_during_route ? "YES" : "NO");
        std::fprintf(stderr, "QUEST_PRESENTATION_VISIBLE=%s OBJECTIVE=%s\n",
                     quest_presentation_visible ? "YES" : "NO",
                     render->ObjectiveText().c_str());
        if (body != nullptr) {
            const ItemRecord* badge = slice.badge.IsValid()
                                          ? services.systemic->GetItem(slice.badge)
                                          : nullptr;
            std::fprintf(stderr,
                         "BODY_STATUS=%s BODY_STATE=disposition_%u drag_%u searched_%s "
                         "BADGE_HOLDER=%llu TERMINAL_SESSION=%s\n",
                         body->status == BodyStatus::Dead ? "DEAD" :
                         body->status == BodyStatus::Unconscious ? "UNCONSCIOUS" :
                         body->status == BodyStatus::Injured ? "INJURED" : "ALIVE",
                         static_cast<unsigned>(body->disposition),
                         static_cast<unsigned>(body->drag_status),
                         body->searched ? "YES" : "NO",
                         static_cast<unsigned long long>(
                         badge != nullptr && badge->current_holder.IsValid()
                                 ? badge->current_holder.GetValue()
                                 : 0),
                         slice.terminal_session ? "YES" : "NO");
        }
        std::fprintf(stderr, "BADGE_REVOKED=%s DURABLE_HISTORY_PRESERVED=%s\n",
                     badge_revoked ? "YES" : "NO",
                     durable_history_preserved ? "YES" : "NO");
        std::fprintf(stderr,
                     "GUARD_LINE_OF_SIGHT_BLOCKED=%s ELEVATOR_ENTRY_ATTEMPTED=%s "
                     "ELEVATOR_ENTRY_DENIED=%s\n",
                     guard_line_of_sight_blocked ? "YES" : "NO",
                     slice.elevator_entry_attempted ? "YES" : "NO",
                     slice.elevator_entry_denied ? "YES" : "NO");
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
