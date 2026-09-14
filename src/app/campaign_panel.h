#pragma once

#include "writeover/render/terminal_backend.h"

#include <algorithm>
#include <string>
#include <vector>

namespace writeover {

// Private application presentation for the three paused campaign surfaces.
// These authored ASCII lines are controls, not optional dialogue subtitles.
inline void DrawCampaignPanel(CharCell* cells, int width, int height,
                              const std::vector<std::string>& lines) {
    if (cells == nullptr || width < 48 || height < 18 || lines.empty()) return;
    const int columns = std::min(100, width - 8);
    const int left = (width - columns) / 2;
    const int top = 7;
    const int bottom = height - 5;
    const auto draw = [&](int row, const std::string& text, bool emphasis) {
        for (int x = -2; x < columns + 2; ++x) {
            CharCell& cell = cells[static_cast<size_t>(row) * width + left + x];
            cell.code_point = x >= 0 && x < static_cast<int>(text.size()) && x < columns
                ? static_cast<unsigned char>(text[static_cast<size_t>(x)]) : U' ';
            cell.fg_r = emphasis ? 118 : 230;
            cell.fg_g = emphasis ? 216 : 221;
            cell.fg_b = emphasis ? 186 : 200;
            cell.bg_r = 8; cell.bg_g = 13; cell.bg_b = 22;
            cell.flags = emphasis ? 1 : 0;
        }
    };
    for (int row = top; row <= bottom; ++row) draw(row, {}, false);
    int row = top;
    for (size_t i = 0; i + 1 < lines.size() && row < bottom - 1; ++i) {
        std::string remaining = lines[i];
        do {
            size_t count = std::min(remaining.size(), static_cast<size_t>(columns));
            if (count < remaining.size()) {
                const size_t space = remaining.rfind(' ', count);
                if (space != std::string::npos && space > 0) count = space;
            }
            draw(row++, remaining.substr(0, count), i == 0 || lines[i].rfind(">", 0) == 0);
            remaining.erase(0, count);
            while (!remaining.empty() && remaining.front() == ' ') remaining.erase(0, 1);
        } while (!remaining.empty() && row < bottom - 1);
    }
    // Controls always have their own reserved row, even on a small terminal.
    draw(bottom, lines.back(), true);
}

} // namespace writeover
