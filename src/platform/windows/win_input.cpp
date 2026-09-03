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

// Keyboard-only backend: reads key/mouse events from the console input.
// Non-blocking: GetNumberOfConsoleInputEvents probes before reading.
// Uses an internal deque to preserve ALL events from a batch (no drops).
// Focus is tracked via FOCUS_EVENT records and ENABLE_WINDOW_INPUT.
class KeyboardOnlyBackend final : public IInputBackend {
public:
    bool Init() override {
        input_handle_ = GetStdHandle(STD_INPUT_HANDLE);
        if (input_handle_ == INVALID_HANDLE_VALUE || input_handle_ == nullptr) {
            return false;
        }
        DWORD mode = 0;
        if (GetConsoleMode(input_handle_, &mode)) {
            old_mode_ = mode;  // save for restore on Shutdown
            mode &= ~ENABLE_QUICK_EDIT_MODE;
            mode &= ~ENABLE_INSERT_MODE;
            // Enable window input so FOCUS_EVENT records are generated.
            mode |= ENABLE_WINDOW_INPUT;
            SetConsoleMode(input_handle_, mode);
        }
        has_focus_ = true;
        return true;
    }

    void Shutdown() override {
        if (input_handle_ != nullptr && input_handle_ != INVALID_HANDLE_VALUE) {
            SetConsoleMode(input_handle_, old_mode_);
        }
    }

    bool Poll(InputEvent& out_event) override {
        // First drain the internal queue (B.2 closure: no event drops).
        if (!queue_.empty()) {
            out_event = queue_.front();
            queue_.pop_front();
            return true;
        }
        // Non-blocking probe: no events available -> return false immediately.
        DWORD available = 0;
        if (!GetNumberOfConsoleInputEvents(input_handle_, &available)) {
            return false;
        }
        if (available == 0) {
            return false;
        }
        // Read a batch of all available events (up to 32) and queue them.
        constexpr DWORD kMaxEvents = 32;
        INPUT_RECORD records[kMaxEvents];
        DWORD count = 0;
        if (!ReadConsoleInputA(input_handle_, records, kMaxEvents, &count)) {
            return false;
        }
        for (DWORD i = 0; i < count; ++i) {
            if (records[i].EventType == FOCUS_EVENT) {
                has_focus_ = records[i].Event.FocusEvent.bSetFocus != FALSE;
                continue;
            }
            if (records[i].EventType == KEY_EVENT) {
                const PhysicalKey key = MapVk(records[i].Event.KeyEvent.wVirtualKeyCode);
                if (key != PhysicalKey::Unknown) {
                    InputEvent evt;
                    evt.key = key;
                    evt.pressed = records[i].Event.KeyEvent.bKeyDown != FALSE;
                    evt.analog = 0.0f;
                    queue_.push_back(evt);
                }
            }
        }
        // Pop one from the queue if available.
        if (!queue_.empty()) {
            out_event = queue_.front();
            queue_.pop_front();
            return true;
        }
        return false;
    }

    bool HasFocus() const override { return has_focus_; }

    const char* Name() const override { return "keyboard-console"; }

private:
    HANDLE input_handle_ = nullptr;
    DWORD old_mode_ = 0;
    bool has_focus_ = true;
    std::deque<InputEvent> queue_;
};

// Cursor-delta backend with center recapture (fallback #1).
// When the console window is foreground, reads cursor delta, then recenters
// the cursor to avoid cursor reaching the screen edge (F-03 closure).
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
        out_event.key = PhysicalKey::Unknown;
        out_event.pressed = false;
        out_event.analog = 0.0f;
        // Only accumulate delta when the console window is foreground.
        if (!HasFocus()) {
            return false;
        }
        POINT p{};
        if (!GetCursorPos(&p)) {
            return false;
        }
        const double dx = static_cast<double>(p.x) - last_x_;
        const double dy = static_cast<double>(p.y) - last_y_;
        if (std::fabs(dx) < 0.5 && std::fabs(dy) < 0.5) {
            return false;
        }
        mouse_delta_x_ += static_cast<float>(dx);
        mouse_delta_y_ += static_cast<float>(dy);

        // Recenter the cursor to the console window center.
        HWND console = GetConsoleWindow();
        if (console != nullptr) {
            RECT rect{};
            if (GetWindowRect(console, &rect)) {
                const int cx = (rect.left + rect.right) / 2;
                const int cy = (rect.top + rect.bottom) / 2;
                SetCursorPos(cx, cy);
                last_x_ = static_cast<double>(cx);
                last_y_ = static_cast<double>(cy);
            }
        }
        return true;
    }

    bool ConsumeMouseDelta(Vec2& out) override {
        if (mouse_delta_x_ == 0.0f && mouse_delta_y_ == 0.0f) {
            return false;
        }
        out.x = mouse_delta_x_;
        out.y = mouse_delta_y_;
        mouse_delta_x_ = 0.0f;
        mouse_delta_y_ = 0.0f;
        return true;
    }

    bool HasFocus() const override {
        HWND fg = GetForegroundWindow();
        if (fg == nullptr) {
            return false;
        }
        return fg == GetConsoleWindow();
    }

    const char* Name() const override { return "cursor-delta"; }

private:
    double last_x_ = 0.0;
    double last_y_ = 0.0;
    float mouse_delta_x_ = 0.0f;
    float mouse_delta_y_ = 0.0f;
};

} // namespace writeover

std::unique_ptr<writeover::IInputBackend> writeover::CreateKeyboardOnlyBackend() {
    return std::unique_ptr<writeover::IInputBackend>(
        new writeover::KeyboardOnlyBackend());
}

std::unique_ptr<writeover::IInputBackend> writeover::CreateCursorDeltaBackend() {
    return std::unique_ptr<writeover::IInputBackend>(
        new writeover::CursorDeltaBackend());
}

#endif // _WIN32