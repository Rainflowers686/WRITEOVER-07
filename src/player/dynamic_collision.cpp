#include "dynamic_collision.h"

#include <algorithm>
#include <cmath>

namespace writeover {

bool DynamicActorOverlaps(const AABB& moving_box, const Vec3& actor_feet,
                         float actor_radius, float actor_height) {
    if (!std::isfinite(moving_box.min.x) ||
        !std::isfinite(moving_box.min.y) ||
        !std::isfinite(moving_box.min.z) ||
        !std::isfinite(moving_box.max.x) ||
        !std::isfinite(moving_box.max.y) ||
        !std::isfinite(moving_box.max.z) ||
        moving_box.min.x > moving_box.max.x ||
        moving_box.min.y > moving_box.max.y ||
        moving_box.min.z > moving_box.max.z) {
        return false;
    }
    if (!std::isfinite(actor_feet.x) || !std::isfinite(actor_feet.y) ||
        !std::isfinite(actor_feet.z) || !std::isfinite(actor_radius) ||
        !std::isfinite(actor_height) || actor_radius <= 0.0f ||
        actor_height <= 0.0f) {
        return false;
    }
    if (moving_box.max.z <= actor_feet.z ||
        moving_box.min.z >= actor_feet.z + actor_height) {
        return false;
    }

    const float player_radius = std::max(
        std::fabs(moving_box.max.x - moving_box.min.x),
        std::fabs(moving_box.max.y - moving_box.min.y)) * 0.5f;
    const float combined_radius = player_radius + actor_radius;
    const float player_center_x = (moving_box.min.x + moving_box.max.x) * 0.5f;
    const float player_center_y = (moving_box.min.y + moving_box.max.y) * 0.5f;
    const float dx = player_center_x - actor_feet.x;
    const float dy = player_center_y - actor_feet.y;
    return dx * dx + dy * dy < combined_radius * combined_radius;
}

} // namespace writeover
