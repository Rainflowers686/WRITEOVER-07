#include "writeover/ai/perception.h"

#include "writeover/common/math.h"

#include <cmath>

namespace writeover {

namespace {
float SightRangeFor(NPCClass npc_class) {
    switch (npc_class) {
    case NPCClass::Full:
        return PerceptionSystem::kFullSightRange;
    case NPCClass::Medium:
        return PerceptionSystem::kMediumSightRange;
    case NPCClass::Light:
    case NPCClass::Guard:
    default:
        return PerceptionSystem::kLightSightRange;
    }
}
} // namespace

PerceptionResult PerceptionSystem::Update(const NPCInstance& npc,
                                          const IWorldQuery& world,
                                          const Vec3& player_pos,
                                          float player_eye_z,
                                          const std::vector<NoiseSource>& noises,
                                          uint32_t sim_frame) const {
    PerceptionResult result;

    const Vec3 to_player = player_pos - npc.position;
    const float dist2 = to_player.x * to_player.x + to_player.y * to_player.y;
    const float range = SightRangeFor(npc.npc_class);

    if (dist2 <= range * range) {
        const float dist = std::sqrt(dist2);
        // Facing check: dot of facing (yaw) with direction to player.
        const float facing_x = std::cos(npc.yaw);
        const float facing_y = std::sin(npc.yaw);
        const float dir_x = dist > 0.0f ? to_player.x / dist : 0.0f;
        const float dir_y = dist > 0.0f ? to_player.y / dist : 0.0f;
        const float dot = facing_x * dir_x + facing_y * dir_y;
        if (dot >= std::cos(kSightFovRad * 0.5f)) {
            // LOS at player eye height (solid cells + heights block).
            const Vec3 eye{npc.position.x, npc.position.y,
                           npc.position.z + GetPostureParams(Posture::Stand).eye_height};
            if (world.LineOfSight(eye, Vec3{player_pos.x, player_pos.y, player_eye_z},
                                  player_eye_z)) {
                result.sees_player = true;
                result.sight_confidence = 1.0f - dist / range;
            }
        }
    }

    // Hearing: nearest loud noise within remaining hearing factor.
    constexpr float kHearingRange = 8.0f;
    float best_loudness = 0.0f;
    for (const auto& noise : noises) {
        if (sim_frame - noise.sim_frame > 120) {
            continue;  // stale after ~1 second
        }
        const float dx = noise.position.x - npc.position.x;
        const float dy = noise.position.y - npc.position.y;
        const float d = std::sqrt(dx * dx + dy * dy) + 0.001f;
        const float loud = noise.loudness * std::max(0.0f, 1.0f - d / kHearingRange);
        if (loud > best_loudness) {
            best_loudness = loud;
            result.hears_noise = true;
            result.noise_position = noise.position;
            result.noise_loudness = loud;
        }
    }

    return result;
}

} // namespace writeover