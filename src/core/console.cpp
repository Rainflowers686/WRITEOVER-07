#include "writeover/core/console.h"

// Non-Windows fallback: no console modes to save, so the guard is inert.
// Windows implementation lives in src/platform/windows/win_console.cpp
// (guarded by _WIN32 there, this TU is guarded the other way around).

#if !defined(_WIN32)

namespace writeover {

ConsoleGuard::ConsoleGuard() = default;
ConsoleGuard::~ConsoleGuard() { Restore(); }
void ConsoleGuard::Restore() { restored_ = true; }
void ConsoleGuard::Disable() { restored_ = true; }

} // namespace writeover

#endif