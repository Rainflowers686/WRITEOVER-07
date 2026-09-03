// writeover_input_probe: minimal human R0 tool. Reuses the PRODUCTION
// InputRuntime, the production runtime backend selection
// (CreateRuntimeBackendSelection: Raw Input primary / CursorDelta fallback),
// and the production InputMapper. Prints live input state at ~10Hz so a real
// human can verify mouse deltas, mouse buttons, WASD, focus and active
// context without any gameplay code.
//
// Usage:
//   writeover_input_probe.exe --backend auto|raw|cursor --context gameplay|dialogue|menu|developer --seconds N
//
// Exit:
//   --seconds elapsed -> 0 (Esc also exits early)
//   --backend raw with Raw Input Init failure -> nonzero (RAW_INPUT_INIT=FAIL)
//   --backend auto with raw failure -> 0 with FALLBACK=YES (never hidden)

#include "writeover/core/console.h"
#include "writeover/player/input.h"
#include "writeover/player/input_runtime.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

struct ProbeConfig {
    writeover::PointerBackendPreference backend =
        writeover::PointerBackendPreference::Auto;
    writeover::InputContext context = writeover::InputContext::Gameplay;
    int seconds = 60;
};

bool ParseContext(const std::string& s, writeover::InputContext& out) {
    if (s == "gameplay") { out = writeover::InputContext::Gameplay; return true; }
    if (s == "dialogue") { out = writeover::InputContext::Dialogue; return true; }
    if (s == "menu") { out = writeover::InputContext::Menu; return true; }
    if (s == "developer") { out = writeover::InputContext::Developer; return true; }
    return false;
}

bool ParseBackend(const std::string& s, writeover::PointerBackendPreference& out) {
    if (s == "auto") { out = writeover::PointerBackendPreference::Auto; return true; }
    if (s == "raw") { out = writeover::PointerBackendPreference::RawInput; return true; }
    if (s == "cursor") { out = writeover::PointerBackendPreference::Cursor; return true; }
    return false;
}

ProbeConfig ParseArgs(int argc, char** argv) {
    ProbeConfig config;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--backend" && i + 1 < argc) {
            if (!ParseBackend(argv[++i], config.backend)) {
                std::fprintf(stderr, "unknown --backend '%s'\n", argv[i]);
                std::exit(2);
            }
        } else if (arg == "--context" && i + 1 < argc) {
            if (!ParseContext(argv[++i], config.context)) {
                std::fprintf(stderr, "unknown --context '%s'\n", argv[i]);
                std::exit(2);
            }
        } else if (arg == "--seconds" && i + 1 < argc) {
            config.seconds = std::atoi(argv[++i]);
            if (config.seconds <= 0) config.seconds = 60;
        }
    }
    return config;
}

const char* ContextName(writeover::InputContext ctx) {
    switch (ctx) {
    case writeover::InputContext::Gameplay: return "gameplay";
    case writeover::InputContext::Dialogue: return "dialogue";
    case writeover::InputContext::Menu: return "menu";
    case writeover::InputContext::Developer: return "developer";
    default: return "?";
    }
}

} // namespace

int main(int argc, char** argv) {
    const ProbeConfig config = ParseArgs(argc, argv);
    writeover::ConsoleGuard guard;  // restores console mode on exit

    // Production runtime backend selection (Raw Input primary).
    writeover::PointerBackendSelection sel =
        writeover::CreateRuntimeBackendSelection(config.backend);

    // Forced raw that failed must exit nonzero so the human knows raw never
    // started (no silent fallback).
    if (config.backend == writeover::PointerBackendPreference::RawInput &&
        sel.raw_init_failed) {
        std::fprintf(stderr,
                     "RAW_INPUT_INIT = FAIL\n"
                     "RAW_INPUT_BACKEND = unavailable\n"
                     "FALLBACK = NO (forced raw)\n");
        return 1;
    }

    writeover::InputRuntime runtime(std::move(sel.keyboard),
                                    std::move(sel.pointer));
    runtime.Init();
    runtime.SetActiveContext(config.context);

    // Production InputMapper (context-aware defaults).
    writeover::InputMapper mapper;
    writeover::InputState state;

    std::printf("INPUT_PROBE\n");
    std::printf("KEYBOARD_BACKEND=%s\n", "keyboard-console");
    std::printf("POINTER_BACKEND=%s\n", sel.pointer_name);
    std::printf("MOUSE_BUTTON_SOURCE=%s\n", sel.mouse_button_source);
    std::printf("ACTIVE_CONTEXT=%s\n", ContextName(config.context));
    if (sel.raw_init_failed) {
        std::printf("RAW_INPUT_INIT=FAIL\n");
        std::printf("FALLBACK=YES\n");
    }
    std::printf("FOCUS=%d\n", runtime.HasFocus() ? 1 : 0);
    std::printf("RUN=%d sec; move mouse, click LMB/RMB/MMB, press W/A/S/D/Space/Shift; Esc exits\n",
                config.seconds);
    std::fflush(stdout);

    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    const auto deadline = start + std::chrono::seconds(config.seconds);

    const size_t kPauseIdx = static_cast<size_t>(writeover::GameAction::Pause);
    int last_print = -1;
    while (Clock::now() < deadline) {
        runtime.SampleTick(state, mapper);
        if (state.action_pressed[kPauseIdx]) {
            break;  // Esc pressed -> early exit
        }

        // 10Hz single-line diagnostics (never spam thousands of lines).
        const int tenth = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                Clock::now() - start).count() / 100);
        if (tenth != last_print) {
            last_print = tenth;
            std::printf(
                "t=%ds focus=%d ctx=%s dx=%7.1f dy=%7.1f "
                "W=%d A=%d S=%d D=%d Sp=%d Sh=%d LMB=%d RMB=%d MMB=%d "
                "pressed={", tenth, state.has_focus ? 1 : 0,
                ContextName(config.context), state.mouse_delta.x,
                state.mouse_delta.y,
                state.action_down[static_cast<size_t>(writeover::GameAction::MoveForward)] ? 1 : 0,
                state.action_down[static_cast<size_t>(writeover::GameAction::MoveLeft)] ? 1 : 0,
                state.action_down[static_cast<size_t>(writeover::GameAction::MoveBackward)] ? 1 : 0,
                state.action_down[static_cast<size_t>(writeover::GameAction::MoveRight)] ? 1 : 0,
                state.action_down[static_cast<size_t>(writeover::GameAction::Sprint)] ? 1 : 0,
                state.action_down[static_cast<size_t>(writeover::GameAction::Jump)] ? 1 : 0,
                state.action_down[static_cast<size_t>(writeover::GameAction::Fire)] ? 1 : 0,
                state.action_down[static_cast<size_t>(writeover::GameAction::AimDownSights)] ? 1 : 0,
                state.action_down[static_cast<size_t>(writeover::GameAction::Melee)] ? 1 : 0);
            for (size_t i = 0; i < writeover::kGameActionCount; ++i) {
                if (state.action_pressed[i]) {
                    std::printf("%zu ", i);
                }
            }
            std::printf("}\n");
            std::fflush(stdout);
        }

        // Sample at ~120Hz (matches the production sim tick cadence).
#if defined(_WIN32)
        Sleep(8);
#endif
    }

    guard.Restore();
    std::printf("INPUT_PROBE_END\n");
    return 0;
}