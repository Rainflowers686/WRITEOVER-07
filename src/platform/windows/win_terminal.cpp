// Windows terminal backends.
// ANSI TrueColor backend: buffered row construction, single stdout flush,
// no per-character cout (forbidden pattern).
// Win32 WriteConsoleOutputW backend: deferred to a release task; the factory
// prefers ANSI whenever VT is available (Windows Terminal / ConEmu expose the
// env markers), so the foundation exercises the REAL path on the reference
// machine.

#include "writeover/render/terminal_backend.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace writeover {

namespace {

// Builds an ANSI TrueColor SGR prefix for a cell; returns empty when the
// cell is identical to the previous one (color-run compression).
std::string AppendSgr(const CharCell& prev, const CharCell& cell) {
    // Run compression: identical fg/bg/flags skip the SGR entirely (F-24).
    if (prev.fg_r == cell.fg_r && prev.fg_g == cell.fg_g &&
        prev.fg_b == cell.fg_b && prev.bg_r == cell.bg_r &&
        prev.bg_g == cell.bg_g && prev.bg_b == cell.bg_b &&
        (prev.flags & 0x01) == (cell.flags & 0x01)) {
        return std::string();
    }
    std::string s;
    if (cell.flags & 0x01) {
        s.append("\x1b[1m");
    } else {
        s.append("\x1b[0m");
    }
    s.append("\x1b[38;2;");
    s.append(std::to_string(cell.fg_r));
    s.push_back(';');
    s.append(std::to_string(cell.fg_g));
    s.push_back(';');
    s.append(std::to_string(cell.fg_b));
    s.append("m");
    s.append("\x1b[48;2;");
    s.append(std::to_string(cell.bg_r));
    s.push_back(';');
    s.append(std::to_string(cell.bg_g));
    s.push_back(';');
    s.append(std::to_string(cell.bg_b));
    s.append("m");
    return s;
}

// 16-color ANSI palette for the Win32 fallback (F-25): full RGB->index
// nearest-neighbor quantization, not the old "red bit only" heuristic.
struct Ansi16Color {
    uint8_t r, g, b;
    WORD fg_attr;  // FOREGROUND_*
    WORD bg_attr;  // BACKGROUND_*
};

constexpr Ansi16Color kAnsi16[] = {
    {0, 0, 0, 0, 0},                          // black
    {128, 0, 0, FOREGROUND_RED, BACKGROUND_RED},
    {0, 128, 0, FOREGROUND_GREEN, BACKGROUND_GREEN},
    {128, 128, 0, FOREGROUND_RED | FOREGROUND_GREEN, BACKGROUND_RED | BACKGROUND_GREEN},
    {0, 0, 128, FOREGROUND_BLUE, BACKGROUND_BLUE},
    {128, 0, 128, FOREGROUND_RED | FOREGROUND_BLUE, BACKGROUND_RED | BACKGROUND_BLUE},
    {0, 128, 128, FOREGROUND_GREEN | FOREGROUND_BLUE, BACKGROUND_GREEN | BACKGROUND_BLUE},
    {192, 192, 192, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
     BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE},
    {128, 128, 128, FOREGROUND_INTENSITY, BACKGROUND_INTENSITY},
    {255, 0, 0, FOREGROUND_INTENSITY | FOREGROUND_RED, BACKGROUND_INTENSITY | BACKGROUND_RED},
    {0, 255, 0, FOREGROUND_INTENSITY | FOREGROUND_GREEN, BACKGROUND_INTENSITY | BACKGROUND_GREEN},
    {255, 255, 0, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
     BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN},
    {0, 0, 255, FOREGROUND_INTENSITY | FOREGROUND_BLUE, BACKGROUND_INTENSITY | BACKGROUND_BLUE},
    {255, 0, 255, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
     BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE},
    {0, 255, 255, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
     BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE},
    {255, 255, 255, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
     BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE},
};

int Nearest16Color(uint8_t r, uint8_t g, uint8_t b) {
    int best = 0;
    int best_dist = INT32_MAX;
    for (int i = 0; i < 16; ++i) {
        const int dr = static_cast<int>(r) - kAnsi16[i].r;
        const int dg = static_cast<int>(g) - kAnsi16[i].g;
        const int db = static_cast<int>(b) - kAnsi16[i].b;
        const int dist = dr * dr + dg * dg + db * db;
        if (dist < best_dist) {
            best_dist = dist;
            best = i;
        }
    }
    return best;
}

class AnsiTrueColorBackend final : public ITerminalBackend {
public:
    bool Init(int width, int height) override {
        width_ = width;
        height_ = height;
        caps_.ansi_escape = true;
        caps_.true_color = true;
        caps_.max_width = width;
        caps_.max_height = height;
        caps_.char_aspect = 0.5f;  // default; manual calibration may adjust
        return true;
    }

    void Shutdown() override {}

    bool Submit(const CharCell* buffer, int width, int height) override {
        if (buffer == nullptr) {
            return false;
        }
        std::string out;
        out.reserve(static_cast<size_t>(width) * height * 3 + 64);
        out.append("\x1b[H");  // home
        CharCell prev{};
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const CharCell& cell = buffer[y * width + x];
                // Color-run compression: only emit SGR when color changes.
                out.append(AppendSgr(prev, cell));
                out.append(CharCellToUtf8(cell.code_point));
                prev = cell;
            }
            out.push_back('\n');
            prev = CharCell{};
        }
        out.append("\x1b[0m");
        std::fwrite(out.data(), 1, out.size(), stdout);
        std::fflush(stdout);
        return true;
    }

    void Restore() override {
        std::fputs("\x1b[0m\x1b[?25h", stdout);
        std::fflush(stdout);
    }

    const TerminalCaps& GetCaps() const override { return caps_; }
    const char* Name() const override { return "ansi-truecolor"; }

private:
    int width_ = 0;
    int height_ = 0;
    TerminalCaps caps_;
};

class Win32WriteConsoleBackend final : public ITerminalBackend {
public:
    bool Init(int width, int height) override {
#if defined(_WIN32)
        width_ = width;
        height_ = height;
        out_ = GetStdHandle(STD_OUTPUT_HANDLE);
        if (out_ == nullptr || out_ == INVALID_HANDLE_VALUE) {
            return false;
        }
        caps_.win32_native = true;
        caps_.max_width = width;
        caps_.max_height = height;
        return true;
#else
        (void)width;
        (void)height;
        return false;
#endif
    }

    void Shutdown() override {}

    bool Submit(const CharCell* buffer, int width, int height) override {
#if defined(_WIN32)
        if (out_ == nullptr || buffer == nullptr) {
            return false;
        }
        const size_t count = static_cast<size_t>(width) * height;
        std::vector<CHAR_INFO> cells(count);
        for (size_t i = 0; i < count; ++i) {
            const CharCell& c = buffer[i];
            const char32_t cp = c.code_point;
            cells[i].Char.UnicodeChar =
                (cp <= 0xFFFF) ? static_cast<wchar_t>(cp) : L'?';
            // Full 16-color nearest-neighbor quantization (F-25).
            const int fi = Nearest16Color(c.fg_r, c.fg_g, c.fg_b);
            const int bi = Nearest16Color(c.bg_r, c.bg_g, c.bg_b);
            cells[i].Attributes = static_cast<WORD>(kAnsi16[fi].fg_attr |
                                                    kAnsi16[bi].bg_attr);
        }
        COORD origin{0, 0};
        SMALL_RECT region{0, 0,
                          static_cast<SHORT>(width - 1),
                          static_cast<SHORT>(height - 1)};
        (void)WriteConsoleOutputW(out_, cells.data(), COORD{static_cast<SHORT>(width),
                                                            static_cast<SHORT>(height)},
                                  origin, &region);
        return true;
#else
        (void)buffer;
        (void)width;
        (void)height;
        return false;
#endif
    }

    void Restore() override {}

    const TerminalCaps& GetCaps() const override { return caps_; }
    const char* Name() const override { return "win32-writeconsole"; }

private:
#if defined(_WIN32)
    HANDLE out_ = nullptr;
#endif
    int width_ = 0;
    int height_ = 0;
    TerminalCaps caps_;
};

} // namespace

std::unique_ptr<ITerminalBackend> CreateTerminalBackend(int width, int height,
                                                        const TerminalProbe& probe) {
    if (probe.vt_enabled) {
        auto ansi = std::make_unique<AnsiTrueColorBackend>();
        if (ansi->Init(width, height)) {
            return ansi;
        }
    }
    auto win32 = std::make_unique<Win32WriteConsoleBackend>();
    if (win32->Init(width, height)) {
        return win32;
    }
    // F-26 closure: last-resort fallback must be initialized too, so the
    // backend is never used uninitialized.
    auto ansi = std::make_unique<AnsiTrueColorBackend>();
    ansi->Init(width, height);
    return ansi;
}

} // namespace writeover