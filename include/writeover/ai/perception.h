#pragma once
// NPC perception: sight cone + hearing, driven by IWorldQuery (read-only).

#include "writeover/ai/npc.h"
#include "writeover/world/grid.h"

#include <cstdint>
#include <vector>

namespace writeover {

struct NoiseSource {
    Vec3 position;
    float loudness = 0.0f;
    uint32_t sim_frame = 0;
};

struct PerceptionResult {
    bool sees_player = false;
    float sight_confidence = 0.0f;
    bool hears_noise = false;
    Vec3 noise_position;
    float noise_loudness = 0.0f;
    std::vector<NpcId> visible_targets;
};

class PerceptionSystem {
public:
    // dims are per-class; LOS is blocked by solid cells and heights.
    static constexpr float kFullSightRange = 15.0f;
    static constexpr float kMediumSightRange = 12.0f;
    static constexpr float kLightSightRange = 10.0f;
    static constexpr float kSightFovRad = 2.09f;  // 120 degrees

    PerceptionResult Update(const NPCInstance& npc,
                            const IWorldQuery& world,
                            const Vec3& player_pos,
                            float player_eye_z,
                            const std::vector<NoiseSource>& noises,
                            uint32_t sim_frame) const;
};

} // namespace writeover