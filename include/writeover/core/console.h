#pragma once
// ConsoleGuard (RAII): restores the original console mode on normal exit.
// Abnormal termination (crash) is best-effort via an optional atexit handler
// and a platform SEH handler; we guarantee normal-exit restoration only
// (M-014 closure - no over-promise of "crash-safe guaranteed").
// Windows implementation lives in src/platform/windows/win_console.cpp;
// the fallback implementation is a no-op guard for non-Windows builds.

#include <cstdint>

namespace writeover {

class ConsoleGuard {
public:
    ConsoleGuard();
    ~ConsoleGuard();
    ConsoleGuard(const ConsoleGuard&) = delete;
    ConsoleGuard& operator=(const ConsoleGuard&) = delete;

    void Restore();
    void Disable();

private:
    void* handle_ = nullptr;
    uint32_t saved_mode_ = 0;
    bool restored_ = false;
    bool active_ = false;
};

} // namespace writeover