#pragma once
// Entry surface of the composition root. The app entry (game_main.cpp) parses
// CLI args and delegates the whole assembly + loop to RunComposition; this API
// stays thin so the platform boundary is explicit.

#ifndef WO_COMPOSITION_ROOT_H
#define WO_COMPOSITION_ROOT_H

#include "writeover/common/types.h"

#include <cstdint>
#include <string>

namespace writeover {

struct GameConfig {
    bool smoke = false;
    uint64_t max_frames = 0;
    uint64_t seed = 0x12345678;
    std::string executable_path;
    std::string data_dir;
    std::string user_data_dir;
    int terminal_w = 240;
    int terminal_h = 67;
    bool save_after_smoke = true;
    std::string frame_dump_path;
    std::string room_id;
    std::string replay_path;
    bool camera_override = false;  // deterministic visual evidence camera only
    Vec3 camera_position;
    float camera_yaw = 0.0f;
    float camera_pitch = 0.0f;
};

// Builds the world/player/ai/narrative modules, the terminal backend, the
// render module, wires EngineContext, runs the fixed-step engine, performs the
// smoke save, and returns the process exit code (0 = success).
int RunComposition(const GameConfig& config);

} // namespace writeover

#endif // WO_COMPOSITION_ROOT_H
