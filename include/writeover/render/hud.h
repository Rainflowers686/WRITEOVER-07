#pragma once
// HUD / text overlay renderer. Draws UTF-8 text rows into the CharCell buffer
// above the 3D layer. CJK text is allowed here (double-width compositor).

#include "writeover/render/terminal_backend.h"

#include <cstdint>
#include <string>

namespace writeover {

struct HudFrame {
    uint16_t health = 100;
    uint16_t ammo_mag = 0;
    uint16_t ammo_reserve = 0;
    uint16_t score = 0;
    const char* preset_name = "COMPATIBILITY";
    const char* subtitle = nullptr;  // UTF-8, or nullptr
    int grid_width = 0;              // for the dev-line, 0 hides
    int grid_height = 0;
};

class HudRenderer {
public:
    void Draw(CharCell* buffer, int width, int height, const HudFrame& frame) const;

private:
    void DrawRow(CharCell* buffer, int width, int row, const std::string& utf8_text) const;
};

} // namespace writeover