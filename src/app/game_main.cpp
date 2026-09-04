// writeover_game entry point. --smoke runs a bounded number of sim ticks,
// renders (using the real terminal backend), saves a smoke save, restores the
// console, and exits 0. Everything else (interactive loop) is release-gate
// work that follows the same composition root.

#include "writeover/core/console.h"
#include "writeover/core/engine.h"
#include "writeover/common/logging.h"

#include "src/app/composition_root.h"
#include "writeover/platform/platform_api.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#ifndef WO_PRODUCT_VERSION
#define WO_PRODUCT_VERSION "0.0.0-dev"
#endif

using namespace writeover;

namespace {

GameConfig ParseArgs(int argc, char** argv) {
    GameConfig config;
    if (argc > 0 && argv[0] != nullptr) config.executable_path = argv[0];
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--smoke") {
            config.smoke = true;
            config.max_frames = 61;
        } else if (arg == "--frames" && i + 1 < argc) {
            config.max_frames = std::strtoull(argv[++i], nullptr, 10);
        } else if (arg == "--seed" && i + 1 < argc) {
            config.seed = std::strtoull(argv[++i], nullptr, 0);
        } else if (arg == "--data-dir" && i + 1 < argc) {
            config.data_dir = argv[++i];
        } else if (arg == "--user-data-dir" && i + 1 < argc) {
            config.user_data_dir = argv[++i];
        } else if (arg == "--width" && i + 1 < argc) {
            config.terminal_w = std::atoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            config.terminal_h = std::atoi(argv[++i]);
        } else if (arg == "--dump-frame" && i + 1 < argc) {
            config.frame_dump_path = argv[++i];
        } else if (arg == "--room" && i + 1 < argc) {
            config.room_id = argv[++i];
        } else if (arg == "--replay" && i + 1 < argc) {
            config.replay_path = argv[++i];
        }
    }
    return config;
}

} // namespace

int main(int argc, char** argv) {
    const GameConfig config = ParseArgs(argc, argv);

    std::fprintf(stderr, "WRITEOVER-07 v%s\n", WO_PRODUCT_VERSION);

    ConsoleGuard guard;                     // restores console mode on exit
    InstallPlatformAtomicReplace();          // MoveFileExW atomic saves

    const int result = RunComposition(config);
    guard.Restore();
    std::fprintf(stderr, "writeover_game exit=%d%s\n", result,
                 config.smoke ? " (smoke)" : "");
    return result;
}
