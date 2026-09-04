#include "writeover/render/hud.h"

#include "writeover/render/terminal_backend.h"

#include <algorithm>
#include <cstdint>

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

} // namespace

void HudRenderer::DrawRow(CharCell* buffer, int width, int row,
                          const std::string& utf8_text) const {
    if (row < 0) {
        return;
    }
    int x = 2;
    for (size_t index = 0; index < utf8_text.size();) {
        if (x >= width) {
            break;
        }
        CharCell& cell = buffer[row * width + x];
        cell.code_point = NextUtf8(utf8_text, index);
        cell.fg_r = 196;
        cell.fg_g = 222;
        cell.fg_b = 232;
        cell.bg_r = 8;
        cell.bg_g = 13;
        cell.bg_b = 22;
        cell.flags = 0x01;
        ++x;
    }
}

void HudRenderer::Draw(CharCell* buffer, int width, int height,
                       const HudFrame& frame) const {
    if (buffer == nullptr || width <= 0 || height <= 0) {
        return;
    }
    std::string top = "HP " + std::to_string(frame.health) +
                      " AMMO " + std::to_string(frame.ammo_mag) + "/" +
                      std::to_string(frame.ammo_reserve);
    std::string dev = std::string("PRESET ") + frame.preset_name +
                      " GRID " + std::to_string(frame.grid_width) + "x" +
                      std::to_string(frame.grid_height);
    DrawRow(buffer, width, std::min(1, height - 1), top);
    if (height > 1) DrawRow(buffer, width, 1, dev);
    if (frame.subtitle != nullptr) {
        DrawRow(buffer, width, std::max(0, height - 3), frame.subtitle);
    }
}

} // namespace writeover
