#include "writeover/render/hud.h"

#include "writeover/render/terminal_backend.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <vector>

namespace writeover {

namespace {

char32_t NextUtf8(const std::string& text, size_t& index) {
    const auto byte = [&](size_t at) -> uint8_t {
        return static_cast<uint8_t>(text[at]);
    };
    const uint8_t first = byte(index++);
    if (first < 0x80) return first;
    int extra = 0;
    char32_t value = 0;
    if ((first & 0xE0) == 0xC0) { extra = 1; value = first & 0x1F; }
    else if ((first & 0xF0) == 0xE0) { extra = 2; value = first & 0x0F; }
    else if ((first & 0xF8) == 0xF0) { extra = 3; value = first & 0x07; }
    else return U'?' ;
    if (index + static_cast<size_t>(extra) > text.size()) {
        index = text.size();
        return U'?';
    }
    for (int i = 0; i < extra; ++i) {
        const uint8_t continuation = byte(index++);
        if ((continuation & 0xC0) != 0x80) return U'?';
        value = (value << 6) | (continuation & 0x3F);
    }
    return value;
}

void DrawText(CharCell* buffer, int width, int row, const std::string& utf8_text,
              Color ink, bool bold) {
    if (row < 0) {
        return;
    }
    int x = 2;
    for (size_t index = 0; index < utf8_text.size();) {
        if (x >= width - 2) {
            break;
        }
        CharCell& cell = buffer[row * width + x];
        cell.code_point = NextUtf8(utf8_text, index);
        cell.fg_r = ink.r;
        cell.fg_g = ink.g;
        cell.fg_b = ink.b;
        cell.bg_r = 8;
        cell.bg_g = 13;
        cell.bg_b = 22;
        cell.flags = bold ? 0x01 : 0;
        ++x;
    }
}

std::vector<std::string> SubtitleRows(const std::string& text, int columns) {
    std::vector<std::string> rows;
    size_t start = 0;
    while (start < text.size() && rows.size() < 2) {
        size_t end = start;
        size_t last_space = std::string::npos;
        int count = 0;
        while (end < text.size() && count < columns) {
            if (text[end] == ' ') last_space = end;
            (void)NextUtf8(text, end);
            ++count;
        }
        if (end < text.size() && last_space != std::string::npos && last_space > start)
            end = last_space;
        rows.push_back(text.substr(start, end - start));
        start = end;
        while (start < text.size() && text[start] == ' ') ++start;
    }
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
    const std::string top = "Health " + std::to_string(frame.health) + "    " + weapon +
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
