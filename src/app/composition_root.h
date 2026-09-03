#pragma once
// Entry surface of the composition root. The app entry (game_main.cpp) parses
// CLI args and delegates the whole assembly + loop to RunComposition; this API
// stays thin so the platform boundary is explicit.

#ifndef WO_COMPOSITION_ROOT_H
#define WO_COMPOSITION_ROOT_H

#include <cstdint>
#include <string>

namespace writeover {

struct GameConfig {
    bool smoke = false;
    uint64_t max_frames = 0;
    uint64_t seed = 0x12345678;
    std::string data_dir = "data";
    int terminal_w = 240;
    int terminal_h = 67;
    bool save_after_smoke = true;
    std::string frame_dump_path;
};

// Builds the world/player/ai/narrative modules, the terminal backend, the
// render module, wires EngineContext, runs the fixed-step engine, performs the
// smoke save, and returns the process exit code (0 = success).
int RunComposition(const GameConfig& config);

} // namespace writeover

#endif // WO_COMPOSITION_ROOT_H