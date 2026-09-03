// Raw Input mouse backend (Windows). Primary pointer backend for the
// production runtime: message-only window + WM_INPUT + non-blocking
// PeekMessage pump. Raw relative deltas never use GetCursorPos/SetCursorPos
// (no screen-edge recapture, no OS acceleration in the relative path).
//
// RIDEV_INPUTSINK lets the message-only window receive input even though the
// console process never owns the visible Windows Terminal window. Background
// samples are drained/discarded by InputRuntime (NeedsBackgroundDrain).
//
// Win32 types stay inside this file (platform layer); gameplay sees only
// IInputBackend / InputState. The platform-neutral packet translation seam
// (TranslateRawMousePacket in common/input_types.h) is unit-tested without
// any Win32 types.

#include "writeover/player/input.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

namespace writeover {

namespace {
const wchar_t kRawInputWindowClass[] = L"WRITEOVER_RawInputMessageWindow";
} // namespace

class RawInputMouseBackend final : public IInputBackend {
public:
    RawInputMouseBackend() = default;
    ~RawInputMouseBackend() override { Shutdown(); }

    bool Init() override {
        if (hwnd_ != nullptr) {
            return true;  // idempotent
        }
        HINSTANCE instance = GetModuleHandleW(nullptr);
        if (instance == nullptr) {
            return false;
        }
        WNDCLASSW wc{};
        wc.lpfnWndProc = &RawInputWndProc;
        wc.hInstance = instance;
        wc.lpszClassName = kRawInputWindowClass;
        if (RegisterClassW(&wc) == 0 &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
        hwnd_ = CreateWindowExW(0, kRawInputWindowClass, L"WRITEOVER Raw Input",
                                0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance,
                                nullptr);
        if (hwnd_ == nullptr) {
            return false;
        }
        SetWindowLongPtrW(hwnd_, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(this));

        // Register mouse raw input (HID usage page 0x01 / usage 0x02).
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;
        rid.dwFlags = RIDEV_INPUTSINK;  // receive even when not foreground
        rid.hwndTarget = hwnd_;
        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
            return false;
        }
        return true;
    }

    void Shutdown() override {
        if (hwnd_ != nullptr) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
        acc_delta_ = Vec2{};
        queue_.clear();
    }

    bool Poll(InputEvent& out_event) override {
        // Non-blocking pump of the message-only window. WM_INPUT handlers
        // accumulate delta and queue button events.
        PumpMessages();
        if (!queue_.empty()) {
            out_event = queue_.front();
            queue_.pop_front();
            return true;
        }
        return false;
    }

    bool ConsumeMouseDelta(Vec2& out) override {
        if (acc_delta_.x == 0.0f && acc_delta_.y == 0.0f) {
            return false;
        }
        out = acc_delta_;
        acc_delta_ = Vec2{};
        return true;
    }

    bool HasFocus() const override {
        // Raw Input has no traditional foreground focus; authoritative focus
        // lives on the keyboard backend. Advisory true (may receive samples
        // anytime via INPUTSINK; runtime gates gameplay on keyboard focus).
        return true;
    }

    // Background (INPUTSINK) samples must be drained so no backlog is applied
    // after focus regain.
    bool NeedsBackgroundDrain() const override { return true; }

    // Discard any accumulated delta / queued events (focus regain rebaseline).
    void RebasePointer() override {
        PumpMessages();  // process backlog into acc/queue, then discard
        acc_delta_ = Vec2{};
        queue_.clear();
    }

    const char* Name() const override { return "raw-input-mouse"; }

private:
    void PumpMessages() {
        MSG msg;
        while (hwnd_ != nullptr &&
               PeekMessageW(&msg, hwnd_, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // Called by RawInputWndProc on WM_INPUT.
    void HandleRawInput(HRAWINPUT raw_input) {
        UINT size = 0;
        if (GetRawInputData(raw_input, RID_INPUT, nullptr, &size,
                            sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1) ||
            size == 0) {
            return;
        }
        std::vector<uint8_t> buffer(size);
        UINT written = GetRawInputData(raw_input, RID_INPUT, buffer.data(),
                                       &size, sizeof(RAWINPUTHEADER));
        if (written == static_cast<UINT>(-1) || written > size) {
            return;
        }
        const RAWINPUT* input = reinterpret_cast<const RAWINPUT*>(buffer.data());
        if (input->header.dwType != RIM_TYPEMOUSE) {
            return;
        }
        const RAWMOUSE& mouse = input->data.mouse;
        RawMousePacket packet;
        packet.dx = mouse.lLastX;
        packet.dy = mouse.lLastY;
        packet.flags = mouse.usFlags;
        packet.button_flags = mouse.usButtonFlags;
        TranslateRawMousePacket(packet, acc_delta_, queue_);
    }

    static LRESULT CALLBACK RawInputWndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                            LPARAM lparam) {
        if (msg == WM_INPUT) {
            RawInputMouseBackend* self = reinterpret_cast<RawInputMouseBackend*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self != nullptr) {
                self->HandleRawInput(reinterpret_cast<HRAWINPUT>(lparam));
            }
            return 0;
        }
        if (msg == WM_NCDESTROY) {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    HWND hwnd_ = nullptr;
    Vec2 acc_delta_;
    std::deque<InputEvent> queue_;
};

} // namespace writeover

std::unique_ptr<writeover::IInputBackend> writeover::CreateRawInputMouseBackend() {
    return std::unique_ptr<writeover::IInputBackend>(
        new writeover::RawInputMouseBackend());
}

#endif // _WIN32