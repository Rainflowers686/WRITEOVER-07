#include "tests/test_harness.h"
#include "src/app/player_product.h"
#include "src/app/presentation_pulse.h"
#include "src/app/player_perception.h"
#include "src/app/campaign_panel.h"
#include "src/app/product_save.h"
#include "src/app/player_save.h"
#include "src/app/terminal_surface.h"
#include "src/app/runtime_time_gate.h"
#include "src/render/text_layout.h"
#include "src/app/presentation_text.h"
#include "writeover/render/frame_encoder.h"
#include "writeover/render/hud.h"
#include "writeover/render/character_renderer.h"
#include "writeover/world/room.h"

namespace writeover {
namespace {
bool ChineseDisplayColumns() {
    WO_CHECK_EQ(text::Columns("中文 FPS，门 A1"), 15);
    WO_CHECK(text::Clip("A中文B", 4) == "A中");
    const auto rows = text::Wrap("中文AB 中文CD", 6);
    WO_CHECK_EQ(rows.size(), 2); WO_CHECK(rows[0] == "中文AB");
    std::vector<CharCell> cells(10);
    WO_CHECK_EQ(text::DrawRow(cells.data(), 10, 0, 5, "中A文", CharCell{}), 5);
    WO_CHECK(cells[0].code_point == U'中' && (cells[1].flags & text::kWideTail));
    WO_CHECK(cells[2].code_point == U'A' && cells[3].code_point == U'文');
    AnsiFrameEncoder encoder;
    std::string output;
    encoder.Encode(cells.data(), 10, 1, output);
    WO_CHECK(output.find("中A文") != std::string::npos);
    // Modifying the tail's style must repaint its head rather than position
    // the cursor inside a two-column glyph.
    cells[1].fg_r = 50; output.clear();
    encoder.Encode(cells.data(), 10, 1, output);
    WO_CHECK(output.find("\x1b[1;1H") != std::string::npos);
    WO_CHECK(output.find("\x1b[1;2H") == std::string::npos);
    cells.assign(10, CharCell{}); output.clear();
    encoder.Encode(cells.data(), 10, 1, output);
    WO_CHECK(output.find("中") == std::string::npos);
    size_t invalid_offset = 0;
    WO_CHECK(text::Decode("\xf0\x80\x80\x80", invalid_offset) == U'?');
    WO_CHECK_EQ(invalid_offset, 4);
    WO_CHECK(text::Wrap("中", 1).front() == "?");
    for (const auto size : {TerminalSurface{48, 18}, TerminalSurface{80, 25}, TerminalSurface{240, 67}}) {
        // Guard cells around the canvas catch row overruns while mixed text
        // exercises footer wrapping and whole-glyph right-edge clipping.
        std::vector<CharCell> guarded(static_cast<size_t>(size.width) * size.height + 2);
        guarded.front().code_point = U'!'; guarded.back().code_point = U'!';
        HudFrame hud;
        hud.subtitle = "清洁工：门禁还亮着，先别开枪。 Cleaner: wait for the reader.";
        HudRenderer{}.Draw(guarded.data() + 1, size.width, size.height, hud);
        DrawCampaignPanel(guarded.data() + 1, size.width, size.height,
                          {"设置 / SETTINGS", "> 简体中文 / English", "返回：ESC / 确认：F"});
        WO_CHECK(guarded.front().code_point == U'!' && guarded.back().code_point == U'!');
        for (int y = 0; y < size.height; ++y) {
            for (int x = 0; x < size.width; ++x) {
                const size_t i = 1 + static_cast<size_t>(y) * size.width + x;
                if (guarded[i].flags & text::kWideHead) {
                    WO_CHECK(x + 1 < size.width && (guarded[i + 1].flags & text::kWideTail));
                }
                if (guarded[i].flags & text::kWideTail) {
                    WO_CHECK(x > 0 && (guarded[i - 1].flags & text::kWideHead));
                }
            }
        }
    }
    return true;
}
bool SurfaceResizeAndRecovery() {
    RuntimeTimeGate clock;
    clock.ObserveSchedulerFrame(0); clock.ObserveSchedulerFrame(10);
    WO_CHECK_EQ(clock.GameFrame(), 10);
    clock.SetSurfacePaused(true); clock.ObserveSchedulerFrame(100);
    WO_CHECK_EQ(clock.GameFrame(), 10);
    clock.SetPaused(true); clock.SetSurfacePaused(false); clock.ObserveSchedulerFrame(150);
    WO_CHECK(clock.Paused()); WO_CHECK_EQ(clock.GameFrame(), 10);
    clock.SetPaused(false); clock.ObserveSchedulerFrame(151);
    WO_CHECK_EQ(clock.GameFrame(), 11);
    TerminalCaps caps;
    caps.max_width = 120; caps.max_height = 40;
    auto fitted = FitTerminalSurface(240, 67, caps);
    WO_CHECK_EQ(fitted.width, 120); WO_CHECK_EQ(fitted.height, 40);
    WO_CHECK(fitted.Usable());
    AnsiFrameEncoder encoder;
    std::vector<CharCell> cells(120 * 40);
    std::string output;
    WO_CHECK(encoder.Encode(cells.data(), 120, 40, output).full);
    // Full frames must use absolute row addressing, not LF at the bottom
    // margin, which scrolls the actual terminal and invalidates delta state.
    WO_CHECK(output.find('\n') == std::string::npos);
    for (const auto size : {TerminalSurface{20, 8}, TerminalSurface{48, 18}, TerminalSurface{240, 67}}) {
        caps.max_width = size.width; caps.max_height = size.height;
        fitted = FitTerminalSurface(240, 67, caps);
        WO_CHECK_EQ(fitted.width, size.width); WO_CHECK_EQ(fitted.height, size.height);
        cells.assign(static_cast<size_t>(size.width) * size.height, CharCell{});
        DrawCampaignPanel(cells.data(), size.width, size.height, {"PAUSE", "Resume", "Escape: back"});
        output.clear();
        WO_CHECK(encoder.Encode(cells.data(), size.width, size.height, output).full);
        WO_CHECK(output.find("\x1b[2J") != std::string::npos);
        output.clear();
        WO_CHECK(encoder.Encode(cells.data(), size.width, size.height, output).unchanged);
    }
    caps.max_width = 0; caps.max_height = 0;
    WO_CHECK_EQ(FitTerminalSurface(240, 67, caps).width, 240);
    WO_CHECK(!FitTerminalSurface(20, 8, caps).Usable());
    return true;
}
Result<void> FailSelectedSaveReplace(const std::string& tmp, const std::string& dest, void* selected) {
    if (std::filesystem::path(dest).filename().string() == *static_cast<std::string*>(selected)) {
        return Result<void>::Err(1, "injected selected role failure");
    }
    std::error_code error;
    std::filesystem::rename(tmp, dest, error);
    return error ? Result<void>::Err(2, "test replacement failed") : Result<void>::Ok();
}
bool SaveRolePartialFailurePreservesFiles() {
    std::filesystem::path directory;
    for (int i = 0; i < 100; ++i) {
        const auto candidate = std::filesystem::temp_directory_path() /
            ("writeover_roles_regression_" + std::to_string(i));
        std::error_code ec;
        if (std::filesystem::create_directory(candidate, ec)) { directory = candidate; break; }
    }
    WO_CHECK(!directory.empty());
    const std::vector<SaveSection> old_sections{{SaveSectionId::Player, {1}}};
    const std::vector<SaveSection> new_sections{{SaveSectionId::Player, {2}}};
    SaveManager save;
    WO_CHECK(WriteProductSaveRoles(directory, ProductSaveRole::PreFinal, old_sections).resume_saved);
    WO_CHECK(WriteProductSaveRoles(directory, ProductSaveRole::Manual, old_sections).resume_saved);
    std::string failure = "pvs_resume.wo07";
    SetAtomicReplaceProvider({&FailSelectedSaveReplace, &failure});
    const auto partial = WriteProductSaveRoles(directory, ProductSaveRole::Manual, new_sections);
    const auto primary = save.LoadWorld((directory / "pvs_manual").string());
    const auto resume = save.LoadWorld((directory / ProductResumeName(directory)).string());
    const auto prefinal = save.LoadWorld((directory / "pvs_pre_final").string());
    const bool retained_tmp = std::filesystem::exists(directory / "pvs_resume.wo07.tmp");
    failure = "pvs_manual.wo07";
    const auto primary_failure = WriteProductSaveRoles(directory, ProductSaveRole::Manual, old_sections);
    const auto primary_after = save.LoadWorld((directory / "pvs_manual").string());
    SetAtomicReplaceProvider({}); // restore before any failing assertion
    std::error_code ec;
    for (const auto* name : {"pvs_manual.wo07", "pvs_manual.wo07.tmp", "pvs_resume.wo07",
                            "pvs_resume.wo07.tmp", "pvs_pre_final.wo07"}) {
        std::filesystem::remove(directory / name, ec);
    }
    std::filesystem::remove(directory, ec);
    WO_CHECK(partial.primary_saved && !partial.resume_saved && retained_tmp);
    WO_CHECK(primary.IsOk() && primary.Value()[0].data == std::vector<uint8_t>{2});
    WO_CHECK(resume.IsOk() && resume.Value()[0].data == std::vector<uint8_t>{1});
    WO_CHECK(prefinal.IsOk() && prefinal.Value()[0].data == std::vector<uint8_t>{1});
    WO_CHECK(!primary_failure.primary_saved && !primary_failure.resume_saved);
    WO_CHECK(primary_after.IsOk() && primary_after.Value()[0].data == std::vector<uint8_t>{2});
    return true;
}
bool PlayerPayloadVersionsAndTruncation() {
    PlayerSaveData source;
    source.room = "room_b1_revival";
    source.health = 63;
    source.locomotion.position = Vec3{2, 3, 0};
    source.locomotion.jump_cooldown_frames = 12;
    source.combat.next_fire_frame = 115;
    source.combat.last_shot_frame = 100;
    const auto bytes = SerializePlayerSave(source, 105);
    PlayerSaveData restored;
    WO_CHECK(ParsePlayerSave(bytes, 2, restored));
    WO_CHECK_EQ(restored.health, 63);
    WO_CHECK_EQ(restored.combat.next_fire_frame, 12);
    WO_CHECK_EQ(restored.locomotion.jump_cooldown_frames, 12);
    WO_CHECK(SerializePlayerSave(restored, 2) == bytes);
    for (size_t length = 0; length < bytes.size(); ++length) {
        const std::vector<uint8_t> truncated(bytes.begin(), bytes.begin() + length);
        WO_CHECK(!ParsePlayerSave(truncated, 2, restored));
        WO_CHECK_EQ(restored.health, 63); // failed parse never commits staged state
    }
    auto corrupt = bytes;
    corrupt[4] = 99; // explicit U16 payload version follows the U32 marker
    WO_CHECK(!ParsePlayerSave(corrupt, 2, restored));
    corrupt = bytes; corrupt.push_back(0);
    WO_CHECK(!ParsePlayerSave(corrupt, 2, restored));
    const std::vector<uint8_t> legacy(bytes.begin() + kPlayerPayloadHeaderBytes, bytes.end());
    WO_CHECK(ParsePlayerSave(legacy, 2, restored));
    WO_CHECK_EQ(restored.health, 63);
    for (size_t missing = 1; missing <= 5; ++missing) {
        const std::vector<uint8_t> truncated(legacy.begin(), legacy.end() - missing);
        const auto envelope = ComposeSaveBuffer({{SaveSectionId::Player, truncated}});
        const auto valid_crc = ParseSaveBuffer(envelope.data(), envelope.size());
        WO_CHECK(valid_crc.IsOk()); // envelope integrity cannot prove payload completeness
        WO_CHECK(!ParsePlayerSave(valid_crc.Value()[0].data, 2, restored));
    }
    return true;
}
InputState Press(GameAction action) {
    InputState input;
    input.has_focus = true;
    input.action_pressed[static_cast<size_t>(action)] = true;
    input.action_down[static_cast<size_t>(action)] = true;
    return input;
}
bool FeedBoundaries() {
    PerceptionFeed feed;
    WO_CHECK(!feed.Publish(PerceptionCategory::Threat, "Hidden", "hidden", "room", 1, false));
    WO_CHECK(feed.Publish(PerceptionCategory::Dialogue, "Heard", "speech", "room", 2));
    WO_CHECK(!feed.Publish(PerceptionCategory::Dialogue, "Heard", "speech", "room", 3));
    WO_CHECK(feed.Publish(PerceptionCategory::Environment, "Seen", "detail", "room", 3, true, false));
    WO_CHECK_EQ(feed.Visible(4, 0).size(), 0);
    WO_CHECK_EQ(feed.Visible(4, 1).size(), 1);
    WO_CHECK_EQ(feed.Visible(4, 2).size(), 2);
    WO_CHECK_EQ(feed.History(true).size(), 2);
    WO_CHECK(feed.Publish(PerceptionCategory::Threat, "Danger", "danger", "room", 4));
    WO_CHECK(feed.Visible(5, 2).front().find("Danger") != std::string::npos);
    WO_CHECK(feed.Visible(1000, 2).empty());
    for (uint64_t i = 10; i < 110; ++i) feed.Publish(PerceptionCategory::Progress, std::to_string(i), "progress", "room", i);
    WO_CHECK_EQ(feed.Size(), PerceptionFeed::kCapacity);
    const std::string long_text(600, 'x'), long_room(200, 'r');
    WO_CHECK(feed.Publish(PerceptionCategory::Environment, long_text, "long", long_room, 120));
    WO_CHECK(!feed.Publish(PerceptionCategory::Environment, long_text, "long", long_room, 121));
    feed.Clear();
    WO_CHECK_EQ(feed.Size(), 0);
    return true;
}
bool MenuOwnership() {
    PlayerProductRuntime product;
    Settings settings = Settings::Defaults();
    product.Open(ProductPage::Boot);
    WO_CHECK_EQ(product.selection, 1);
    product.selection = 0;
    WO_CHECK(product.Handle(Press(GameAction::Interact), settings) == ProductCommand::None);
    product.continue_available = true;
    WO_CHECK(product.Handle(Press(GameAction::Interact), settings) == ProductCommand::Continue);
    product.Handle(Press(GameAction::MoveBackward), settings);
    product.Handle(Press(GameAction::Interact), settings);
    WO_CHECK(product.page == ProductPage::NewGame);
    product.Handle(Press(GameAction::MoveBackward), settings);
    WO_CHECK(product.Handle(Press(GameAction::Interact), settings) == ProductCommand::NewGame);
    product.boot_context = false;
    product.Open(ProductPage::Pause);
    product.dead = true;
    WO_CHECK(product.Handle(Press(GameAction::Pause), settings) == ProductCommand::None);
    WO_CHECK(product.Active());
    product.dead = false;
    WO_CHECK(product.Handle(Press(GameAction::Pause), settings) == ProductCommand::Resume);
    WO_CHECK(!product.Active());
    product.Open(ProductPage::History);
    auto unfocused = Press(GameAction::Pause);
    unfocused.has_focus = false;
    WO_CHECK(product.Handle(unfocused, settings) == ProductCommand::None);
    WO_CHECK(product.page == ProductPage::History);
    for (int i = 0; i < 2000; ++i) product.Handle(Press(GameAction::MoveRight), settings);
    WO_CHECK_EQ(product.scroll, 1024);
    return true;
}
bool HeldInputReleaseBoundary() {
    auto input = Press(GameAction::Fire);
    auto held = input.action_down;
    const size_t fire = static_cast<size_t>(GameAction::Fire);
    for (int i = 0; i < 4; ++i) {
        input.action_pressed[fire] = true; // includes an operating-system repeat
        { ProductInputLease lease(input, held, true);
          WO_CHECK(!input.action_down[fire]); WO_CHECK(!input.action_pressed[fire]); }
        WO_CHECK(input.action_down[fire]); // backend raw state must survive
    }
    input.action_down[fire] = false; input.action_released[fire] = true;
    { ProductInputLease lease(input, held, false); }
    WO_CHECK(!held[fire]);
    input = Press(GameAction::Fire);
    { ProductInputLease lease(input, held, false); WO_CHECK(input.action_pressed[fire]); }
    return true;
}
bool BoundControlsAndPreferences() {
    Settings settings = Settings::Defaults();
    PresentationText translations;
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path();
    WO_CHECK(translations.Load(root / "data/text"));
    WO_CHECK(translations.Count() > 100);
    WO_CHECK(translations.Present("CONTINUE", "zh-CN") == "继续游戏");
    WO_CHECK(translations.Present("CONTINUE", "en") == "CONTINUE");
    WO_CHECK(translations.Present("A weapon was issued. The record is less certain about you.", "zh-CN") == "武器已经发放。至于你，记录还没核实。");
    PlayerProductRuntime first_run;
    first_run.Open(ProductPage::Language);
    WO_CHECK(settings.language.empty());
    WO_CHECK(first_run.Handle(Press(GameAction::Interact), settings) == ProductCommand::SettingsChanged);
    WO_CHECK(settings.language == "zh-CN" && first_run.page == ProductPage::Boot);
    settings.key_bindings[0][static_cast<size_t>(GameAction::Interact)] = PhysicalKey::E;
    WO_CHECK(ProductControlText("[F] Use / F TRAVEL", settings) == "[E] Use / E TRAVEL");
    WO_CHECK(ProductControlText("WASD MOVE | F INTERACT | LMB FIRE", settings) ==
             "W/A/S/D MOVE | E INTERACT | MOUSE1 FIRE");
    WO_CHECK(ProductKeyName(PhysicalKey::Unknown) == "UNBOUND");
    PlayerProductRuntime product;
    product.Open(ProductPage::Settings);
    const auto before = settings.sensory_verbosity;
    WO_CHECK(product.Handle(Press(GameAction::Interact), settings) == ProductCommand::SettingsChanged);
    WO_CHECK_EQ(settings.sensory_verbosity, (before + 1) % 3);
    SettingsRegistry registry;
    settings.text_duration = 2;
    WO_CHECK(registry.Save("product_preferences.cfg", settings).IsOk());
    const auto loaded = registry.Load("product_preferences.cfg");
    WO_CHECK(loaded.IsOk());
    WO_CHECK_EQ(loaded.Value().sensory_verbosity, settings.sensory_verbosity);
    WO_CHECK_EQ(loaded.Value().text_duration, 2);
    WO_CHECK(loaded.Value().language == "zh-CN");
    for (const int cap : {0, 30, 60, 120, 144, 240, 255}) {
        settings.frame_rate_cap = static_cast<uint8_t>(cap);
        WO_CHECK(registry.Save("product_preferences.cfg", settings).IsOk());
        const auto parsed = registry.Load("product_preferences.cfg");
        WO_CHECK(parsed.IsOk());
        WO_CHECK_EQ(parsed.Value().frame_rate_cap, cap <= 120 ? cap : 0);
    }
    std::vector<uint8_t> first, second;
    Serializer a(first); settings.Save(a);
    settings.text_duration = 0; settings.sensory_verbosity = 0; settings.language = "en";
    Serializer b(second); settings.Save(b);
    WO_CHECK(first == second); // cosmetic text preferences never alter world wire
    return true;
}
bool PanelScrollAndBounds() {
    std::vector<std::string> rows{"LONG RECORD", std::string(200, 'a'), "LAST EVIDENCE", "A/D SCROLL  ESC BACK"};
    for (const int width : {48, 80, 120}) {
        constexpr int height = 18;
        std::vector<CharCell> cells(static_cast<size_t>(width) * height + 2);
        cells.front().code_point = U'X'; cells.back().code_point = U'Y';
        DrawCampaignPanel(cells.data() + 1, width, height, rows, 1024);
        WO_CHECK(cells.front().code_point == U'X' && cells.back().code_point == U'Y');
        std::string text;
        for (const auto& cell : cells) text += static_cast<char>(cell.code_point);
        WO_CHECK(text.find("LAST EVIDENCE") != std::string::npos);
        WO_CHECK(text.find("ESC BACK") != std::string::npos);
    }
    std::vector<CharCell> small(40 * 12);
    DrawCampaignPanel(small.data(), 40, 12, rows);
    WO_CHECK(small.front().code_point == U'R');
    PlayerProductRuntime product;
    const Settings settings = Settings::Defaults();
    product.Open(ProductPage::Pause);
    product.selection = 12;
    const auto menu = product.Rows(settings);
    WO_CHECK(menu[1] == "  RESUME"); // stable position, not reordered to top
    std::vector<CharCell> menu_cells(48 * 18);
    DrawCampaignPanel(menu_cells.data(), 48, 18, menu);
    std::string menu_text;
    for (const auto& cell : menu_cells) menu_text += cell.code_point < 128
        ? static_cast<char>(cell.code_point) : ' ';
    WO_CHECK(menu_text.find("> QUIT") != std::string::npos);
    WO_CHECK(menu_text.find("CONFIRM") != std::string::npos);
    return true;
}

bool CosmeticTimingAndFrameLimit() {
    PresentationPulse pulse;
    pulse.Trigger(100, 12);
    WO_CHECK(!pulse.Active(99));
    // Render sampling frequency and repeated paused renders cannot consume it.
    for (int repeat = 0; repeat < 20; ++repeat) WO_CHECK(pulse.Active(104));
    WO_CHECK_EQ(pulse.Elapsed(108), 8);
    WO_CHECK(!pulse.Active(112));
    pulse.Extend(120, 10);
    pulse.Extend(123, 1);
    WO_CHECK(pulse.Active(129));
    WO_CHECK(!pulse.Active(130));
    pulse.Reset();
    WO_CHECK(!pulse.Active(100)); // successful load clears future effects
    PlayerProductRuntime product;
    Settings settings = Settings::Defaults();
    product.Open(ProductPage::Settings);
    product.selection = 8;
    for (int expected : {30, 60, 120, 0}) {
        WO_CHECK(product.Handle(Press(GameAction::Interact), settings) == ProductCommand::SettingsChanged);
        WO_CHECK_EQ(settings.frame_rate_cap, expected);
    }
    return true;
}
bool PerceptionGeometry() {
    Grid grid(12, 12);
    GridWorldQuery query(&grid);
    LocomotionState pose; pose.position = {2.5f, 5.5f, 0}; pose.yaw = 0;
    const Vec3 visible{5.5f, 5.5f, pose.EyePosition().z};
    WO_CHECK(PlayerCanPerceive(pose, visible, query, 90));
    WO_CHECK(!PlayerCanPerceive(pose, Vec3{0.5f, 5.5f, visible.z}, query, 90));
    GridCell wall = grid.GetCell(4, 5); wall.flags |= CellFlag_Solid;
    grid.SetCell(4, 5, wall);
    WO_CHECK(!PlayerCanPerceive(pose, visible, query, 90));
    return true;
}
bool SaveRoleEnvelope() {
    WO_CHECK(std::string(ProductSaveName(ProductSaveRole::Manual)) != ProductSaveName(ProductSaveRole::PreFinal));
    std::vector<SaveSection> sections;
    for (const auto id : {SaveSectionId::Player, SaveSectionId::World, SaveSectionId::Rng,
         SaveSectionId::Events, SaveSectionId::Ai, SaveSectionId::Narrative, SaveSectionId::Systemic})
        sections.push_back({id, {1}});
    SaveManager save;
    WO_CHECK(save.SaveWorld("product_envelope", sections).IsOk());
    WO_CHECK(ProductSaveEnvelopeValid("product_envelope"));
    sections.pop_back();
    WO_CHECK(save.SaveWorld("product_incomplete", sections).IsOk());
    WO_CHECK(!ProductSaveEnvelopeValid("product_incomplete"));
    return true;
}
bool OpenDistanceHasNoInventedCeiling() {
    Grid grid(64, 64);
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
        GridCell cell; cell.ceiling_height = 100;
        grid.SetCell(x, y, cell);
    }
    CharacterView view; view.origin = {1.5f, 32.5f, 1.6f};
    std::vector<CharCell> cells(120 * 40);
    RenderCharacterFrame(grid.Data().data(), 64, 64, view, cells.data(), 120, 40, 35.0f);
    const auto& sky = cells[60];
    WO_CHECK(sky.code_point == U' ');
    WO_CHECK_EQ(sky.bg_r, 5); WO_CHECK_EQ(sky.bg_g, 9); WO_CHECK_EQ(sky.bg_b, 14);
    return true;
}
bool AuthoredRoofParapetContainsJump() {
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path();
    const auto loaded = LoadRoomFile((root / "data/rooms/room_roof_exit.woc").string());
    WO_CHECK(loaded.IsOk());
    GridWorldQuery query(&loaded.Value().grid);
    for (const auto direction : {Vec2{1, 0}, Vec2{0, 1}, Vec2{-1, 0}}) {
        LocomotionState pose;
        pose.position = direction.x > 0 ? Vec3{13.0f, 32, 0} :
            direction.y > 0 ? Vec3{10, 36, 0} : Vec3{2, 20, 0};
        pose.contact.grounded = true;
        for (int i = 0; i < 600; ++i) {
            TryJump(pose);
            IntegrateLocomotion(pose, direction, true, query, 1.0f / 120.0f);
        }
        WO_CHECK(pose.position.x > 0.5f && pose.position.x < 15.0f && pose.position.y < 38.0f);
        WO_CHECK(pose.position.z >= -0.01f);
    }
    return true;
}
bool KnownEvidenceAndNearestInspect() {
    SystemicWorld world;
    KnowledgeAssetRecord known; known.id = KnowledgeAssetId::New(1);
    known.source = ResourceId::New(1); known.known_by = {EntityId::New(1)};
    WO_CHECK(world.AddKnowledgeAsset(known));
    known.id = KnowledgeAssetId::New(2); known.known_by = {EntityId::New(99)};
    WO_CHECK(world.AddKnowledgeAsset(known));
    WO_CHECK_EQ(PlayerKnownEvidence(world, EntityId::New(1)), 1);
    std::vector<RuntimeNpc> npcs(3);
    for (size_t i = 0; i < npcs.size(); ++i) {
        npcs[i].instance.id = NpcId::New(i + 1); npcs[i].room = RoomId::New(1);
        npcs[i].instance.position = {3.0f - static_cast<float>(i), 0, 0};
        npcs[i].instance.role = i == 1 ? Role::Doctor : Role::Guard;
    }
    npcs[2].room = RoomId::New(2); // closer but in another room
    SceneRuntime scene;
    const auto rows = InspectVisibleTarget(npcs, scene, world, RoomId::New(1), "room", Vec3{0, 0, 1.6f},
        [](const Vec3&, float, float) { return true; }, [](const char*) { return false; });
    WO_CHECK(rows.front() == "EXAMINE / Medical staff");
    const auto hidden = InspectVisibleTarget(npcs, scene, world, RoomId::New(1), "room", Vec3{},
        [](const Vec3&, float, float) { return false; }, [](const char*) { return false; });
    WO_CHECK(hidden.front() == "EXAMINE");
    return true;
}
} // namespace
void RegisterProductTests(TestHarness& harness) {
    harness.Add("product.chinese display columns", &ChineseDisplayColumns);
    harness.Add("product.surface resize and recovery", &SurfaceResizeAndRecovery);
    harness.Add("product.save role partial failure", &SaveRolePartialFailurePreservesFiles);
    harness.Add("product.player payload versions and truncation", &PlayerPayloadVersionsAndTruncation);
    harness.Add("product.feed boundaries", &FeedBoundaries);
    harness.Add("product.menu ownership", &MenuOwnership);
    harness.Add("product.held input release boundary", &HeldInputReleaseBoundary);
    harness.Add("product.bound controls and preferences", &BoundControlsAndPreferences);
    harness.Add("product.panel scroll and bounds", &PanelScrollAndBounds);
    harness.Add("product.cosmetic timing and frame limit", &CosmeticTimingAndFrameLimit);
    harness.Add("product.perception geometry", &PerceptionGeometry);
    harness.Add("product.save role envelope", &SaveRoleEnvelope);
    harness.Add("character.open distance has no invented ceiling", &OpenDistanceHasNoInventedCeiling);
    harness.Add("product.authored roof parapet contains jump", &AuthoredRoofParapetContainsJump);
    harness.Add("product.known evidence and nearest inspect", &KnownEvidenceAndNearestInspect);
}
} // namespace writeover
