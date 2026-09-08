#pragma once

// Small application-facing interaction helpers. These functions keep the
// authored scene/link records and the camera interaction contract out of the
// composition root without introducing a general target framework.

#include "src/app/scene_runtime.h"
#include "writeover/player/controller.h"
#include "writeover/render/raycaster.h"

namespace writeover {

// Tests the center pixel of the same pinhole camera used by the character
// renderer against one bounded world target and then applies the world's
// existing line-of-sight rule.
bool CameraRayHitsTarget(const LocomotionState& player,
                         int terminal_width, int terminal_height,
                         float vertical_fov_degrees,
                         const IWorldQuery& world,
                         const Vec3& target, float radius, float height);

// Returns the first authored link whose source room and bounds contain the
// player. Content order is deterministic and remains the tie-breaker for
// deliberately overlapping authored links.
const SceneTransition* FindSceneTransitionAt(
    const SceneRuntime& scene, const std::string& source_room,
    const Vec3& player_position);

} // namespace writeover
