#include "writeover/render/terminal_backend.h"

#include "writeover/core/settings.h"

#include <cstdlib>
#include <string>

namespace writeover {

namespace {
bool HasEnv(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0';
}
} // namespace

TerminalProbe ProbeTerminalEnv() {
    TerminalProbe probe;
    probe.is_windows_terminal = HasEnv("WT_SESSION");
    probe.is_conemu = HasEnv("ConEmuANSI");
    const char* term = std::getenv("TERM");
    if (term != nullptr) {
        const std::string term_s(term);
        if (term_s.find("xterm") != std::string::npos ||
            term_s.find("vt100") != std::string::npos) {
            probe.vt_enabled = true;
        }
    }
    probe.vt_enabled = probe.vt_enabled || probe.is_windows_terminal ||
                       probe.is_conemu;
    return probe;
}

const char* TerminalKindName(const TerminalProbe& probe) {
    if (probe.is_windows_terminal) {
        return "windows-terminal";
    }
    if (probe.is_conemu) {
        return "conemu";
    }
    if (probe.vt_enabled) {
        return "vt-compatible";
    }
    return "legacy-conhost";
}

QualityPreset SuggestPreset(const TerminalProbe& probe, int refresh_hint_hz) {
    if (probe.is_windows_terminal && refresh_hint_hz >= 120) {
        return QualityPreset::Ultra120;
    }
    if (probe.is_windows_terminal) {
        return QualityPreset::HighRefresh;
    }
    if (refresh_hint_hz >= 90) {
        return QualityPreset::Presentation60;
    }
    return QualityPreset::Compatibility;
}

} // namespace writeover