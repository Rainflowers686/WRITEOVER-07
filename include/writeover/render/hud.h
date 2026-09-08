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
    const char* weapon_name = "PISTOL";
    const char* objective = nullptr;   // compact player-facing quest objective
    const char* subtitle = nullptr;  // UTF-8, or nullptr
    int grid_width = 0;              // shown only when developer_overlay is true
    int grid_height = 0;             // shown only when developer_overlay is true
    bool developer_overlay = false;
};

class HudRenderer {
public:
    void Draw(CharCell* buffer, int width, int height, const HudFrame& frame) const;

private:
    void DrawRow(CharCell* buffer, int width, int row, const std::string& utf8_text) const;
};

} // namespace writeover
