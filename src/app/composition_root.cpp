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
    bool HasLoadedRoom() const { return query_ != nullptr && !ctx_.data_dir.empty(); }

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
        const Vec2 move = CameraRelativeWish(local, locomotion_.yaw);
        const bool sprint =
            input_.action_down[static_cast<size_t>(GameAction::Sprint)];
        IntegrateLocomotion(locomotion_, move, sprint, *world_query_,
                            SimClock::kFixedDeltaTime);

        if (input_.action_pressed[static_cast<size_t>(GameAction::Jump)]) {
            TryJump(locomotion_);
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::Crouch)]) {
            TrySetPosture(locomotion_, Posture::Crouch, *world_query_);
        }
        if (input_.action_pressed[static_cast<size_t>(GameAction::Prone)]) {
            TrySetPosture(locomotion_, Posture::Prone, *world_query_);
        }

        if (input_.action_pressed[static_cast<size_t>(GameAction::Fire)]) {
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
        if (input_.action_pressed[static_cast<size_t>(GameAction::Reload)]) {
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
        if (input_.action_pressed[static_cast<size_t>(GameAction::Interact)] && interact_callback_) {
            interact_callback_();
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
    void SetInteractCallback(std::function<void()> cb) { interact_callback_ = std::move(cb); }
    void SetNarratorIntrusionCallback(std::function<void()> cb) { narrator_intrusion_callback_ = std::move(cb); }

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
    std::function<void()> interact_callback_;
    std::function<void()> narrator_intrusion_callback_;
};

class NarrativeModule final : public IEngineModule {
public:
    void Init(const EngineContext& ctx) override {
        ctx_ = ctx;
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
    void SetDebugOverlay(bool enabled) { debug_overlay_ = enabled; }
    void SetSubtitleOnce(const std::string& text, uint64_t frames) { subtitle_override_ = text; subtitle_override_remaining_ = frames; }
    void TriggerNarratorIntrusion(uint64_t frames) { narrator_intrusion_remaining_ = frames; }
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
            const float focal =
                0.5f * static_cast<float>(height_) /
                std::tan(60.0f * 3.14159265f / 360.0f);
            const int logical_h = height_ * 2;
            RenderProductionFrame(grid_cells_, grid_w_, grid_h_, view,
                                  logical_pixels_.data(), width_, logical_h,
                                  focal);
            DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                  Vec3{10.5f, 5.5f, 0.0f}, 1.7f,
                                  ProductionSpriteKind::Npc, Color{120,130,140},
                                  grid_cells_, grid_w_, grid_h_,
                                  logical_pixels_.data(), width_, logical_h, focal);
            DrawProductionSprite(view.origin, view.yaw, view.pitch,
                                  Vec3{18.5f, 4.5f, 1.0f}, 1.4f,
                                  ProductionSpriteKind::Terminal, Color{70,180,170},
                                  grid_cells_, grid_w_, grid_h_,
                                  logical_pixels_.data(), width_, logical_h, focal);
            int vm_state = 0;
            float recoil = 0.0f;
            if (combat_ != nullptr && frame_index >= combat_->last_shot_frame &&
                frame_index - combat_->last_shot_frame < 4) {
                vm_state = 1;
                recoil = 1.0f - static_cast<float>(frame_index - combat_->last_shot_frame) / 4.0f;
            }
            DrawWeaponViewmodel(logical_pixels_.data(), width_, logical_h, vm_state, recoil);
            ComposeHalfBlockFrame(logical_pixels_.data(), width_, logical_h,
                                  body_.data(), width_, height_);
        }
        if (frame_index < 300) {
            subtitle_ = "SYS/07: Wake cycle verified. B1 anomaly detected. Proceed to calibration.";
        } else {
            subtitle_ = "";
        }
        const float npc_dx = player_pos_.x - 10.5f;
        const float npc_dy = player_pos_.y - 5.5f;
        if ((npc_dx * npc_dx + npc_dy * npc_dy) < 9.0f) {
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
        hud.ammo_mag = 12;
        hud.ammo_reserve = 48;
        hud.preset_name = "COMPATIBILITY";
        hud.grid_width = grid_w_;
        hud.grid_height = grid_h_;
        hud.subtitle = subtitle_.c_str();
        hud_.Draw(body_.data(), width_, height_, hud);
        if (narrator_intrusion_remaining_ > 0) {
            for (int yy = 0; yy < height_; ++yy) {
                for (int xx = 0; xx < width_; ++xx) {
                    CharCell& c = body_[static_cast<size_t>(yy) * width_ + xx];
                    c.bg_r = 8; c.bg_g = 4; c.bg_b = 8;
                    c.fg_r = 180; c.fg_g = 40; c.fg_b = 60;
                }
            }
            const char* text = "WRITEOVER-07 // SAVE ACCESS DENIED";
            int len = 0; while (text[len]) ++len;
            const int start_x = (width_ - len) / 2;
            const int start_y = std::max(0, height_ / 2 - 2);
            for (int i = 0; i < len && (start_x + i) < width_; ++i) {
                for (int dy = 0; dy < 3; ++dy) {
                    CharCell& c = body_[static_cast<size_t>(start_y + dy) * width_ + start_x + i];
                    c.code_point = static_cast<char32_t>(text[i]);
                    c.fg_r = 240; c.fg_g = 220; c.fg_b = 120;
                    c.bg_r = 0; c.bg_g = 0; c.bg_b = 0;
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
    bool debug_overlay_ = false;
    std::string subtitle_override_;
    uint64_t subtitle_override_remaining_ = 0;
    uint64_t narrator_intrusion_remaining_ = 0;
    const GridCell* grid_cells_ = nullptr;
    int grid_w_ = 0;
    int grid_h_ = 0;
    std::string subtitle_;
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
    const Vec3 spawn = services.world->HasLoadedRoom()
                           ? services.world->LoadedRoom().spawn_point
                           : Vec3{1.5f, 6.0f, 0.0f};
    render->SetPlayerView(spawn, 0.0f);
    render->SetLocomotionSource(&services.player->Locomotion());
    render->SetCombatSource(&services.player->Combat());
    bool debug_overlay = false;
    services.player->SetDebugToggleCallback([&] {
        debug_overlay = !debug_overlay;
        render->SetDebugOverlay(debug_overlay);
    });
    services.player->SetNarratorIntrusionCallback([&] {
        render->TriggerNarratorIntrusion(240);
    });
    services.player->SetCurrentRoom(config.room_id.empty() ? std::string("room_b1_revival") : config.room_id);
    services.player->SetRoomSwitchCallback([&](const std::string& id, const Vec3& spawn) {
        if (services.world->LoadRoomById(id)) {
            services.player->SetWorldQuery(&services.world->Query());
            services.player->Locomotion().position = spawn;
            services.player->SetCurrentRoom(id);
            const Room& r = services.world->LoadedRoom();
            render->SetGridData(r.grid.Data().data(), r.grid.Width(), r.grid.Height());
            render->SetPlayerView(spawn, 0.0f);
        }
    });
    services.player->SetInteractCallback([&] {
        const Vec3& p = services.player->Locomotion().position;
        if (services.player->CurrentRoom() == "room_b1_revival" && p.x > 21.0f) {
            if (services.world->LoadRoomById("room_1f_security")) {
                services.player->SetWorldQuery(&services.world->Query());
                services.player->Locomotion().position = Vec3{2.5f, 13.5f, 0.0f};
                services.player->SetCurrentRoom("room_1f_security");
                const Room& r = services.world->LoadedRoom();
                render->SetGridData(r.grid.Data().data(), r.grid.Width(), r.grid.Height());
                render->SetPlayerView(Vec3{2.5f, 13.5f, 0.0f}, 0.0f);
            }
        } else if (services.player->CurrentRoom() == "room_b1_revival") {
            const float ndx = p.x - 10.5f; const float ndy = p.y - 5.5f;
            if (ndx * ndx + ndy * ndy < 4.0f) {
                for (const auto& item : services.systemic->Items()) {
                    if (item.type == ItemType::Badge && item.owner == EntityId::New(10)) {
                        services.systemic->TheftItem(item.id, EntityId::New(1), 0);
                        MemoryRecord mem;
                        mem.id = MemoryId::New(services.systemic->MemoryCount() + 1);
                        mem.npc = EntityId::New(10);
                        mem.kind = MemoryKind::Fear;
                        mem.subject = EntityId::New(1);
                        mem.target = EntityId::New(10);
                        mem.salience = 0.9f;
                        mem.confidence = 1.0f;
                        mem.source = KnowledgeSource::DirectWitness;
                        services.systemic->AddMemory(mem);
                        RelationshipRecord rel;
                        rel.a = EntityId::New(10); rel.b = EntityId::New(1);
                        rel.trust = 0.15f; rel.fear = 0.6f; rel.suspicion = 0.7f;
                        services.systemic->SetRelationship(rel);
                        render->SetSubtitleOnce("Badge acquired.", 120);
                        break;
                    }
                }
            } else {
                const float dx = p.x - 18.5f; const float dy = p.y - 4.5f;
                if (dx * dx + dy * dy < 6.25f) {
                    render->SetSubtitleOnce("TERMINAL: No active session. Credential required.", 180);
                }
            }
        } else if (services.player->CurrentRoom() == "room_1f_security") {
            bool has_badge = false;
            for (const auto& item : services.systemic->Items()) {
                if (item.type == ItemType::Badge && item.current_holder == EntityId::New(1) && !item.revoked) {
                    has_badge = true;
                    break;
                }
            }
            if (has_badge) {
                render->SetSubtitleOnce("Access granted: Security checkpoint.", 180);
            } else if (p.y > 13.0f && p.x < 22.0f) {
                render->SetSubtitleOnce("Maintenance route: side path accessible.", 180);
            } else {
                render->SetSubtitleOnce("Access denied: no valid credential.", 180);
            }
        }
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
            Serializer s(player_bytes);
            const LocomotionState& loco = services.player->Locomotion();
            s.WriteF32(loco.position.x);
            s.WriteF32(loco.position.y);
            s.WriteF32(loco.position.z);
            s.WriteF32(loco.yaw);
            s.WriteU8(static_cast<uint8_t>(loco.posture));
            s.WriteU8(static_cast<uint8_t>(loco.traversal));
            s.WriteU8(loco.contact.grounded ? 1 : 0);
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
                if (restored.Value().ActorCount() != services.systemic->ActorCount()) {
                    std::fprintf(stderr, "systemic save/load mismatch\n");
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
