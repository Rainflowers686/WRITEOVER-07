#pragma once

#include "writeover/render/terminal_backend.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace writeover {

// Private application presentation for paused campaign and product surfaces.
// These authored ASCII lines are controls, not optional dialogue subtitles.
inline void DrawCampaignPanel(CharCell* cells, int width, int height,
                              const std::vector<std::string>& lines, size_t scroll = 0) {
    if (cells == nullptr || width <= 0 || height <= 0 || lines.empty()) return;
    if (width < 48 || height < 18) {
        const std::string message = "RESIZE TERMINAL: minimum 48 x 18";
        for (int i = 0; i < width * height; ++i) cells[i] = CharCell{};
        for (size_t i = 0; i < message.size() && i < static_cast<size_t>(width * height); ++i) {
            cells[i].code_point = static_cast<unsigned char>(message[i]);
            cells[i].fg_r = 240; cells[i].fg_g = 220; cells[i].fg_b = 180;
        }
        return;
    }
    const int columns = std::min(100, width - 8);
    const int left = (width - columns) / 2;
    const int top = 3;
    const int bottom = height - 3;
    // Retain the paused world as context, but remove competing HUD/scene
    // contrast. The frame itself is authored structural glyphs, not a bitmap.
    for (int i = 0; i < width * height; ++i) {
        auto& cell = cells[i];
        cell.fg_r /= 3; cell.fg_g /= 3; cell.fg_b /= 3;
        cell.bg_r /= 2; cell.bg_g /= 2; cell.bg_b /= 2;
        cell.flags = 0;
    }
    const auto draw = [&](int row, const std::string& text, bool emphasis) {
        if (row < 0 || row >= height) return;
        for (int x = -2; x < columns + 2; ++x) {
            CharCell& cell = cells[static_cast<size_t>(row) * width + left + x];
            cell.code_point = x >= 0 && x < static_cast<int>(text.size()) && x < columns
                ? static_cast<unsigned char>(text[static_cast<size_t>(x)]) : U' ';
            cell.fg_r = emphasis ? 118 : 230;
            cell.fg_g = emphasis ? 216 : 221;
            cell.fg_b = emphasis ? 186 : 200;
            const bool selected = text.rfind("> ", 0) == 0;
            cell.bg_r = selected ? 20 : 8;
            cell.bg_g = selected ? 39 : 13;
            cell.bg_b = selected ? 43 : 22;
            cell.flags = emphasis ? 1 : 0;
        }
    };
    for (int row = top; row <= bottom; ++row) draw(row, {}, false);
    const auto stroke = [&](int x, int y, char32_t glyph) {
        auto& cell = cells[static_cast<size_t>(y) * width + x];
        cell.code_point = glyph;
        cell.fg_r = 63; cell.fg_g = 102; cell.fg_b = 108;
        cell.bg_r = 8; cell.bg_g = 13; cell.bg_b = 22; cell.flags = 0;
    };
    for (int x = left - 2; x <= left + columns + 1; ++x) {
        stroke(x, top - 1, U'─'); stroke(x, bottom + 1, U'─');
    }
    for (int y = top; y <= bottom; ++y) {
        stroke(left - 2, y, U'│'); stroke(left + columns + 1, y, U'│');
    }
    stroke(left - 2, top - 1, U'┌'); stroke(left + columns + 1, top - 1, U'┐');
    stroke(left - 2, bottom + 1, U'└'); stroke(left + columns + 1, bottom + 1, U'┘');
    const auto wrap = [&](const std::string& line) {
        std::vector<std::string> result;
        std::string remaining = line;
        do {
            size_t count = std::min(remaining.size(), static_cast<size_t>(columns));
            if (count < remaining.size()) {
                const size_t space = remaining.rfind(' ', count);
                if (space != std::string::npos && space > 0) count = space;
            }
            result.push_back(remaining.substr(0, count));
            remaining.erase(0, count);
            while (!remaining.empty() && remaining.front() == ' ') remaining.erase(0, 1);
        } while (!remaining.empty());
        return result;
    };
    auto footer = wrap(lines.back());
    if (footer.size() > 3) footer.resize(3);
    const int footer_top = bottom - static_cast<int>(footer.size()) + 1;
    std::vector<std::pair<std::string, bool>> body;
    const auto title = wrap(lines.front());
    draw(top, title.front(), true);
    for (size_t i = 1; i < title.size(); ++i) body.push_back({title[i], true});
    for (size_t i = 1; i + 1 < lines.size(); ++i) {
        for (const auto& line : wrap(lines[i])) body.push_back({line, lines[i].rfind(">", 0) == 0});
    }
    const size_t capacity = static_cast<size_t>(std::max(1, footer_top - top - 2));
    size_t start = std::min(scroll, body.size() > capacity ? body.size() - capacity : 0);
    if (scroll == 0) {
        for (size_t i = 0; i < body.size(); ++i) {
            if (body[i].first.rfind("> ", 0) == 0 && i >= capacity)
                start = i - capacity + 1;
        }
    }
    for (size_t i = start; i < body.size() && i - start < capacity; ++i) {
        draw(top + 1 + static_cast<int>(i - start), body[i].first, body[i].second);
    }
    if (body.size() > capacity) draw(footer_top - 1,
        "ROWS " + std::to_string(start + 1) + "-" + std::to_string(std::min(start + capacity, body.size())) +
        "/" + std::to_string(body.size()) + " / SCROLL TO READ MORE", false);
    for (size_t i = 0; i < footer.size(); ++i) draw(footer_top + static_cast<int>(i), footer[i], true);
    // Text rows fill the panel interior; restore the two side rails last.
    for (int y = top; y <= bottom; ++y) {
        stroke(left - 2, y, U'│'); stroke(left + columns + 1, y, U'│');
    }
}

} // namespace writeover
