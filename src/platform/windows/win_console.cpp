// Windows implementation of the ConsoleGuard (normal-exit restore).
// Abnormal-termination restore is best-effort via atexit; SEH wiring is a
// release-gate task and is documented as NOT implemented yet (M-014 closure:
// no over-promising crash-safe restoration).

#include "writeover/core/console.h"

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdlib>

namespace writeover {

namespace {
ConsoleGuard* g_guard = nullptr;

void AtexitRestore() {
    if (g_guard != nullptr) {
        g_guard->Restore();
    }
}
} // namespace

ConsoleGuard::ConsoleGuard() {
    handle_ = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr &&
        GetConsoleMode(static_cast<HANDLE>(handle_), &mode)) {
        saved_mode_ = static_cast<uint32_t>(mode);
        active_ = true;
        if (g_guard == nullptr) {
            g_guard = this;
            std::atexit(AtexitRestore);
        }
    }
}

ConsoleGuard::~ConsoleGuard() { Restore(); }

void ConsoleGuard::Restore() {
    if (restored_) {
        return;
    }
    restored_ = true;
    if (!active_) {
        return;
    }
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
        SetConsoleMode(static_cast<HANDLE>(handle_), saved_mode_);
    }
    // Also re-enable the cursor: rendering may have hidden it.
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != nullptr && out != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO info{};
        if (GetConsoleCursorInfo(out, &info)) {
            info.bVisible = TRUE;
            SetConsoleCursorInfo(out, &info);
        }
    }
    active_ = false;
}

void ConsoleGuard::Disable() { restored_ = true; }

} // namespace writeover

#endif // _WIN32