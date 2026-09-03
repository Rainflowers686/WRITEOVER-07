#include "writeover/render/hud.h"

#include "writeover/render/terminal_backend.h"

namespace writeover {

void HudRenderer::DrawRow(CharCell* buffer, int width, int row,
                          const std::string& utf8_text) const {
    if (row < 0) {
        return;
    }
    int x = 0;
    for (unsigned char ch : utf8_text) {
        if (x >= width) {
            break;
        }
        buffer[row * width + x].code_point = static_cast<char32_t>(ch);
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
    DrawRow(buffer, width, 0, top);
    DrawRow(buffer, width, 1, dev);
    if (frame.subtitle != nullptr) {
        DrawRow(buffer, width, height - 2, frame.subtitle);
    }
}

} // namespace writeover