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
#include "writeover/render/hud.h"
#include "writeover/render/raycaster.h"
#include "writeover/render/reference_renderer.h"
#include "writeover/render/production_renderer.h"
#include "writeover/render/terminal_backend.h"
#include "writeover/world/fact_belief.h"
#include "writeover/world/grid.h"
#include "writeover/world/infrastructure.h"
#include "writeover/world/room.h"

#include "src/app/composition_root.h"
#include "src/app/runtime_paths.h"
#include "writeover/platform/platform_api.h"

#include <cstdint>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <functional>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace writeover {

class AiModule final : public IEngineModule {
public:
    void Init(const EngineContext& ctx) override {
        ctx_ = ctx;
        runtime_.Attach(systemic_, ctx.events, ctx.sim_rng);
    }
    void Shutdown() override {}
    void SimTick(const SimClock& clock) override {
        if (player_position_source_) runtime_.SetPlayerPose(
            player_position_source_(), player_eye_source_ ? player_eye_source_() : kEyeStand);
        runtime_.Tick(clock.FrameCount());
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
    bool AddNpc(const NPCInstance& npc, RoomId room) { return runtime_.AddNpc(npc, room); }
    bool ConfigureBodyDiscovery(NpcId cleaner, EntityId body, ContainerId container,
                                uint64_t due_frame) {
        return runtime_.ConfigureBodyDiscovery(cleaner, body, container, due_frame);
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
    void Save(Serializer& s) const { runtime_.Save(s); }
    bool Load(Deserializer& d) { return runtime_.Load(d); }
    const char* Name() const override { return "ai"; }

private:
    EngineContext ctx_{};
    SystemicWorld* systemic_ = nullptr;
    AutonomousNpcSystem runtime_;
    std::function<Vec3()> player_position_source_;
    std::function<float()> player_eye_source_;
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
            }
            if (room.IsOk()) {
                loaded_room_ = room.Value();
                query_ = std::make_unique<GridWorldQuery>(&loaded_room_.grid);
            }
        }
        if (query_ == nullptr) {
            synthetic_grid_ = Grid(16, 12);
            query_ = std::make_unique<GridWorldQuery>(&synthetic_grid_);
        }
    }

    void Shutdown() override {}

    const IWorldQuery& Query() const { return *query_; }
    FactStore& Facts() { return facts_; }
    InfrastructureSystem& Infra() { return infra_; }
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
        return true;
    }
    Room& LoadedRoom() { return loaded_room_; }
    bool HasLoadedRoom() const {
        return query_ != nullptr && loaded_room_.grid.Width() > 0 &&
               loaded_room_.grid.Height() > 0;
    }

    const GridCell* GridCells() const {
        return HasLoadedRoom() ? loaded_room_.grid.Data().data() : nullptr;
    }
    int GridWidth() const {
        return HasLoadedRoom() ? loaded_room_.grid.Width() : 0;
    }
    int GridHeight() const {
        return HasLoadedRoom() ? loaded_room_.grid.Height() : 0;
    }

    void PushCommand(WorldCommand cmd) { commands_.push_back(std::move(cmd)); }

    void SimTick(const SimClock& clock) override {
        const uint64_t frame = clock.FrameCount();
        for (auto& cmd : commands_) {
            ApplyCommand(cmd, frame);
        }
        commands_.clear();
    }

    const char* Name() const override { return "world"; }

private:
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
    Room loaded_room_;
    Grid synthetic_grid_{16, 12};
    std::unique_ptr<GridWorldQuery> query_;
    FactStore facts_;
    InfrastructureSystem infra_;
    std::vector<WorldCommand> commands_;
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
        const uint64_t frame = clock.FrameCount();
        current_frame_ = frame;

        if (input_.action_pressed[static_cast<size_t>(GameAction::Pause)]) {
            paused_ = !paused_;
            if (pause_callback_) pause_callback_();
        }
        if (paused_) return;

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

        combat_.aiming = input_.action_down[static_cast<size_t>(GameAction::AimDownSights)] &&
                         !dragging;
        if (!dragging && !weapon_restricted) {
            if (input_.action_pressed[static_cast<size_t>(GameAction::WeaponSlot1)]) {
                combat_.slot = WeaponSlot::Pistol;
            } else if (input_.action_pressed[static_cast<size_t>(GameAction::WeaponSlot2)]) {
                combat_.slot = WeaponSlot::Smg;
            } else if (input_.action_pressed[static_cast<size_t>(GameAction::WeaponSlot3)]) {
                combat_.slot = WeaponSlot::Stunner;
            }
        }

        if (!weapon_restricted &&
            input_.action_pressed[static_cast<size_t>(GameAction::Fire)]) {
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
    bool paused_ = false;
    uint64_t current_frame_ = 0;
};

class NarrativeModule final : public IEngineModule {
public:
    void Init(const EngineContext& ctx) override {
        ctx_ = ctx;
        if (!ctx_.data_dir.empty()) {
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

    void SimTick(const SimClock& clock) override {
        if (facts_ == nullptr) return;
        const uint64_t frame = clock.FrameCount();
        const Storylet* s = engine_.SelectEligible(
            *facts_, {}, {}, {}, 1, frame);
        if (s != nullptr) {
            engine_.MarkFired(s->id);
            SubtitleLine line;
            line.text = "storylet-fired:" + s->text_id;
            line.start_frame = static_cast<uint32_t>(frame);
            line.ttl_frames = 240;
            line.speaker_id = NarratorSpeakerId();
            line.persona = 1;
            queue_.Push(line);
            // Real causality: the ledger entry references the storylet event.
            ledger_.Push(CausalityEntry{
                EventId::New(s->id.GetValue()),
                EventId::Invalid(),  // root of a causality chain
                frame, EventKind::Notification});
        }
        queue_.Advance(static_cast<uint32_t>(frame));
        // No fake frame->frame-1 causality chains (F-07 closure).
        // The ledger is populated only by real storylet/event occurrences.
    }

    const char* Name() const override { return "narrative"; }

private:
    EngineContext ctx_{};
    StoryletEngine engine_;
    DialogueQueue queue_;
    CausalityLedger ledger_;
    const FactStore* facts_ = nullptr;
};

class RenderModule final : public IRenderModule {
public:
    RenderModule(std::unique_ptr<ITerminalBackend> backend, int w, int h)
        : backend_(std::move(backend)),
          width_(w),
          height_(h) {
        body_.assign(static_cast<size_t>(w) * h, CharCell{});
        logical_pixels_.assign(static_cast<size_t>(w) * (h * 2), Color{});
        backend_->Init(w, h);
    }

    void SetPlayerView(const Vec3& pos, float yaw) {
        player_pos_ = pos;
        player_yaw_ = yaw;
    }
    // Per-frame source of truth: the renderer reads the player's live
    // locomotion state instead of a one-time snapshot (F-23 closure).
    void SetCombatSource(const CombatState* combat) { combat_ = combat; }
    void SetNpcSource(const std::vector<RuntimeNpc>* npcs) { npcs_ = npcs; }
    void SetSettingsSource(const Settings* settings) { settings_ = settings; }
    void SetSceneId(const std::string& id) { scene_id_ = id; }
    void SetDebugOverlay(bool enabled) { debug_overlay_ = enabled; }
    void SetSubtitleOnce(const std::string& text, uint64_t frames) { subtitle_override_ = text; subtitle_override_remaining_ = frames; }
    void TriggerNarratorIntrusion(uint64_t frames) {
        narrator_intrusion_remaining_ = frames;
        narrator_intrusion_total_ = frames;
    }
    bool NarratorTypographyActive() const { return narrator_intrusion_remaining_ > 0; }
    bool DebugOverlay() const { return debug_overlay_; }

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
    const std::vector<Color>& LogicalPixels() const { return logical_pixels_; }

    void SetGridData(const GridCell* cells, int w, int h) {
        grid_cells_ = cells;
        grid_w_ = w;
        grid_h_ = h;
    }

    void RenderFrame(uint64_t frame_index, float alpha) override {
        (void)alpha;
        std::fill(logical_pixels_.begin(), logical_pixels_.end(), Color{0, 0, 0});
        std::fill(body_.begin(), body_.end(), CharCell{});
        if (locomotion_ != nullptr) {
            player_pos_ = locomotion_->position;
            player_yaw_ = locomotion_->yaw;
        }
        if (grid_cells_ != nullptr && grid_w_ > 0 && grid_h_ > 0) {
            // Production half-block pixel framebuffer path.
            ProductionView view;
            view.origin = Vec3{player_pos_.x, player_pos_.y,
                               locomotion_ != nullptr
                                   ? locomotion_->EyePosition().z
                                   : kEyeStand};
            view.yaw = player_yaw_;
            view.pitch = locomotion_ != nullptr ? locomotion_->pitch : 0.0f;
            const int logical_h = height_ * 2;
            const float focal =
                0.5f * static_cast<float>(logical_h) /
                std::tan(60.0f * 3.14159265f / 360.0f);
            RenderProductionFrame(grid_cells_, grid_w_, grid_h_, view,
                                  logical_pixels_.data(), width_, logical_h,
                                  focal);
            if (scene_id_ == "room_b1_revival") {
                if (npcs_ != nullptr) {
                    for (const auto& runtime : *npcs_) {
                        if (runtime.instance.state == NPCState::Dead) continue;
                        const Color tint = runtime.instance.cognition == CognitionTier::Full
                                                ? Color{196, 118, 188}
                                                : Color{126, 154, 170};
                        DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                             runtime.instance.position, 1.7f,
                                             ProductionSpriteKind::Npc, tint,
                                             grid_cells_, grid_w_, grid_h_,
                                             logical_pixels_.data(), width_, logical_h, focal);
                    }
                }
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{18.5f, 4.5f, 0.0f}, 1.4f,
                                      ProductionSpriteKind::Terminal, Color{56, 178, 164},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{14.0f, 3.0f, 2.2f}, 0.55f,
                                      ProductionSpriteKind::Camera, Color{86, 116, 128},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{15.5f, 4.0f, 0.0f}, 1.0f,
                                      ProductionSpriteKind::Crate, Color{150, 102, 48},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
            } else if (scene_id_ == "room_01_calibration") {
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{7.5f, 5.5f, 0.0f}, 1.4f,
                                      ProductionSpriteKind::Terminal, Color{56, 178, 164},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{11.5f, 4.5f, 0.0f}, 1.0f,
                                      ProductionSpriteKind::Sign, Color{184, 126, 44},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
            } else if (scene_id_ == "room_1f_security") {
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{15.5f, 8.5f, 0.0f}, 1.5f,
                                      ProductionSpriteKind::Npc, Color{142, 150, 164},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{20.5f, 8.5f, 0.0f}, 2.2f,
                                      ProductionSpriteKind::Door, Color{72, 122, 132},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{5.5f, 4.5f, 0.0f}, 1.1f,
                                      ProductionSpriteKind::Sign, Color{184, 126, 44},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
            } else if (scene_id_ == "room_service_medical") {
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{8.5f, 5.5f, 0.0f}, 1.4f,
                                      ProductionSpriteKind::Medical, Color{74, 164, 158},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
            } else if (scene_id_ == "room_restroom_staff") {
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{13.5f, 5.5f, 0.0f}, 1.8f,
                                      ProductionSpriteKind::Door, Color{92, 122, 132},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
            } else if (scene_id_ == "room_elevator_lobby") {
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{9.5f, 5.5f, 0.0f}, 2.6f,
                                      ProductionSpriteKind::Elevator, Color{68, 156, 162},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
            }
            int vm_state = 0;
            float recoil = 0.0f;
            if (combat_ != nullptr && frame_index >= combat_->last_shot_frame &&
                frame_index - combat_->last_shot_frame < 4) {
                vm_state = 1;
                recoil = 1.0f - static_cast<float>(frame_index - combat_->last_shot_frame) / 4.0f;
            }
            if (combat_ != nullptr && combat_->aiming) vm_state = 2;
            DrawWeaponViewmodel(logical_pixels_.data(), width_, logical_h, vm_state, recoil);
            DrawVisualEffects(frame_index, logical_h);
            ComposeHalfBlockFrame(logical_pixels_.data(), width_, logical_h,
                                  body_.data(), width_, height_);
        }
        if (frame_index < 300 && scene_id_ == "room_b1_revival") {
            subtitle_ = "SYS/07: Wake cycle verified. B1 anomaly detected. Proceed to calibration.";
        } else if (frame_index < 180 && scene_id_ == "room_01_calibration") {
            subtitle_ = "CALIBRATION / LOGISTICS: Verify the route. Keep the service door clear.";
        } else if (frame_index < 180 && scene_id_ == "room_1f_security") {
            subtitle_ = "1F SECURITY: Present identity. Watch the cameras.";
        } else if (frame_index < 180 && scene_id_ == "room_service_medical") {
            subtitle_ = "SERVICE / MEDICAL: The building still remembers its staff.";
        } else if (frame_index < 180 && scene_id_ == "room_elevator_lobby") {
            subtitle_ = "ELEVATOR LOBBY: Restricted floors remain listening.";
        } else {
            subtitle_ = "";
        }
        const float npc_dx = player_pos_.x - 10.5f;
        const float npc_dy = player_pos_.y - 5.5f;
        if (scene_id_ == "room_b1_revival" &&
            (npc_dx * npc_dx + npc_dy * npc_dy) < 9.0f) {
            subtitle_ = "Maintenance: 07... you are not scheduled to be here.";
        }
        if (subtitle_override_remaining_ > 0) {
            subtitle_ = subtitle_override_;
            --subtitle_override_remaining_;
        }
        if (debug_overlay_) {
            subtitle_ = "F3 DEBUG | pos " + std::to_string(player_pos_.x) + "," +
                        std::to_string(player_pos_.y) + " yaw " + std::to_string(player_yaw_);
        }
        HudFrame hud;
        hud.health = 100;
        if (combat_ != nullptr) {
            const size_t slot = static_cast<size_t>(combat_->slot);
            hud.ammo_mag = combat_->ammo_in_mag[slot];
            hud.ammo_reserve = combat_->reserve[slot];
        } else {
            hud.ammo_mag = 12;
            hud.ammo_reserve = 48;
        }
        hud.preset_name = settings_ != nullptr && settings_->preset == QualityPreset::Ultra120
                              ? "ULTRA120" : "COMPATIBILITY";
        hud.grid_width = grid_w_;
        hud.grid_height = grid_h_;
        hud.subtitle = subtitle_.c_str();
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
            --narrator_intrusion_remaining_;
        }
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

    void DrawVisualEffects(uint64_t frame_index, int logical_h) {
        const bool reduce_flicker = settings_ != nullptr && settings_->reduce_flicker;
        const bool reduce_shake = settings_ != nullptr && settings_->reduce_camera_shake;
        if (shot_flash_remaining_ > 0) {
            const float gain = reduce_flicker ? 0.24f : 0.48f;
            const int radius = reduce_flicker ? 5 : 10;
            const int cx = width_ / 2;
            const int cy = logical_h / 2;
            for (int y = std::max(0, cy - radius); y < std::min(logical_h, cy + radius); ++y) {
                for (int x = std::max(0, cx - radius); x < std::min(width_, cx + radius); ++x) {
                    const float dx = static_cast<float>(x - cx);
                    const float dy = static_cast<float>(y - cy);
                    if (dx * dx + dy * dy > static_cast<float>(radius * radius)) continue;
                    Color& pixel = logical_pixels_[static_cast<size_t>(y) * width_ + x];
                    pixel.r = static_cast<uint8_t>(std::min(255.0f,
                        static_cast<float>(pixel.r) + 255.0f * gain));
                    pixel.g = static_cast<uint8_t>(std::min(255.0f,
                        static_cast<float>(pixel.g) + 190.0f * gain));
                    pixel.b = static_cast<uint8_t>(std::min(255.0f,
                        static_cast<float>(pixel.b) + 70.0f * gain));
                }
            }
            --shot_flash_remaining_;
        }
        if (hit_flash_remaining_ > 0) {
            const int band = reduce_flicker ? 2 : 4;
            for (int y = 0; y < logical_h; ++y) {
                for (int x = 0; x < width_; ++x) {
                    if (x >= band && x < width_ - band && y >= band && y < logical_h - band) continue;
                    Color& pixel = logical_pixels_[static_cast<size_t>(y) * width_ + x];
                    pixel.r = static_cast<uint8_t>(std::min(255,
                        static_cast<int>(pixel.r) + 36));
                }
            }
            --hit_flash_remaining_;
        }
        if (explosion_remaining_ > 0) {
            const int cx = width_ / 2;
            const int cy = logical_h / 2;
            const int radius = reduce_flicker ? 12 : 22;
            for (int i = 0; i < 12; ++i) {
                const int dx = ((static_cast<int>(frame_index) * 17 + i * 29) %
                                (radius * 2 + 1)) - radius;
                const int dy = ((static_cast<int>(frame_index) * 11 + i * 19) %
                                (radius * 2 + 1)) - radius;
                const int x = cx + dx;
                const int y = cy + dy;
                if (x < 0 || x >= width_ || y < 0 || y >= logical_h) continue;
                logical_pixels_[static_cast<size_t>(y) * width_ + x] = Color{222, 146, 52};
            }
            --explosion_remaining_;
        }
        if (shake_remaining_ > 0) {
            if (!reduce_shake) {
                const int dx = static_cast<int>((frame_index * 13) % 3) - 1;
                const int dy = static_cast<int>((frame_index * 7) % 3) - 1;
                if (dx != 0 || dy != 0) {
                    const std::vector<Color> shifted = logical_pixels_;
                    for (int y = 0; y < logical_h; ++y) {
                        for (int x = 0; x < width_; ++x) {
                            const int sx = std::clamp(x - dx, 0, width_ - 1);
                            const int sy = std::clamp(y - dy, 0, logical_h - 1);
                            logical_pixels_[static_cast<size_t>(y) * width_ + x] =
                                shifted[static_cast<size_t>(sy) * width_ + sx];
                        }
                    }
                }
            }
            --shake_remaining_;
        }
    }

    std::unique_ptr<ITerminalBackend> backend_;
    const int width_;
    const int height_;
    HudRenderer hud_;
    std::vector<CharCell> body_;
    std::vector<Color> logical_pixels_;
    Vec3 player_pos_;
    float player_yaw_ = 0.0f;
    const LocomotionState* locomotion_ = nullptr;
    const CombatState* combat_ = nullptr;
    const std::vector<RuntimeNpc>* npcs_ = nullptr;
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
    std::string scene_id_ = "room_b1_revival";
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
            return true;
        }
        ++frame_;
        return false;
    }
    bool HasFocus() const override { return true; }
    const char* Name() const override { return "replay-keyboard"; }
    const std::string& Error() const { return error_; }

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
};

// InputModule: private integration class that samples the keyboard and mouse
// backends through InputRuntime each sim tick. Uses the production runtime
// backend selection (Raw Input primary, CursorDelta fallback) so the app and
// the input probe share the same pointer path.
class InputModule final : public IEngineModule {
public:
    explicit InputModule(std::unique_ptr<IInputBackend> replay_keyboard = nullptr)
        : selection_(CreateRuntimeBackendSelection(PointerBackendPreference::Auto)),
          runtime_(nullptr, nullptr) {
        if (replay_keyboard) {
            selection_.pointer_name = "replay";
            selection_.mouse_button_source = "replay";
            runtime_ = InputRuntime(std::move(replay_keyboard), nullptr);
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
    const bool data_ready =
        std::filesystem::is_directory(data_root / "rooms", data_ec) &&
        std::filesystem::is_regular_file(
            data_root / "systemic" / "systemic_seed.bin", data_ec);
    if (!data_ready) {
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
    Logger logger;
    logger.SetMinLevel(LogLevel::Info);
    if (std::filesystem::is_regular_file(user_data_root / "settings.cfg")) {
        SettingsRegistry registry;
        const auto loaded = registry.Load((user_data_root / "settings.cfg").string());
        if (loaded.IsOk()) {
            settings = loaded.Value();
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
    if (!config.replay_path.empty()) {
        replay_keyboard = std::make_unique<ReplayKeyboardBackend>(config.replay_path);
    }
    auto input_module = std::make_unique<InputModule>(std::move(replay_keyboard));
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
    engine.RegisterModule(services.world.get());
    engine.RegisterModule(services.player.get());
    engine.RegisterModule(services.ai.get());
    engine.RegisterModule(services.narrative.get());

    // Terminal backend selection (env heuristics, no fake probes) plus the
    // render module that samples the real raycaster for the smoke frame.
    const TerminalProbe probe = ProbeTerminalEnv();
    std::unique_ptr<ITerminalBackend> backend =
        CreateTerminalBackend(config.terminal_w, config.terminal_h, probe);
    if (!backend) {
        std::fprintf(stderr, "fatal: no terminal backend available\n");
        return 2;
    }
    auto render = std::make_unique<RenderModule>(std::move(backend),
                                                 config.terminal_w,
                                                 config.terminal_h);
    render->SetSettingsSource(&settings);
    render->SetSceneId(config.room_id.empty() ? "room_b1_revival" : config.room_id);
    const Vec3 spawn = services.world->HasLoadedRoom()
                           ? services.world->LoadedRoom().spawn_point
                           : Vec3{1.5f, 6.0f, 0.0f};
    render->SetPlayerView(spawn, 0.0f);
    render->SetLocomotionSource(&services.player->Locomotion());
    render->SetCombatSource(&services.player->Combat());

    // PVS-01 owns a deliberately small authored interaction set. These are
    // real systemic records shared by the callbacks below; they are not a
    // second gameplay world and they keep the kernel seams observable.
    struct SliceRuntime {
        EntityId player = EntityId::New(1);
        NpcId guard_npc;
        NpcId cleaner_npc;
        EntityId body = EntityId::New(9002);
        ContainerId cart = ContainerId::New(9001);
        TerminalId terminal = TerminalId::New(9003);
        ObservationSourceId camera = ObservationSourceId::New(9004);
        QuestId opening_quest = QuestId::New(9005);
        RoomId b1_room;
        ItemId badge;
        ItemId cash;
        bool terminal_session = false;
        bool bribe_done = false;
        bool schedule_found = false;
    } slice;
    std::vector<std::string> replay_route;
    bool replay_save_attempted = false;
    bool replay_load_attempted = false;
    bool replay_save_ok = false;
    bool replay_load_ok = false;
    if (services.world->HasLoadedRoom()) slice.b1_room = services.world->LoadedRoom().id;
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
        BodyRecord body;
        body.id = slice.body;
        body.npc = slice.guard_npc;
        body.status = BodyStatus::Unconscious;
        body.disposition = BodyDisposition::Exposed;
        body.position = Vec3{10.5f, 5.5f, 0.0f};
        body.room = slice.b1_room;
        HideableContainer cart;
        cart.id = slice.cart;
        cart.kind = ContainerKind::CleaningCart;
        cart.position = Vec3{15.5f, 4.0f, 0.0f};
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
        const bool setup_ok = services.systemic->AddBody(body) &&
                              services.systemic->AddContainer(cart) &&
                              services.systemic->AddTerminal(terminal) &&
                              services.systemic->AddObservationSource(camera) &&
                              services.systemic->AddQuest(opening);
        if (!setup_ok) {
            std::fprintf(stderr, "pvs slice systemic setup rejected\n");
            return 9;
        }
        if (slice.badge.IsValid()) {
            // The badge starts on the incapacitated guard. Search reveals it;
            // only the later Theft transition moves physical custody.
            services.systemic->TransferItem(slice.badge, slice.body);
        }
    }

    // Bind authored identity records to a small real runtime population. The
    // incapacitated badge owner remains a body, while the other seed actors
    // receive their authored binary profile and use the same perception,
    // memory, decision, and event path as production play.
    if (slice.b1_room.IsValid()) {
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
            if (profile.spawn_room != slice.b1_room || profile.id == slice.guard_npc) {
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
            if (!services.ai->AddNpc(npc, slice.b1_room)) {
                std::fprintf(stderr, "pvs runtime npc setup rejected\n");
                return 10;
            }
        }
        services.ai->SetWorldQuery(&services.world->Query());
        services.ai->SetActiveRoom(slice.b1_room);
        services.ai->SetPlayerPositionSource([&] {
            return services.player->Locomotion().position;
        });
        services.ai->SetPlayerEyeSource([&] {
            return services.player->Locomotion().EyePosition().z;
        });
        const bool cleaner_present = std::any_of(
            services.ai->Npcs().begin(), services.ai->Npcs().end(),
            [&](const RuntimeNpc& runtime) {
                return runtime.instance.id == slice.cleaner_npc;
            });
        if (slice.cleaner_npc.IsValid() && cleaner_present &&
            !services.ai->ConfigureBodyDiscovery(slice.cleaner_npc, slice.body,
                                                 slice.cart, 720)) {
            std::fprintf(stderr, "pvs body-discovery runtime setup rejected\n");
            return 11;
        }
    }
    render->SetNpcSource(&services.ai->Npcs());
    bool debug_overlay = false;
    services.player->SetDebugToggleCallback([&] {
        debug_overlay = !debug_overlay;
        render->SetDebugOverlay(debug_overlay);
    });
    services.player->SetNarratorIntrusionCallback([&] {
        render->TriggerNarratorIntrusion(240);
    });
    services.ai->SetShotFeedbackCallback([&](const ShotFeedback& feedback) {
        render->TriggerShotFeedback(feedback);
    });
    services.player->SetFireCallback([&](const FireRequest& request,
                                         const WeaponDef& weapon) {
        (void)services.ai->HandlePlayerShot(request, weapon,
                                            services.player->CurrentFrame());
    });

    auto switch_room = [&](const std::string& id, const Vec3& spawn_point) -> bool {
        if (!services.world->LoadRoomById(id)) return false;
        services.player->SetWorldQuery(&services.world->Query());
        services.ai->SetWorldQuery(&services.world->Query());
        services.player->Locomotion().position = spawn_point;
        services.player->Locomotion().velocity = Vec3{};
        services.player->SetCurrentRoom(id);
        services.ai->SetActiveRoom(services.world->LoadedRoom().id);
        const Room& room = services.world->LoadedRoom();
        render->SetGridData(room.grid.Data().data(), room.grid.Width(), room.grid.Height());
        render->SetPlayerView(spawn_point, services.player->Locomotion().yaw);
        render->SetSceneId(id);
        if (!config.replay_path.empty() &&
            (replay_route.empty() || replay_route.back() != id)) {
            replay_route.push_back(id);
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
        { Serializer s(n_b); services.narrative->Storylets().Save(s); }
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
        std::string restored_room;
        LocomotionState restored_loco = services.player->Locomotion();
        CombatState restored_combat = services.player->Combat();
        SystemicWorld restored_systemic;
        auto fail_load = [&](const char* text) {
            render->SetSubtitleOnce(text, 180);
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
                restored_combat.spread_factor < 0.0f || restored_combat.spread_factor > 1.0f) {
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
            WorldModule restored_world;
            Deserializer d(world_section->data.data(), world_section->data.size());
            if (!restored_world.LoadState(d) || d.HasError() || !d.AtEnd()) {
                fail_load("Load failed: world state rejected."); return;
            }
        }
        {
            EventBus restored_events;
            Deserializer d(events_section->data.data(), events_section->data.size());
            restored_events.Load(d);
            if (d.HasError() || !d.AtEnd()) {
                fail_load("Load failed: event state rejected."); return;
            }
        }
        {
            DeterministicRNG restored_rng;
            Deserializer d(rng_section->data.data(), rng_section->data.size());
            restored_rng.Load(d);
            if (d.HasError() || !d.AtEnd() ||
                (restored_rng.GetState0() == 0 && restored_rng.GetState1() == 0)) {
                fail_load("Load failed: RNG state rejected."); return;
            }
        }
        {
            StoryletEngine restored_narrative;
            Deserializer d(narrative_section->data.data(), narrative_section->data.size());
            restored_narrative.Load(d);
            if (d.HasError() || !d.AtEnd()) {
                fail_load("Load failed: narrative state rejected."); return;
            }
        }
        if (restored_room.empty()) {
            fail_load("Load failed: incomplete player room."); return;
        }
        if (!switch_room(restored_room, restored_loco.position)) {
            fail_load("Load failed: saved room unavailable.");
            return;
        }
        Deserializer world_d(world_section->data.data(), world_section->data.size());
        Deserializer events_d(events_section->data.data(), events_section->data.size());
        Deserializer rng_d(rng_section->data.data(), rng_section->data.size());
        Deserializer narrative_d(narrative_section->data.data(), narrative_section->data.size());
        Deserializer ai_d(ai_section->data.data(), ai_section->data.size());
        if (!services.world->LoadState(world_d) || world_d.HasError() || !world_d.AtEnd()) {
            fail_load("Load failed: world state commit rejected."); return;
        }
        *services.systemic = std::move(restored_systemic);
        events.Load(events_d);
        sim_rng.Load(rng_d);
        services.narrative->Storylets().Load(narrative_d);
        if (!services.ai->Load(ai_d) || ai_d.HasError() || !ai_d.AtEnd() ||
            events_d.HasError() || !events_d.AtEnd() || rng_d.HasError() || !rng_d.AtEnd() ||
            narrative_d.HasError() || !narrative_d.AtEnd()) {
            fail_load("Load failed: runtime state commit rejected."); return;
        }
        services.player->Locomotion() = restored_loco;
        services.player->Combat() = restored_combat;
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
    services.player->SetInteractCallback([&] {
        const Vec3& p = services.player->Locomotion().position;
        const uint64_t frame = services.player->CurrentFrame();
        const EntityId player = slice.player;
        const EntityId guard = EntityId::New(slice.guard_npc.GetValue());
        const auto near = [&](float x, float y, float radius) {
            const float dx = p.x - x;
            const float dy = p.y - y;
            return dx * dx + dy * dy <= radius * radius;
        };

        if (services.player->CurrentRoom() == "room_b1_revival") {
            if (p.x > 21.0f) {
                if (!switch_room("room_01_calibration", Vec3{1.5f, 9.5f, 0.0f})) {
                    render->SetSubtitleOnce("Calibration room unavailable.", 120);
                }
                return;
            }
            if (near(10.5f, 5.5f, 2.0f)) {
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
                                MemoryRecord memory;
                                memory.id = MemoryId::New(services.systemic->MemoryCount() + 1);
                                memory.npc = guard;
                                memory.kind = MemoryKind::Fear;
                                memory.subject = player;
                                memory.target = guard;
                                memory.room = slice.b1_room;
                                memory.frame = frame;
                                memory.salience = 0.9f;
                                memory.confidence = 0.8f;
                                memory.source = KnowledgeSource::DirectWitness;
                                services.systemic->AddMemory(memory);
                                RelationshipRecord relationship;
                                relationship.a = guard;
                                relationship.b = player;
                                relationship.trust = 0.15f;
                                relationship.fear = 0.60f;
                                relationship.suspicion = 0.70f;
                                services.systemic->SetRelationship(relationship);
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
            if (near(15.5f, 4.0f, 1.8f)) {
                if (services.systemic->GetDrag(slice.body) != nullptr) {
                    services.systemic->EndDrag(slice.body, frame);
                    if (services.systemic->HideBody(slice.body, slice.cart, frame)) {
                        render->SetSubtitleOnce("Cart latched. The corridor is quiet again.", 180);
                    }
                } else {
                    render->SetSubtitleOnce("Cleaning cart: empty and unlocked.", 120);
                }
                return;
            }
            if (near(14.0f, 3.0f, 1.4f)) {
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
                render->SetSubtitleOnce("Camera offline. The blind spot is temporary.", 150);
                return;
            }
            if (near(18.5f, 4.5f, 2.5f)) {
                if (!slice.terminal_session &&
                    services.systemic->ReaderAcceptsItem(slice.badge, 2)) {
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
                        services.systemic->PlayerState().truth_exposure =
                            std::min(1.0f, services.systemic->PlayerState().truth_exposure + 0.08f);
                        render->SetSubtitleOnce("TERMINAL: calibration route unlocked.", 180);
                    }
                } else if (slice.terminal_session) {
                    render->SetSubtitleOnce("TERMINAL: session active. Route copied.", 120);
                } else {
                    render->SetSubtitleOnce("TERMINAL: credential required.", 150);
                }
                return;
            }
            render->SetSubtitleOnce("B1: maintenance, camera loop, terminal, or checkpoint.", 100);
            return;
        }

        if (services.player->CurrentRoom() == "room_01_calibration") {
            if (p.x > 13.0f) {
                if (!switch_room("room_service_medical", Vec3{2.5f, 7.5f, 0.0f})) {
                    render->SetSubtitleOnce("Service route unavailable.", 120);
                }
                return;
            }
            if (near(7.5f, 5.5f, 2.0f)) {
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
            const bool has_badge = services.systemic->ReaderAcceptsItem(slice.badge, 2);
            if (near(15.5f, 8.5f, 2.2f)) {
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
                        render->SetSubtitleOnce("He takes the money. That is not the same as trust.", 180);
                    }
                } else if (slice.bribe_done) {
                    render->SetSubtitleOnce("Checkpoint: the guard remembers the exchange.", 150);
                } else {
                    render->SetSubtitleOnce("ACCESS DENIED. No valid credential.", 180);
                }
                return;
            }
            if (p.y > 12.0f && p.x < 22.0f) {
                if (!switch_room("room_service_medical", Vec3{2.5f, 7.5f, 0.0f})) {
                    render->SetSubtitleOnce("Service route unavailable.", 120);
                }
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
            if (p.x > 20.0f && p.y < 5.0f) {
                switch_room("room_service_medical", Vec3{2.5f, 7.5f, 0.0f});
                return;
            }
            render->SetSubtitleOnce("1F: checkpoint, staff route, or maintenance access.", 100);
            return;
        }

        if (services.player->CurrentRoom() == "room_service_medical") {
            if (p.x > 16.0f) {
                switch_room("room_1f_security", Vec3{2.5f, 13.5f, 0.0f});
            } else if (p.y > 10.5f) {
                switch_room("room_elevator_lobby", Vec3{2.5f, 6.5f, 0.0f});
            } else {
                render->SetSubtitleOnce("MEDICAL: supplies are logged before they are used.", 160);
            }
            return;
        }

        if (services.player->CurrentRoom() == "room_restroom_staff") {
            if (p.x > 16.0f) {
                switch_room("room_elevator_lobby", Vec3{2.5f, 6.5f, 0.0f});
            } else {
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
        render->SetSubtitleOnce("Melee ready: close-range contact is loud.", 90);
    });
    if (services.world->HasLoadedRoom()) {
        const Room& room = services.world->LoadedRoom();
        render->SetGridData(room.grid.Data().data(),
                            room.grid.Width(), room.grid.Height());
    }
    engine.SetRenderModule(render.get());

    const int result = engine.Run(config.max_frames);

    if (!config.frame_dump_path.empty() && !render->LogicalPixels().empty()) {
        const auto& px = render->LogicalPixels();
        const int pw = config.terminal_w;
        const int ph = config.terminal_h * 2;
        std::filesystem::path dump_path(config.frame_dump_path);
        if (dump_path.has_parent_path()) {
            std::error_code dump_ec;
            std::filesystem::create_directories(dump_path.parent_path(), dump_ec);
        }
        FILE* f = std::fopen(config.frame_dump_path.c_str(), "wb");
        if (f) {
            std::fprintf(f, "P6\n%d %d\n255\n", pw, ph);
            for (const auto& p : px) {
                std::fputc(p.r, f); std::fputc(p.g, f); std::fputc(p.b, f);
            }
            std::fclose(f);
        }
    }

    if (!config.replay_path.empty()) {
        size_t full_npcs = 0;
        size_t semi_npcs = 0;
        for (const auto& runtime : services.ai->Npcs()) {
            if (runtime.instance.cognition == CognitionTier::Full) {
                ++full_npcs;
            } else if (runtime.instance.cognition == CognitionTier::SemiHuman) {
                ++semi_npcs;
            }
        }
        std::fprintf(stderr, "REPLAY_RESULT=%s\n",
                     result == 0 ? "PASS" : "FAIL");
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
        std::fprintf(stderr, "NARRATOR_TYPOGRAPHY_ACTIVE=%s\n",
                     render->NarratorTypographyActive() ? "YES" : "NO");
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
            services.narrative->Storylets().Save(s);
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
                StoryletEngine restored_narrative;
                restored_narrative.Load(d);
                if (d.HasError() || !d.AtEnd()) {
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
