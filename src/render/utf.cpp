#include "writeover/render/terminal_backend.h"

#include <string>

namespace writeover {

std::string CharCellToUtf8(char32_t cp) {
    std::string out;
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}

// Glyph policy helper: returns true for code points that are single-width in
// terminal columns (used by the 3D layer policy in the render contract).
bool IsSingleWidthGlyph(char32_t cp) {
    if (cp >= static_cast<char32_t>(k3dLayerBlockLo) &&
        cp <= static_cast<char32_t>(k3dLayerBlockHi)) {
        return true;  // block/quadrant elements are single-width
    }
    // CJK ranges are double-width and must NOT appear in the 3D layer;
    // they belong to the compositor layer only.
    if (cp >= 0x2E80 && cp <= 0x9FFF) {
        return false;
    }
    if (cp >= 0xF900 && cp <= 0xFAFF) {
        return false;
    }
    if (cp >= 0xFF00 && cp <= 0xFFEF) {
        return false;
    }
    return cp <= 0xFFFF;
}

} // namespace writeover