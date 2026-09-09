#include "src/app/interaction_runtime.h"

#include <cmath>

namespace writeover {

bool CameraRayHitsTarget(const LocomotionState& player,
                         int terminal_width, int terminal_height,
                         float vertical_fov_degrees,
                         const IWorldQuery& world,
                         const Vec3& target, float radius, float height,
                         float max_distance) {
    if (terminal_width <= 0 || terminal_height <= 0 ||
        !std::isfinite(vertical_fov_degrees) || vertical_fov_degrees <= 1.0f ||
        vertical_fov_degrees >= 179.0f || !std::isfinite(radius) ||
        !std::isfinite(height) || radius <= 0.0f || height <= 0.0f ||
        !std::isfinite(max_distance) || max_distance <= 0.0f ||
        !std::isfinite(target.x) || !std::isfinite(target.y) ||
        !std::isfinite(target.z)) {
        return false;
    }
    const Vec3 eye = player.EyePosition();
    const float half_fov_radians = vertical_fov_degrees * 3.14159265f / 360.0f;
    const float focal = 0.5f * static_cast<float>(terminal_height) /
                        std::tan(half_fov_radians);
    if (!std::isfinite(focal) || focal <= 0.01f) return false;
    const CameraProjection camera(
        eye, player.yaw, player.pitch, terminal_width, terminal_height,
        focal, 0.5f);
    const Vec3 ray = camera.RayDirectionAt(
        static_cast<float>(terminal_width) * 0.5f,
        static_cast<float>(terminal_height) * 0.5f);
    const AABB bounds{
        Vec3{target.x - radius, target.y - radius, target.z},
        Vec3{target.x + radius, target.y + radius, target.z + height}};
    float hit_distance = 0.0f;
    if (!IntersectRayAabb(eye, ray, bounds, hit_distance)) return false;
    if (hit_distance > max_distance) return false;
    // IntersectRayAabb returns the forward exit when the eye is already
    // inside a target proxy.  That is the right nearest-target ordering
    // policy, but using the exit for visibility would trace through a wall
    // behind a nearby terminal or door and falsely reject the target.  An
    // already-overlapped target is visible at the eye plane; test that local
    // point instead of the far side of its proxy.
    const bool eye_inside = eye.x >= bounds.min.x && eye.x <= bounds.max.x &&
                            eye.y >= bounds.min.y && eye.y <= bounds.max.y &&
                            eye.z >= bounds.min.z && eye.z <= bounds.max.z;
    const float visible_distance = eye_inside ? 0.0f : hit_distance;
    const Vec3 hit_point{eye.x + ray.x * visible_distance,
                         eye.y + ray.y * visible_distance,
                         eye.z + ray.z * visible_distance};
    return world.LineOfSight(eye, hit_point, hit_point.z);
}

const SceneTransition* FindSceneTransitionAt(
    const SceneRuntime& scene, const std::string& source_room,
    const Vec3& player_position) {
    if (!std::isfinite(player_position.x) ||
        !std::isfinite(player_position.y)) return nullptr;
    for (const SceneTransition& transition : scene.Transitions()) {
        if (transition.source_room == source_room &&
            player_position.x >= transition.min_x &&
            player_position.x <= transition.max_x &&
            player_position.y >= transition.min_y &&
            player_position.y <= transition.max_y) {
            return &transition;
        }
    }
    return nullptr;
}

} // namespace writeover
