#pragma once

#include "writeover/render/terminal_backend.h"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace writeover::text {

// Compositor-only pair markers; authored world cells never use these bits.
inline constexpr uint8_t kWideHead = 0x08;
inline constexpr uint8_t kWideTail = 0x10;

inline char32_t Decode(std::string_view value, size_t& offset) {
    if (offset >= value.size()) return U'?';
    const auto first = static_cast<unsigned char>(value[offset++]);
    if (first < 0x80) return first;
    int count = 0;
    char32_t cp = 0;
    char32_t minimum = 0;
    if (first >= 0xC2 && first <= 0xDF) { count = 1; cp = first & 31; minimum = 0x80; }
    else if (first >= 0xE0 && first <= 0xEF) { count = 2; cp = first & 15; minimum = 0x800; }
    else if (first >= 0xF0 && first <= 0xF4) { count = 3; cp = first & 7; minimum = 0x10000; }
    else return U'?';
    for (int i = 0; i < count; ++i) {
        if (offset >= value.size()) return U'?';
        const auto byte = static_cast<unsigned char>(value[offset]);
        if ((byte & 0xC0) != 0x80) return U'?';
        ++offset;
        cp = (cp << 6) | (byte & 63);
    }
    return cp < minimum || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) ? U'?' : cp;
}

// Deterministic width for authored English/Simplified Chinese text. Ambiguous
// box-drawing characters remain one column, preserving the world art contract.
inline int Columns(char32_t cp) {
    return (cp >= 0x1100 && cp <= 0x115F) ||
           (cp >= 0x2E80 && cp <= 0xA4CF && cp != 0x303F) ||
           (cp >= 0xAC00 && cp <= 0xD7A3) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFE10 && cp <= 0xFE19) ||
           (cp >= 0xFE30 && cp <= 0xFE6F) ||
           (cp >= 0xFF01 && cp <= 0xFF60) ||
           (cp >= 0xFFE0 && cp <= 0xFFE6) ||
           (cp >= 0x20000 && cp <= 0x3FFFD) ? 2 : 1;
}

inline int Columns(std::string_view value) {
    int columns = 0;
    for (size_t at = 0; at < value.size();) columns += Columns(Decode(value, at));
    return columns;
}

inline std::string Clip(std::string_view value, int columns) {
    size_t end = 0;
    for (size_t at = 0; at < value.size();) {
        const int used = Columns(Decode(value, at));
        if (used > columns) break;
        columns -= used; end = at;
    }
    return std::string(value.substr(0, end));
}

inline std::vector<std::string> Wrap(std::string_view value, int columns) {
    std::vector<std::string> rows;
    if (columns <= 0) return rows;
    while (!value.empty()) {
        size_t end = 0, last_space = std::string_view::npos;
        int used = 0;
        bool newline = false;
        for (size_t at = 0; at < value.size();) {
            const size_t start = at;
            const char32_t cp = Decode(value, at);
            if (cp == U'\n') { end = start; newline = true; break; }
            const int width = Columns(cp);
            if (used + width > columns) break;
            if (cp == U' ') last_space = start;
            used += width; end = at;
        }
        if (!newline && end < value.size() && last_space != std::string_view::npos && last_space > 0)
            end = last_space;
        if (end == 0 && !newline) {
            // A wide glyph cannot fit a one-column panel; make progress with
            // an explicit replacement, never half a character or an infinite loop.
            size_t at = 0; (void)Decode(value, at);
            rows.push_back("?"); value.remove_prefix(at); continue;
        }
        rows.emplace_back(value.substr(0, end));
        value.remove_prefix(end + (newline ? 1 : 0));
        while (!value.empty() && value.front() == ' ') value.remove_prefix(1);
    }
    if (rows.empty()) rows.emplace_back();
    return rows;
}

inline int DrawRow(CharCell* row, int width, int left, int columns,
                   std::string_view value, CharCell style) {
    if (!row || left < 0 || left >= width || columns <= 0) return 0;
    const int end = std::min(width, left + columns);
    int x = left;
    style.flags &= static_cast<uint8_t>(~(kWideHead | kWideTail));
    for (size_t at = 0; at < value.size();) {
        char32_t cp = Decode(value, at);
        if (cp < 0x20 || cp == 0x7F) cp = U'?';
        const int size = Columns(cp);
        if (x + size > end) break;
        row[x] = style; row[x].code_point = cp;
        if (size == 2) {
            row[x].flags |= kWideHead;
            row[x + 1] = style; row[x + 1].code_point = U' ';
            row[x + 1].flags |= kWideTail;
        }
        x += size;
    }
    return x - left;
}

} // namespace writeover::text
