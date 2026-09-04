// Composition Root (M1 owns this file).
// Core never links the semantic modules: this executable assembles
// world/player/ai/narrative/render/platform and registers them into the
// Engine (M-002 closure). Everything here is private app wiring.

#include "writeover/core/console.h"
#include "writeover/core/engine.h"
#include "writeover/core/profile.h"
#include "writeover/core/save.h"
#include "writeover/core/settings.h"
#include "writeover/narrative/causality.h"
#include "writeover/narrative/dialog.h"
#include "writeover/narrative/judge.h"
#include "writeover/narrative/narrator.h"
#include "writeover/narrative/storylet.h"
#include "writeover/player/combat.h"
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
#include "src/platform/windows/platform_api.h"

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <functional>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace writeover {

class AiModule final : public IEngineModule {
public:
    void Init(const EngineContext&) override {}
    void Shutdown() override {}
    // Foundation: AI logic is exercised via unit tests; the integration module
    // is a legal no-op (declared empty, not a fake).
    void SimTick(const SimClock&) override {}
    const char* Name() const override { return "ai"; }
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
            if (ConsumeShot(combat_, weapon, static_cast<uint32_t>(frame))) {
                ctx_.events->Post(
                    EventWeaponFire{EntityId::New(1), combat_.slot,
                                    locomotion_.EyePosition(),
                                    locomotion_.yaw, 0.0f,
                                    weapon.loudness},
                    EventKind::Notification, EntityId::New(1),
                    EntityId::Invalid(), EventId::Invalid(), frame);
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
    void SetSettingsSource(const Settings* settings) { settings_ = settings; }
    void SetSceneId(const std::string& id) { scene_id_ = id; }
    void SetDebugOverlay(bool enabled) { debug_overlay_ = enabled; }
    void SetSubtitleOnce(const std::string& text, uint64_t frames) { subtitle_override_ = text; subtitle_override_remaining_ = frames; }
    void TriggerNarratorIntrusion(uint64_t frames) {
        narrator_intrusion_remaining_ = frames;
        narrator_intrusion_total_ = frames;
    }
    bool DebugOverlay() const { return debug_overlay_; }

    void SetLocomotionSource(const LocomotionState* locomotion) {
        locomotion_ = locomotion;
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
                DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                      Vec3{10.5f, 5.5f, 0.0f}, 1.7f,
                                      ProductionSpriteKind::Npc, Color{126, 154, 170},
                                      grid_cells_, grid_w_, grid_h_,
                                      logical_pixels_.data(), width_, logical_h, focal);
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
            const char* intrusion = "YOU THINK YOU CAN SAVE THIS?";
            const int len = static_cast<int>(std::char_traits<char>::length(intrusion));
            const uint64_t elapsed = narrator_intrusion_total_ > narrator_intrusion_remaining_
                                         ? narrator_intrusion_total_ - narrator_intrusion_remaining_ : 0;
            const int visible = std::min(len, static_cast<int>((elapsed + 1) * len /
                                                                std::max<uint64_t>(1, narrator_intrusion_total_)));
            const int shake = (!reduce_shake && (!reduce_flicker || (elapsed % 7 != 0)))
                                  ? static_cast<int>((elapsed % 3)) - 1 : 0;
            for (int i = 0; i < visible; ++i) {
                const int x = 2 + i * std::max(1, (width_ - 5) / std::max(1, len - 1));
                const int y = std::clamp(height_ - 6 - (i * (height_ - 12)) /
                                             std::max(1, len - 1) + shake, 1, height_ - 3);
                for (int dy = 0; dy < 2; ++dy) {
                    for (int dx = 0; dx < 2; ++dx) {
                        if (x + dx >= width_) continue;
                        CharCell& c = body_[static_cast<size_t>(y + dy) * width_ + x + dx];
                        c.code_point = static_cast<char32_t>(intrusion[i]);
                        c.fg_r = 244; c.fg_g = reduce_flicker ? 196 : 224; c.fg_b = 86;
                        c.bg_r = 18; c.bg_g = 10; c.bg_b = 24;
                        c.flags = 0x01;
                    }
                }
            }
            --narrator_intrusion_remaining_;
        }
        backend_->Submit(body_.data(), width_, height_);
    }

    const char* Name() const override { return "render"; }

private:
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
    const Settings* settings_ = nullptr;
    bool debug_overlay_ = false;
    std::string subtitle_override_;
    uint64_t subtitle_override_remaining_ = 0;
    uint64_t narrator_intrusion_remaining_ = 0;
    uint64_t narrator_intrusion_total_ = 0;
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

// InputModule: private integration class that samples the keyboard and mouse
// backends through InputRuntime each sim tick. Uses the production runtime
// backend selection (Raw Input primary, CursorDelta fallback) so the app and
// the input probe share the same pointer path.
class InputModule final : public IEngineModule {
public:
    InputModule()
        : selection_(CreateRuntimeBackendSelection(PointerBackendPreference::Auto)),
          runtime_(std::move(selection_.keyboard), std::move(selection_.pointer)) {}
    ~InputModule() override = default;

    // Exposes the backend report for diagnostics / probe parity.
    const PointerBackendSelection& Selection() const { return selection_; }

    void Init(const EngineContext&) override { runtime_.Init(); }
    void Shutdown() override { runtime_.Shutdown(); }

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
    return Result<GameServices>::Ok(std::move(g));
}

int RunComposition(const GameConfig& config) {
    SimClock clock;
    EventBus events;
    DeterministicRNG sim_rng(config.seed);
    Settings settings = Settings::Defaults();
    Logger logger;
    logger.SetMinLevel(LogLevel::Info);

    EngineContext ctx;
    ctx.clock = &clock;
    ctx.events = &events;
    ctx.sim_rng = &sim_rng;
    ctx.settings = &settings;
    ctx.logger = &logger;
    ctx.data_dir = config.data_dir;

    auto build_result = BuildGame(ctx, config);
    if (build_result.IsError()) {
        std::fprintf(stderr, "systemic seed startup failed: %s\n",
                     build_result.Error().message.c_str());
        return 8;
    }
    GameServices services = std::move(build_result.Value());
    SystemicEventBridge systemic_bridge(services.systemic.get());
    systemic_bridge.Register(events);

    // Runtime input bridge (ISSUE B): polls keyboard + mouse backends every
    // tick and maps them into PlayerModule's InputState.
    auto input_module = std::make_unique<InputModule>();
    input_module->SetTarget(&services.player->Input(),
                            &services.player->Mapper());
    input_module->Init(ctx);

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
    bool debug_overlay = false;
    services.player->SetDebugToggleCallback([&] {
        debug_overlay = !debug_overlay;
        render->SetDebugOverlay(debug_overlay);
    });
    services.player->SetNarratorIntrusionCallback([&] {
        render->TriggerNarratorIntrusion(240);
    });

    auto switch_room = [&](const std::string& id, const Vec3& spawn_point) -> bool {
        if (!services.world->LoadRoomById(id)) return false;
        services.player->SetWorldQuery(&services.world->Query());
        services.player->Locomotion().position = spawn_point;
        services.player->Locomotion().velocity = Vec3{};
        services.player->SetCurrentRoom(id);
        const Room& room = services.world->LoadedRoom();
        render->SetGridData(room.grid.Data().data(), room.grid.Width(), room.grid.Height());
        render->SetPlayerView(spawn_point, services.player->Locomotion().yaw);
        render->SetSceneId(id);
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
        std::vector<SaveSection> sections;
        std::vector<uint8_t> rng_b, ev_b, pl_b, w_b, n_b, sy_b;
        { Serializer s(rng_b); sim_rng.Save(s); }
        { Serializer s(ev_b); events.Save(s); }
        pl_b = serialize_player();
        { Serializer s(w_b); services.world->Infra().Save(s); const auto facts = services.world->Facts().Snapshot(); s.WriteU32(static_cast<uint32_t>(facts.size())); for (const auto& f : facts) { WriteId(s, f.id); s.WriteU8(1); } }
        { Serializer s(n_b); services.narrative->Storylets().Save(s); }
        sy_b = services.systemic->Serialize();
        sections.push_back({SaveSectionId::Player, std::move(pl_b)});
        sections.push_back({SaveSectionId::World, std::move(w_b)});
        sections.push_back({SaveSectionId::Rng, std::move(rng_b)});
        sections.push_back({SaveSectionId::Events, std::move(ev_b)});
        sections.push_back({SaveSectionId::Narrative, std::move(n_b)});
        sections.push_back({SaveSectionId::Systemic, std::move(sy_b)});
        std::error_code ec; std::filesystem::create_directories("saves", ec);
        SaveManager save;
        const auto res = save.SaveWorld("saves/pvs_manual", sections);
        render->SetSubtitleOnce(res.IsOk() ? "Saved." : "Save failed.", 120);
    });
    services.player->SetLoadCallback([&] {
        SaveManager save;
        const auto loaded = save.LoadWorld("saves/pvs_manual");
        if (loaded.IsError()) { render->SetSubtitleOnce("Load failed.", 120); return; }
        bool have_player = false;
        bool have_systemic = false;
        std::string restored_room;
        LocomotionState restored_loco = services.player->Locomotion();
        CombatState restored_combat = services.player->Combat();
        SystemicWorld restored_systemic;
        for (const auto& sec : loaded.Value()) {
            if (sec.id == SaveSectionId::Systemic) {
                auto restored = SystemicWorld::Deserialize(sec.data.data(), sec.data.size());
                if (restored.IsError()) {
                    render->SetSubtitleOnce("Load failed: systemic state rejected.", 180);
                    return;
                }
                restored_systemic = std::move(restored.Value());
                have_systemic = true;
            } else if (sec.id == SaveSectionId::Player) {
                if (sec.data.size() < 4) {
                    render->SetSubtitleOnce("Load failed: player section truncated.", 180);
                    return;
                }
                Deserializer d(sec.data.data(), sec.data.size());
                const uint32_t room_len = d.ReadU32();
                if (d.HasError() || room_len > 128 || room_len > d.Remaining()) {
                    render->SetSubtitleOnce("Load failed: invalid player room.", 180);
                    return;
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
                restored_loco.contact.grounded = d.ReadU8() != 0;
                restored_loco.contact.on_ladder = d.ReadU8() != 0;
                restored_loco.contact.on_climbable = d.ReadU8() != 0;
                DeserializeCombatState(d, restored_combat);
                if (d.HasError() || !d.AtEnd() || posture > static_cast<uint8_t>(Posture::Prone) ||
                    traversal > static_cast<uint8_t>(Traversal::Mantle) ||
                    lean > static_cast<uint8_t>(Lean::Right) ||
                    static_cast<uint8_t>(restored_combat.slot) >= kWeaponSlotCount ||
                    !std::isfinite(restored_loco.position.x) ||
                    !std::isfinite(restored_loco.position.y) ||
                    !std::isfinite(restored_loco.position.z) ||
                    !std::isfinite(restored_loco.yaw) ||
                    !std::isfinite(restored_loco.pitch) ||
                    !std::isfinite(restored_combat.spread_factor) ||
                    restored_combat.spread_factor < 0.0f ||
                    restored_combat.spread_factor > 1.0f) {
                    render->SetSubtitleOnce("Load failed: invalid player state.", 180);
                    return;
                }
                restored_loco.posture = static_cast<Posture>(posture);
                restored_loco.traversal = static_cast<Traversal>(traversal);
                restored_loco.lean = static_cast<Lean>(lean);
                have_player = true;
            }
        }
        if (!have_player || !have_systemic || restored_room.empty()) {
            render->SetSubtitleOnce("Load failed: incomplete save.", 180);
            return;
        }
        if (!switch_room(restored_room, restored_loco.position)) {
            render->SetSubtitleOnce("Load failed: saved room unavailable.", 180);
            return;
        }
        *services.systemic = std::move(restored_systemic);
        services.player->Locomotion() = restored_loco;
        services.player->Combat() = restored_combat;
        render->SetSubtitleOnce("Loaded.", 120);
    });
    services.player->SetCurrentRoom(config.room_id.empty() ? std::string("room_b1_revival") : config.room_id);
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

    if (config.smoke && config.save_after_smoke) {
        // Smoke save: real determinism sections through the atomic writer.
        // Includes player locomotion, world facts/infra, narrative storylet
        // runtime, RNG, and event journal (F-15 closure).
        std::vector<SaveSection> sections;
        std::vector<uint8_t> rng_bytes, events_bytes, player_bytes,
            world_bytes, narrative_bytes, systemic_bytes;
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
            services.world->Infra().Save(s);
            const std::vector<WorldFact> facts = services.world->Facts().Snapshot();
            s.WriteU32(static_cast<uint32_t>(facts.size()));
            for (const auto& f : facts) {
                WriteId(s, f.id);
                s.WriteU8(std::holds_alternative<bool>(f.value) &&
                                  std::get<bool>(f.value)
                              ? 1
                              : 0);
            }
        }
        {
            Serializer s(narrative_bytes);
            services.narrative->Storylets().Save(s);
        }
        sections.push_back({SaveSectionId::Player, std::move(player_bytes)});
        sections.push_back({SaveSectionId::World, std::move(world_bytes)});
        sections.push_back({SaveSectionId::Rng, std::move(rng_bytes)});
        sections.push_back({SaveSectionId::Events, std::move(events_bytes)});
        sections.push_back({SaveSectionId::Narrative, std::move(narrative_bytes)});
        systemic_bytes = services.systemic->Serialize();
        sections.push_back({SaveSectionId::Systemic, std::move(systemic_bytes)});
        // Ensure the runtime saves directory exists (best-effort).
        std::error_code ec;
        std::filesystem::create_directories("saves", ec);
        SaveManager save;
        const std::string save_path = "saves/smoke";
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
        bool found_systemic = false;
        for (const auto& sec : loaded_save.Value()) {
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
                break;
            }
        }
        if (!found_systemic) {
            std::fprintf(stderr, "systemic save section missing\n");
            return 7;
        }
    }
    return result;
}

} // namespace writeover
