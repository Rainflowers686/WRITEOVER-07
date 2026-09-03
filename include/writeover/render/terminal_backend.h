#pragma once
// Terminal render contract. Glyphs are char32_t code points internally;
// 3D layer is single-width characters only; CJK lives in the HUD compositor
// layer. Terminals do NOT ack SGR sequences, so capability probing is
// HEURISTIC (env/known-host), not an "acceptance probe" (M-014 closure).
// Char aspect is a default + manual first-run calibration, never promised as
// automatic measurement.

#include "writeover/common/types.h"

#include <cstdint>
#include <memory>
#include <string>

namespace writeover {

enum class QualityPreset : uint8_t;

struct CharCell {
    char32_t code_point = U' ';
    uint8_t fg_r = 255, fg_g = 255, fg_b = 255;
    uint8_t bg_r = 0, bg_g = 0, bg_b = 0;
    uint8_t flags = 0;  // 0x01 bold, 0x02 blink, 0x04 underline
};

inline constexpr int k3dLayerSingleWidthMin = 0x20;   // first printable single-width
inline constexpr int k3dLayerBlockLo = 0x2580;        // U+2580 upper half block
inline constexpr int k3dLayerBlockHi = 0x259F;        // through quadrant blocks

struct TerminalCaps {
    bool true_color = false;
    bool ansi_escape = false;
    bool win32_native = false;
    int max_width = 0;
    int max_height = 0;
    float char_aspect = 0.5f;  // default; manual calibration may adjust
};

class ITerminalBackend {
public:
    virtual ~ITerminalBackend() = default;
    virtual bool Init(int width, int height) = 0;
    virtual void Shutdown() = 0;
    virtual bool Submit(const CharCell* buffer, int width, int height) = 0;
    virtual void Restore() = 0;
    virtual const TerminalCaps& GetCaps() const = 0;
    virtual const char* Name() const = 0;
};

// Heuristic probe results (no fake terminal "acceptance").
struct TerminalProbe {
    bool is_windows_terminal = false;   // WT_SESSION present
    bool is_conemu = false;             // ConEmuANSI present
    bool vt_enabled = false;            // TERM/xterm-256color or known host
    int max_width = 0;
    int max_height = 0;
};

TerminalProbe ProbeTerminalEnv();
const char* TerminalKindName(const TerminalProbe& probe);
QualityPreset SuggestPreset(const TerminalProbe& probe, int refresh_hint_hz);

// Factory: picks ANSI TrueColor when VT is available, else Win32
// WriteConsoleOutputW. Implementation in src/platform/windows/.
std::unique_ptr<ITerminalBackend> CreateTerminalBackend(int width, int height,
                                                        const TerminalProbe& probe);

// --- Glyph helpers (render layer) ---
std::string CharCellToUtf8(char32_t cp);
bool IsSingleWidthGlyph(char32_t cp);

} // namespace writeover