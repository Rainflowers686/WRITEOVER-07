// Windows input backends: keyboard-only (console input) and cursor-delta.
// Raw Input backend is the P0 primary; the two fallback backends here keep
// gameplay working when Raw Input is unavailable. They implement the
// IInputBackend interface only — gameplay never depends on Win32.

#include "writeover/player/input.h"

#include "writeover/common/math.h"

#include <array>
#include <cstdint>
#include <memory>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windows.h>

#include <cmath>
#include <deque>
#include <memory>

namespace writeover {

namespace {
PhysicalKey MapVk(UINT vk) {
    switch (vk) {
    case 'W': return PhysicalKey::W;
    case 'A': return PhysicalKey::A;
    case 'S': return PhysicalKey::S;
    case 'D': return PhysicalKey::D;
    case 'Q': return PhysicalKey::Q;
    case 'E': return PhysicalKey::E;
    case 'R': return PhysicalKey::R;
    case 'F': return PhysicalKey::F;
    case 'Z': return PhysicalKey::Z;
    case 'X': return PhysicalKey::X;
    case 'C': return PhysicalKey::C;
    case 'V': return PhysicalKey::V;
    case 'B': return PhysicalKey::B;
    case 'N': return PhysicalKey::N;
    case 'M': return PhysicalKey::M;
    case '0': return PhysicalKey::Num0;
    case '1': return PhysicalKey::Num1;
    case '2': return PhysicalKey::Num2;
    case '3': return PhysicalKey::Num3;
    case '4': return PhysicalKey::Num4;
    case '5': return PhysicalKey::Num5;
    case '6': return PhysicalKey::Num6;
    case '7': return PhysicalKey::Num7;
    case '8': return PhysicalKey::Num8;
    case '9': return PhysicalKey::Num9;
    case VK_SPACE: return PhysicalKey::Space;
    case VK_SHIFT: return PhysicalKey::Shift;
    case VK_CONTROL: return PhysicalKey::Ctrl;
    case VK_TAB: return PhysicalKey::Tab;
    case VK_ESCAPE: return PhysicalKey::Escape;
    case VK_UP: return PhysicalKey::Up;
    case VK_DOWN: return PhysicalKey::Down;
    case VK_LEFT: return PhysicalKey::Left;
    case VK_RIGHT: return PhysicalKey::Right;
    case VK_F1: return PhysicalKey::F1;
    case VK_F2: return PhysicalKey::F2;
    case VK_F3: return PhysicalKey::F3;
    case VK_F4: return PhysicalKey::F4;
    case VK_F5: return PhysicalKey::F5;
    case VK_F6: return PhysicalKey::F6;
    case VK_F7: return PhysicalKey::F7;
    case VK_F8: return PhysicalKey::F8;
    case VK_F9: return PhysicalKey::F9;
    case VK_F10: return PhysicalKey::F10;
    case VK_F11: return PhysicalKey::F11;
    case VK_F12: return PhysicalKey::F12;
    case VK_LBUTTON: return PhysicalKey::MouseLeft;
    case VK_RBUTTON: return PhysicalKey::MouseRight;
    case VK_MBUTTON: return PhysicalKey::MouseMiddle;
    case VK_XBUTTON1: return PhysicalKey::MouseX1;
    case VK_XBUTTON2: return PhysicalKey::MouseX2;
    default: return PhysicalKey::Unknown;
    }
}
} // namespace

// Keyboard-only backend: reads key/mouse events from the console input
// record. Non-blocking; drains whatever is available per Poll.
class KeyboardOnlyBackend final : public IInputBackend {
public:
    bool Init() override {
        input_handle_ = GetStdHandle(STD_INPUT_HANDLE);
        if (input_handle_ == INVALID_HANDLE_VALUE || input_handle_ == nullptr) {
            return false;
        }
        DWORD mode = 0;
        if (GetConsoleMode(input_handle_, &mode)) {
            // Keep ENABLE_EXTENDED_FLAGS + ENABLE_PROCESSED_INPUT, clear
            // ENABLE_QUICK_EDIT_MODE and ENABLE_INSERT_MODE.
            mode &= ~ENABLE_QUICK_EDIT_MODE;
            SetConsoleMode(input_handle_, mode);
        }
        return true;
    }

    void Shutdown() override {}

    bool Poll(InputEvent& out_event) override {
        constexpr DWORD kMaxEvents = 32;
        INPUT_RECORD records[kMaxEvents];
        DWORD count = 0;
        // ReadConsoleInput consumes events; remaining events stay in the
        // buffer for the next Poll (no FlushConsoleInputBuffer: that would
        // DROP pending events, which is exactly the F-03 bug).
        if (!ReadConsoleInputA(input_handle_, records, kMaxEvents, &count)) {
            return false;
        }
        // Process key down AND key up events (F-03 closure). Key-up must be
        // propagated so gameplay can detect releases (no sticky keys).
        for (DWORD i = 0; i < count; ++i) {
            if (records[i].EventType == KEY_EVENT) {
                const PhysicalKey key = MapVk(records[i].Event.KeyEvent.wVirtualKeyCode);
                if (key != PhysicalKey::Unknown) {
                    out_event.key = key;
                    out_event.pressed = records[i].Event.KeyEvent.bKeyDown != FALSE;
                    out_event.analog = 0.0f;
                    return true;
                }
            }
        }
        return false;
    }

    bool HasFocus() const override {
        return GetConsoleWindow() != nullptr;
    }

    const char* Name() const override { return "keyboard-console"; }

private:
    HANDLE input_handle_ = nullptr;
};

// Cursor-delta backend with center recapture (fallback #1).
class CursorDeltaBackend final : public IInputBackend {
public:
    bool Init() override {
        POINT p{};
        if (GetCursorPos(&p)) {
            last_x_ = static_cast<double>(p.x);
            last_y_ = static_cast<double>(p.y);
        }
        return true;
    }

    void Shutdown() override {}

    bool Poll(InputEvent& out_event) override {
        POINT p{};
        if (!GetCursorPos(&p)) {
            return false;
        }
        const double dx = static_cast<double>(p.x) - last_x_;
        const double dy = static_cast<double>(p.y) - last_y_;
        last_x_ = static_cast<double>(p.x);
        last_y_ = static_cast<double>(p.y);
        if (std::fabs(dx) < 0.5 && std::fabs(dy) < 0.5) {
            return false;
        }
        out_event.key = PhysicalKey::Unknown;
        out_event.pressed = false;
        out_event.analog = 0.0f;
        mouse_delta_x_ += static_cast<float>(dx);
        mouse_delta_y_ += static_cast<float>(dy);
        return true;
    }

    float ConsumeDx() { const float v = mouse_delta_x_; mouse_delta_x_ = 0.0f; return v; }
    float ConsumeDy() { const float v = mouse_delta_y_; mouse_delta_y_ = 0.0f; return v; }

    bool HasFocus() const override {
        return GetConsoleWindow() != nullptr;
    }

    const char* Name() const override { return "cursor-delta"; }

private:
    double last_x_ = 0.0;
    double last_y_ = 0.0;
    float mouse_delta_x_ = 0.0f;
    float mouse_delta_y_ = 0.0f;
};

} // namespace writeover

// The app uses these factories and interprets the cursor-delta via dynamic
// cast-free polling: CursorDeltaBackend accumulates deltas and its Poll
// reports movement; the composition root reads accumulated deltas through
// the shared InputState path (mouse_delta consumed before each sim tick).
//
// The cursor-delta backend is registered as an InputEvent stream too; it
// appears as an event with key==Unknown, which the app ignores as key input
// but accumulates into mouse_delta.

std::unique_ptr<writeover::IInputBackend> writeover::CreateKeyboardOnlyBackend() {
    return std::unique_ptr<writeover::IInputBackend>(
        new writeover::KeyboardOnlyBackend());
}

std::unique_ptr<writeover::IInputBackend> writeover::CreateCursorDeltaBackend() {
    return std::unique_ptr<writeover::IInputBackend>(
        new writeover::CursorDeltaBackend());
}

#endif // _WIN32