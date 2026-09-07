#pragma once

// Private player-side geometry helper.  Runtime actor identity stays in the
// composition root; this helper only answers the bounded spatial question the
// player controller needs: does a moving player box overlap a standing actor?

#include "writeover/common/types.h"

namespace writeover {

bool DynamicActorOverlaps(const AABB& moving_box, const Vec3& actor_feet,
                         float actor_radius, float actor_height);

} // namespace writeover
