#include "tests/test_harness.h"
#include "src/app/player_product.h"
#include "src/app/player_perception.h"
#include "src/app/campaign_panel.h"
#include "src/app/product_save.h"
#include "writeover/render/character_renderer.h"
#include "writeover/world/room.h"

namespace writeover {
namespace {
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
    std::vector<uint8_t> first, second;
    Serializer a(first); settings.Save(a);
    settings.text_duration = 0; settings.sensory_verbosity = 0;
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
    harness.Add("product.feed boundaries", &FeedBoundaries);
    harness.Add("product.menu ownership", &MenuOwnership);
    harness.Add("product.held input release boundary", &HeldInputReleaseBoundary);
    harness.Add("product.bound controls and preferences", &BoundControlsAndPreferences);
    harness.Add("product.panel scroll and bounds", &PanelScrollAndBounds);
    harness.Add("product.perception geometry", &PerceptionGeometry);
    harness.Add("product.save role envelope", &SaveRoleEnvelope);
    harness.Add("character.open distance has no invented ceiling", &OpenDistanceHasNoInventedCeiling);
    harness.Add("product.authored roof parapet contains jump", &AuthoredRoofParapetContainsJump);
    harness.Add("product.known evidence and nearest inspect", &KnownEvidenceAndNearestInspect);
}
} // namespace writeover
