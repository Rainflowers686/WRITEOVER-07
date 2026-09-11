// Bounded review sheets from the production CharCell renderer. These fixtures
// complement real room captures; they are NOT foreground gameplay acceptance.
#include "writeover/render/character_renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {
using namespace writeover;
constexpr int kWidth = 80;
constexpr int kHeight = 48;
constexpr int kGridWidth = 20;
constexpr int kGridHeight = 18;
constexpr float kPi = 3.14159265358979323846f;

struct Case {
    const char* title;
    float distance;
    float yaw;
    bool wall = false;
    bool overlap = false;
    int body = 0;
};

void Label(std::vector<CharCell>& cells, const std::string& label) {
    for (size_t i = 0; i < label.size() && i + 1 < kWidth; ++i) {
        CharCell& cell = cells[kWidth + i + 1];
        cell.code_point = static_cast<unsigned char>(label[i]);
        cell.fg_r = 224; cell.fg_g = 220; cell.fg_b = 196;
        cell.bg_r = 5; cell.bg_g = 9; cell.bg_b = 14;
    }
}

std::vector<CharCell> Frame(const CharacterArtBank& bank,
                           CharacterSpriteKind kind, const Case& scenario) {
    GridCell fill;
    fill.material = 1;
    fill.light = 180;
    std::vector<GridCell> grid(kGridWidth * kGridHeight, fill);
    for (int y = 0; y < kGridHeight; ++y) {
        for (int x = 0; x < kGridWidth; ++x) {
            if (x == 0 || y == 0 || x == kGridWidth - 1 || y == kGridHeight - 1)
                grid[static_cast<size_t>(y) * kGridWidth + x].flags = CellFlag_Solid;
            if (scenario.wall && x == 2)
                grid[static_cast<size_t>(y) * kGridWidth + x].floor_height = 0.85f;
        }
    }
    CharacterView view;
    view.origin = {1.5f, 8.5f, kEyeStand};
    if (scenario.body != 0) view.pitch = -0.30f;
    const float focal = 0.5f * kHeight; // 90-degree vertical FOV, no art-only zoom.
    std::vector<CharCell> frame(kWidth * kHeight);
    RenderCharacterFrame(grid.data(), kGridWidth, kGridHeight, view,
                          frame.data(), kWidth, kHeight, focal);
    CharacterSpriteInstance actor;
    actor.position = {1.5f + scenario.distance, 8.5f, 0.0f};
    actor.kind = kind;
    actor.height = 1.8f;
    actor.yaw = scenario.yaw;
    if (scenario.body != 0) {
        actor.kind = scenario.body == 1 ? CharacterSpriteKind::BodyUnconscious
                                       : CharacterSpriteKind::BodyDead;
        actor.height = 0.45f;
    }
    std::vector<CharacterSpriteInstance> actors{actor};
    if (scenario.overlap) {
        CharacterSpriteInstance rear = actor;
        rear.position.x += 0.65f;
        rear.position.y += 0.35f;
        rear.kind = kind == CharacterSpriteKind::SecurityGuard
                        ? CharacterSpriteKind::FullHuman
                        : CharacterSpriteKind::SecurityGuard;
        actors.push_back(rear);
    }
    DrawCharacterSprites(view, actors, bank, grid.data(), kGridWidth, kGridHeight,
                          frame.data(), kWidth, kHeight, focal);
    Label(frame, scenario.title);
    return frame;
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: writeover_art_review ART_FILE OUTPUT_DIRECTORY\n");
        return 2;
    }
    CharacterArtBank bank;
    if (!bank.Load(argv[1])) return 3;
    const std::array<Case, 12> cases{{
        {"Near / front / 1.8m", 1.8f, kPi},
        {"Near / left / 1.8m", 1.8f, kPi * 0.5f},
        {"Near / right / 1.8m", 1.8f, -kPi * 0.5f},
        {"Near / back / 1.8m", 1.8f, 0.0f},
        {"Mid / front / 5m", 5.0f, kPi},
        {"Mid / side / 5m", 5.0f, kPi * 0.5f},
        {"Far / 13m", 13.0f, kPi},
        {"Partial low wall / 3m", 3.0f, kPi, true},
        {"Two actors / 2.6m", 2.6f, kPi, false, true},
        {"Unconscious / 2m", 2.0f, kPi, false, false, 1},
        {"Dead / 2m", 2.0f, kPi, false, false, 2},
        {"Back + wall / 3m", 3.0f, 0.0f, true},
    }};
    const std::array<CharacterSpriteKind, 3> kinds{{
        CharacterSpriteKind::SecurityGuard, CharacterSpriteKind::FullHuman,
        CharacterSpriteKind::MaintenanceWorker}};
    const std::array<const char*, 3> names{{"guard_sheet.svg", "human_sheet.svg",
                                         "maintenance_sheet.svg"}};
    constexpr int sheet_width = kWidth * 3;
    constexpr int sheet_height = kHeight * 4;
    for (size_t kind = 0; kind < kinds.size(); ++kind) {
        std::vector<CharCell> sheet(sheet_width * sheet_height);
        for (size_t panel = 0; panel < cases.size(); ++panel) {
            const std::vector<CharCell> frame = Frame(bank, kinds[kind], cases[panel]);
            const int left = static_cast<int>(panel % 3) * kWidth;
            const int top = static_cast<int>(panel / 3) * kHeight;
            for (int y = 0; y < kHeight; ++y)
                std::copy_n(frame.begin() + y * kWidth, kWidth,
                            sheet.begin() + (top + y) * sheet_width + left);
        }
        const auto output = std::filesystem::path(argv[2]) / names[kind];
        if (!WriteCharacterFrameSvg(sheet.data(), sheet_width, sheet_height,
                                    output.string())) return 4;
    }
    std::printf("ART_REVIEW_FIXTURES=RENDERED FOREGROUND_ACCEPTANCE=NOT_CLAIMED\n");
    return 0;
}
