// Portable terminal input fallback.  It is intentionally small: keyboard
// taps and arrow deltas make the game playable from a POSIX terminal, while
// the richer relative mouse path remains platform-specific where available.

#include "writeover/player/input.h"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <queue>

#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif

namespace writeover {

namespace {

PhysicalKey MapAscii(unsigned char c) {
    switch (c) {
    case 'w': case 'W': return PhysicalKey::W;
    case 'a': case 'A': return PhysicalKey::A;
    case 's': case 'S': return PhysicalKey::S;
    case 'd': case 'D': return PhysicalKey::D;
    case 'q': case 'Q': return PhysicalKey::Q;
    case 'e': case 'E': return PhysicalKey::E;
    case 'r': case 'R': return PhysicalKey::R;
    case 'f': case 'F': return PhysicalKey::F;
    case 'z': case 'Z': return PhysicalKey::Z;
    case 'x': case 'X': return PhysicalKey::X;
    case 'c': case 'C': return PhysicalKey::C;
    case 'v': case 'V': return PhysicalKey::V;
    case ' ': return PhysicalKey::Space;
    case 9: return PhysicalKey::Tab;
    case 27: return PhysicalKey::Escape;
    case '1': return PhysicalKey::Num1;
    case '2': return PhysicalKey::Num2;
    case '3': return PhysicalKey::Num3;
    case '4': return PhysicalKey::Num4;
    case '5': return PhysicalKey::Num5;
    case '6': return PhysicalKey::Num6;
    case '7': return PhysicalKey::Num7;
    case '8': return PhysicalKey::Num8;
    case '9': return PhysicalKey::Num9;
    case '0': return PhysicalKey::Num0;
    default: return PhysicalKey::Unknown;
    }
}

class NullPointerBackend final : public IInputBackend {
public:
    bool Init() override { return true; }
    void Shutdown() override {}
    bool Poll(InputEvent&) override { return false; }
    bool HasFocus() const override { return true; }
    const char* Name() const override { return "posix-no-pointer"; }
};

class PosixKeyboardBackend final : public IInputBackend {
public:
    bool Init() override {
#if defined(__unix__) || defined(__APPLE__)
        if (::isatty(STDIN_FILENO) != 0) {
            if (::tcgetattr(STDIN_FILENO, &saved_) != 0) return false;
            struct termios raw = saved_;
            ::cfmakeraw(&raw);
            if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) return false;
            old_flags_ = ::fcntl(STDIN_FILENO, F_GETFL, 0);
            if (old_flags_ >= 0) {
                (void)::fcntl(STDIN_FILENO, F_SETFL, old_flags_ | O_NONBLOCK);
            }
            tty_ = true;
        }
#endif
        initialized_ = true;
        return true;
    }

    void Shutdown() override {
#if defined(__unix__) || defined(__APPLE__)
        if (tty_) {
            (void)::tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
            if (old_flags_ >= 0) {
                (void)::fcntl(STDIN_FILENO, F_SETFL, old_flags_);
            }
        }
#endif
        initialized_ = false;
        while (!events_.empty()) events_.pop();
    }

    bool Poll(InputEvent& out) override {
        if (!events_.empty()) {
            out = events_.front();
            events_.pop();
            return true;
        }
#if defined(__unix__) || defined(__APPLE__)
        unsigned char bytes[32] = {};
        const ssize_t count = ::read(STDIN_FILENO, bytes, sizeof(bytes));
        if (count <= 0) {
            return false;
        }
        for (ssize_t i = 0; i < count; ++i) {
            const unsigned char c = bytes[i];
            if (c == 27 && i + 2 < count && bytes[i + 1] == '[') {
                PhysicalKey arrow = PhysicalKey::Unknown;
                switch (bytes[i + 2]) {
                case 'A': arrow = PhysicalKey::Up; break;
                case 'B': arrow = PhysicalKey::Down; break;
                case 'C': arrow = PhysicalKey::Right; break;
                case 'D': arrow = PhysicalKey::Left; break;
                default: break;
                }
                if (arrow != PhysicalKey::Unknown) {
                    events_.push({arrow, true, 0.0f});
                    events_.push({arrow, false, 0.0f});
                    i += 2;
                    continue;
                }
            }
            const PhysicalKey key = MapAscii(c);
            if (key != PhysicalKey::Unknown) {
                events_.push({key, true, 0.0f});
                events_.push({key, false, 0.0f});
            }
        }
#else
        (void)out;
#endif
        if (events_.empty()) return false;
        out = events_.front();
        events_.pop();
        return true;
    }

    bool HasFocus() const override { return initialized_; }
    const char* Name() const override { return "posix-terminal-keyboard"; }

private:
    bool initialized_ = false;
    bool tty_ = false;
    int old_flags_ = -1;
#if defined(__unix__) || defined(__APPLE__)
    struct termios saved_{};
#endif
    std::queue<InputEvent> events_;
};

} // namespace

std::unique_ptr<IInputBackend> CreateKeyboardOnlyBackend() {
    return std::make_unique<PosixKeyboardBackend>();
}

std::unique_ptr<IInputBackend> CreateCursorDeltaBackend() {
    return std::make_unique<NullPointerBackend>();
}

std::unique_ptr<IInputBackend> CreateRawInputMouseBackend() { return nullptr; }

PointerBackendSelection CreateRuntimeBackendSelection(PointerBackendPreference) {
    PointerBackendSelection selection;
    selection.keyboard = CreateKeyboardOnlyBackend();
    selection.pointer = std::make_unique<NullPointerBackend>();
    selection.pointer_name = "posix-no-pointer";
    selection.mouse_button_source = "none";
    return selection;
}

} // namespace writeover
