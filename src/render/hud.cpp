#include "writeover/render/hud.h"

#include "writeover/render/terminal_backend.h"
#include "text_layout.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <vector>

namespace writeover {

namespace {

void DrawText(CharCell* buffer, int width, int row, const std::string& utf8_text,
              Color ink, bool bold) {
    if (row < 0) {
        return;
    }
    CharCell style;
    style.fg_r = ink.r; style.fg_g = ink.g; style.fg_b = ink.b;
    style.bg_r = 8; style.bg_g = 13; style.bg_b = 22;
    style.flags = bold ? 0x01 : 0;
    text::DrawRow(buffer + static_cast<size_t>(row) * width, width, 2, width - 4, utf8_text, style);
}

std::vector<std::string> SubtitleRows(const std::string& text, int columns) {
    auto rows = text::Wrap(text, columns);
    if (rows.size() > 2) rows.resize(2);
    return rows;
}

} // namespace

void HudRenderer::DrawRow(CharCell* buffer, int width, int row,
                          const std::string& utf8_text) const {
    DrawText(buffer, width, row, utf8_text, {188, 206, 208}, false);
}

void HudRenderer::Draw(CharCell* buffer, int width, int height,
                       const HudFrame& frame) const {
    if (buffer == nullptr || width <= 0 || height <= 0) {
        return;
    }
    std::string weapon = frame.weapon_name != nullptr ? frame.weapon_name : "Pistol";
    for (size_t i = 1; i < weapon.size(); ++i)
        weapon[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(weapon[i])));
    const std::string top = std::string(frame.health_label ? frame.health_label : "Health") + " " + std::to_string(frame.health) + "    " + weapon +
                            "  " + std::to_string(frame.ammo_mag) + " / " +
                            std::to_string(frame.ammo_reserve);
    DrawText(buffer, width, std::min(1, height - 1), top,
              frame.health <= 25 ? Color{230, 108, 98} : Color{144, 163, 168}, false);
    if (frame.developer_overlay && height > 2) {
        std::string dev = std::string("F3 DEV  PRESET ") + frame.preset_name +
                          "  GRID " + std::to_string(frame.grid_width) + "x" +
                          std::to_string(frame.grid_height);
        DrawRow(buffer, width, 2, dev);
    }
    if (frame.objective != nullptr && frame.objective[0] != '\0') {
        const int objective_row = height >= 12 ? (frame.developer_overlay ? 4 : 3) : 2;
        if (objective_row < height - 3) {
            DrawText(buffer, width, objective_row,
                      std::string("> ") + frame.objective, {201, 208, 195}, false);
        }
    }
    if (frame.interaction_prompt != nullptr &&
        frame.interaction_prompt[0] != '\0') {
        const int prompt_row = height >= 12 ? (frame.developer_overlay ? 6 : 5) : 3;
        if (prompt_row < height - 3) {
            DrawText(buffer, width, prompt_row, frame.interaction_prompt,
                      {118, 216, 186}, true);
        }
    }
    if (width > 4 && height > 4) {
        CharCell& crosshair = buffer[static_cast<size_t>(height / 2) * width + width / 2];
        crosshair.code_point = U'·';
        crosshair.fg_r = 190;
        crosshair.fg_g = 226;
        crosshair.fg_b = 216;
        // Keep the underlying material. A black cell behind a tiny dot cut
        // an apparent hole into NPC faces and small control displays.
        crosshair.flags = 0;
    }
    if (frame.subtitle != nullptr && frame.subtitle[0] != '\0' && width > 4 && height >= 8) {
        const auto rows = SubtitleRows(frame.subtitle, std::min(90, width - 4));
        for (size_t i = 0; i < rows.size(); ++i) {
            DrawText(buffer, width, height - 4 + static_cast<int>(i), rows[i],
                      {230, 221, 200}, false);
        }
    }
}

} // namespace writeover
