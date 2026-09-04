// POSIX ANSI terminal backend.  It consumes the same portable frame encoder
// as the Windows VT backend and deliberately suppresses output when stdout is
// redirected, which keeps CI/smoke logs usable while frame dumps remain real.

#include "writeover/render/frame_encoder.h"
#include "writeover/render/terminal_backend.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <memory>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace writeover {

namespace {

class PosixAnsiBackend final : public ITerminalBackend {
public:
    bool Init(int width, int height) override {
        width_ = width;
        height_ = height;
        caps_.true_color = true;
        caps_.ansi_escape = true;
        caps_.char_aspect = 0.5f;
#if defined(__unix__) || defined(__APPLE__)
        is_tty_ = ::isatty(STDOUT_FILENO) != 0;
        struct winsize ws{};
        if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            caps_.max_width = static_cast<int>(ws.ws_col);
            caps_.max_height = static_cast<int>(ws.ws_row);
        }
#else
        is_tty_ = false;
#endif
        initialized_ = true;
        return width_ > 0 && height_ > 0;
    }

    void Shutdown() override { Restore(); initialized_ = false; }

    bool Submit(const CharCell* buffer, int width, int height) override {
        if (!initialized_ || buffer == nullptr || width <= 0 || height <= 0) {
            return false;
        }
        std::string encoded;
        encoder_.Encode(buffer, width, height, encoded);
#if defined(__unix__) || defined(__APPLE__)
        if (is_tty_ && !encoded.empty()) {
            const char* ptr = encoded.data();
            size_t remaining = encoded.size();
            while (remaining > 0) {
                const ssize_t written = ::write(STDOUT_FILENO, ptr, remaining);
                if (written <= 0) {
                    if (written < 0 && errno == EINTR) continue;
                    return false;
                }
                ptr += written;
                remaining -= static_cast<size_t>(written);
            }
        }
#else
        (void)encoded;
#endif
        return true;
    }

    void Restore() override {
#if defined(__unix__) || defined(__APPLE__)
        if (is_tty_) {
            static constexpr char kReset[] = "\x1b[0m\x1b[?25h\n";
            const ssize_t reset_written =
                ::write(STDOUT_FILENO, kReset, sizeof(kReset) - 1);
            (void)reset_written;
        }
#endif
        encoder_.Reset();
    }

    const TerminalCaps& GetCaps() const override { return caps_; }
    const char* Name() const override { return "ansi-posix"; }

private:
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
    bool is_tty_ = false;
    TerminalCaps caps_{};
    AnsiFrameEncoder encoder_;
};

} // namespace

std::unique_ptr<ITerminalBackend> CreateTerminalBackend(int width, int height,
                                                        const TerminalProbe&) {
    (void)width;
    (void)height;
    return std::make_unique<PosixAnsiBackend>();
}

} // namespace writeover
