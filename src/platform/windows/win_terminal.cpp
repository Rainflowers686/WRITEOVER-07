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
        out.reserve(static_cast<size_t>(width) * height * 8 + 64);
        out.append("\x1b[H");  // home
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const CharCell& cell = buffer[y * width + x];
                if (cell.flags & 0x01) {
                    out.append("\x1b[1m");
                } else {
                    out.append("\x1b[0m");
                }
                out.append("\x1b[38;2;");
                out.append(std::to_string(cell.fg_r));
                out.push_back(';');
                out.append(std::to_string(cell.fg_g));
                out.push_back(';');
                out.append(std::to_string(cell.fg_b));
                out.append("m");
                out.append("\x1b[48;2;");
                out.append(std::to_string(cell.bg_r));
                out.push_back(';');
                out.append(std::to_string(cell.bg_g));
                out.push_back(';');
                out.append(std::to_string(cell.bg_b));
                out.append("m");
                out.append(CharCellToUtf8(cell.code_point));
            }
            out.push_back('\n');
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
            cells[i].Attributes = static_cast<WORD>(
                (c.bg_r >= 128 ? 0x4000 : 0) |
                (c.fg_r >= 128 ? 0x0004 : 0));
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
    return std::make_unique<AnsiTrueColorBackend>();
}

} // namespace writeover