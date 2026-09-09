#include "writeover/render/raycaster.h"

#include "writeover/common/math.h"

#include <cmath>
#include <limits>

namespace writeover {

namespace {

struct CellInfo {
    float floor_z = 0.0f;
    float ceiling_z = 4.0f;
    bool solid = false;
    uint8_t material = 0;
    uint8_t light = 255;
};

CellInfo CellAt(const GridCell* grid, int grid_w, int grid_h, int col, int row) {
    if (col < 0 || row < 0 || col >= grid_w || row >= grid_h) {
        CellInfo out;
        out.solid = true;  // out-of-bounds = solid wall
        out.floor_z = -1.0f;
        out.ceiling_z = 5.0f;
        return out;
    }
    const GridCell& c = grid[row * grid_w + col];
    CellInfo out;
    out.floor_z = c.floor_height;
    out.ceiling_z = c.ceiling_height;
    out.solid = c.IsSolid();
    out.material = c.material;
    out.light = c.light;
    return out;
}

void PushSegment(RayResult& result, float bottom, float top, float distance,
                 uint8_t material, uint8_t light, uint8_t flag) {
    if (result.segment_count >= kMaxSegmentsPerRay) {
        result.truncated = true;
        return;
    }
    if (top <= bottom) {
        return;  // degenerate, deterministic skip
    }
    OccludingSegment& seg = result.segments[result.segment_count++];
    seg.bottom_z = bottom;
    seg.top_z = top;
    seg.distance = distance;
    seg.material = material;
    seg.light = light;
    seg.flag = flag;
}

} // namespace

RayResult CastColumnRay(const RayConfig& config,
                        const GridCell* grid, int grid_w, int grid_h) {
    RayResult result;

    const float dx = std::cos(config.yaw);
    const float dy = std::sin(config.yaw);

    int col = static_cast<int>(std::floor(config.origin_xy.x));
    int row = static_cast<int>(std::floor(config.origin_xy.y));
    CellInfo start = CellAt(grid, grid_w, grid_h, col, row);

    const float step_x = dx >= 0.0f ? 1.0f : -1.0f;
    const float step_y = dy >= 0.0f ? 1.0f : -1.0f;
    constexpr float kInf = 1e30f;
    const float t_delta_x = std::fabs(dx) > 1e-9f ? std::fabs(1.0f / dx) : kInf;
    const float t_delta_y = std::fabs(dy) > 1e-9f ? std::fabs(1.0f / dy) : kInf;

    float t_max_x = t_delta_x >= kInf
                        ? kInf
                        : (step_x > 0.0f ? (static_cast<float>(col) + 1.0f - config.origin_xy.x)
                                         : (config.origin_xy.x - static_cast<float>(col))) * t_delta_x;
    float t_max_y = t_delta_y >= kInf
                        ? kInf
                        : (step_y > 0.0f ? (static_cast<float>(row) + 1.0f - config.origin_xy.y)
                                         : (config.origin_xy.y - static_cast<float>(row))) * t_delta_y;

    result.final_floor_z = start.floor_z;
    result.final_ceiling_z = start.ceiling_z;

    while (true) {
        if (t_max_x >= config.max_distance && t_max_y >= config.max_distance) {
            break;
        }

        int next_col = col;
        int next_row = row;
        float t_boundary = 0.0f;
        // Corner tie policy (F-21): when both t values are equal within
        // epsilon, the ray passes exactly through a grid corner. Advance
        // BOTH axes in the same step (supercover-style two-axis advance) so
        // no spurious corner-only intermediate cell is visited.
        constexpr float kCornerTieEpsilon = 1e-6f;
        if (std::fabs(t_max_x - t_max_y) <= kCornerTieEpsilon) {
            t_boundary = t_max_x;
            t_max_x += t_delta_x;
            t_max_y += t_delta_y;
            next_col = col + (step_x > 0.0f ? 1 : -1);
            next_row = row + (step_y > 0.0f ? 1 : -1);
        } else if (t_max_x < t_max_y) {
            t_boundary = t_max_x;
            t_max_x += t_delta_x;
            next_col = col + (step_x > 0.0f ? 1 : -1);
        } else {
            t_boundary = t_max_y;
            t_max_y += t_delta_y;
            next_row = row + (step_y > 0.0f ? 1 : -1);
        }

        if (t_boundary > config.max_distance) {
            break;
        }

        const CellInfo near_cell = CellAt(grid, grid_w, grid_h, col, row);
        const CellInfo far_cell = CellAt(grid, grid_w, grid_h, next_col, next_row);

        const float opening_bottom = std::max(near_cell.floor_z, far_cell.floor_z);
        const float opening_top = std::min(near_cell.ceiling_z, far_cell.ceiling_z);
        const float distance = t_boundary;

        if (opening_top <= opening_bottom || far_cell.solid || near_cell.solid) {
            const float b = std::min(near_cell.floor_z, far_cell.floor_z);
            const float t = std::max(near_cell.ceiling_z, far_cell.ceiling_z);
            PushSegment(result, b, t, distance, far_cell.solid ? far_cell.material : near_cell.material,
                        near_cell.light, SegFullWall);
            result.hit_full_occlusion = true;
            result.full_occlusion_distance = distance;
            result.final_floor_z = near_cell.floor_z;
            result.final_ceiling_z = near_cell.ceiling_z;
            break;
        }

        // Bidirectional height boundary faces (F-20 closure).
        if (far_cell.floor_z > near_cell.floor_z) {
            PushSegment(result, near_cell.floor_z, far_cell.floor_z, distance,
                        far_cell.material, far_cell.light, SegFloorRise);
        } else if (far_cell.floor_z < near_cell.floor_z) {
            // Floor drop / trench: the far cell's floor is lower; the trench
            // wall spans [far_floor, near_floor] at this boundary.
            PushSegment(result, far_cell.floor_z, near_cell.floor_z, distance,
                        near_cell.material, near_cell.light, SegFloorDrop);
        }
        if (far_cell.ceiling_z < near_cell.ceiling_z) {
            PushSegment(result, far_cell.ceiling_z, near_cell.ceiling_z, distance,
                        far_cell.material, far_cell.light, SegCeilingDrop);
        } else if (far_cell.ceiling_z > near_cell.ceiling_z) {
            // Ceiling rise: far cell has a higher ceiling; the face at this
            // boundary spans [near_ceiling, far_ceiling].
            PushSegment(result, near_cell.ceiling_z, far_cell.ceiling_z, distance,
                        far_cell.material, far_cell.light, SegCeilingRise);
        }

        col = next_col;
        row = next_row;
        result.final_floor_z = far_cell.floor_z;
        result.final_ceiling_z = far_cell.ceiling_z;
        if (far_cell.solid) {
            break;  // loop re-checks from solid cell next iteration
        }
    }

    return result;
}

CameraProjection::CameraProjection(const Vec3& camera_origin,
                                   float camera_yaw, float camera_pitch,
                                   int width, int height, float vertical_focal,
                                   float horizontal_cell_aspect)
    : origin(camera_origin), yaw(camera_yaw), pitch(camera_pitch),
      screen_width(width), screen_height(height),
      focal_y(std::max(vertical_focal, 0.001f)),
      cell_aspect(std::max(horizontal_cell_aspect, 0.01f)) {
    // screen X is measured in cells, whereas focal_y is measured in rows.
    // A narrow terminal cell therefore has a larger focal length in cell
    // units, not a smaller one.
    focal_x = focal_y / cell_aspect;
}

float CameraProjection::CenterY() const {
    return static_cast<float>(screen_height) * 0.5f +
           std::tan(pitch) * focal_y;
}

float CameraProjection::ColumnYaw(int screen_x) const {
    const float offset = static_cast<float>(screen_x) + 0.5f -
                         static_cast<float>(screen_width) * 0.5f;
    return yaw + std::atan2(offset, focal_x);
}

Vec3 CameraProjection::Forward() const {
    const float cos_pitch = std::cos(pitch);
    return Vec3{std::cos(yaw) * cos_pitch,
                std::sin(yaw) * cos_pitch,
                std::sin(pitch)};
}

Vec3 CameraProjection::Right() const {
    return Vec3{-std::sin(yaw), std::cos(yaw), 0.0f};
}

Vec3 CameraProjection::Up() const {
    const float sin_pitch = std::sin(pitch);
    const float cos_pitch = std::cos(pitch);
    return Vec3{-std::cos(yaw) * sin_pitch,
                -std::sin(yaw) * sin_pitch,
                cos_pitch};
}

Vec3 CameraProjection::RayDirectionAt(float screen_x, float screen_y) const {
    if (focal_x <= 0.001f || focal_y <= 0.001f) {
        return Forward();
    }
    const float camera_x = (screen_x - static_cast<float>(screen_width) * 0.5f) /
                           focal_x;
    // CenterY() is the projected world horizon, not the image-plane centre.
    // The camera forward vector already contains the requested pitch, so the
    // centre pixel must map directly to Forward().  Using CenterY() here
    // would apply pitch a second time and make look-up/look-down interaction
    // rays diverge from the wall/sprite projection contract.
    const float camera_up = (static_cast<float>(screen_height) * 0.5f -
                             screen_y) / focal_y;
    const Vec3 direction = Forward() + Right() * camera_x + Up() * camera_up;
    const float length_sq = direction.x * direction.x +
                            direction.y * direction.y +
                            direction.z * direction.z;
    if (!std::isfinite(length_sq) || length_sq <= 1e-12f) return Forward();
    const float inverse_length = 1.0f / std::sqrt(length_sq);
    return direction * inverse_length;
}

Vec3 CameraProjection::HorizontalColumnDirection(int screen_x) const {
    const float column_yaw = ColumnYaw(screen_x);
    return Vec3{std::cos(column_yaw), std::sin(column_yaw), 0.0f};
}

float CameraProjection::Depth(const Vec3& world_point) const {
    const Vec3 delta = world_point - origin;
    const Vec3 forward = Forward();
    return delta.x * forward.x + delta.y * forward.y + delta.z * forward.z;
}

float CameraProjection::HorizontalDepth(const Vec3& world_point) const {
    const float dx = world_point.x - origin.x;
    const float dy = world_point.y - origin.y;
    return dx * std::cos(yaw) + dy * std::sin(yaw);
}

float CameraProjection::ScreenX(const Vec3& world_point) const {
    const float depth = Depth(world_point);
    if (!std::isfinite(depth) || depth <= 0.001f) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    const Vec3 delta = world_point - origin;
    const Vec3 right = Right();
    const float lateral = delta.x * right.x + delta.y * right.y;
    return static_cast<float>(screen_width) * 0.5f + lateral / depth * focal_x;
}

float CameraProjection::ScreenY(const Vec3& world_point) const {
    const float depth = Depth(world_point);
    if (!std::isfinite(depth) || depth <= 0.001f) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    const Vec3 delta = world_point - origin;
    const Vec3 up = Up();
    const float camera_up = delta.x * up.x + delta.y * up.y + delta.z * up.z;
    return static_cast<float>(screen_height) * 0.5f - camera_up / depth * focal_y;
}

float CameraProjection::ScreenYAtHorizontalDepth(float world_z,
                                                  float horizontal_depth) const {
    if (!std::isfinite(world_z) || !std::isfinite(horizontal_depth) ||
        horizontal_depth <= 0.001f) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    const float relative_z = world_z - origin.z;
    const float cos_pitch = std::cos(pitch);
    const float sin_pitch = std::sin(pitch);
    const float camera_depth = horizontal_depth * cos_pitch +
                               relative_z * sin_pitch;
    if (!std::isfinite(camera_depth) || camera_depth <= 0.001f) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    const float camera_up = -horizontal_depth * sin_pitch +
                            relative_z * cos_pitch;
    return static_cast<float>(screen_height) * 0.5f -
           camera_up / camera_depth * focal_y;
}

float CameraProjection::HorizontalRayDepthToPoint(const Vec3& world_point,
                                                  int screen_x) const {
    const Vec3 ray = HorizontalColumnDirection(screen_x);
    return (world_point.x - origin.x) * ray.x +
           (world_point.y - origin.y) * ray.y;
}

bool IntersectRayAabb(const Vec3& origin, const Vec3& direction,
                      const AABB& bounds, float& out_distance) {
    if (!std::isfinite(origin.x) || !std::isfinite(origin.y) ||
        !std::isfinite(origin.z) || !std::isfinite(direction.x) ||
        !std::isfinite(direction.y) || !std::isfinite(direction.z) ||
        !std::isfinite(bounds.min.x) || !std::isfinite(bounds.min.y) ||
        !std::isfinite(bounds.min.z) || !std::isfinite(bounds.max.x) ||
        !std::isfinite(bounds.max.y) || !std::isfinite(bounds.max.z) ||
        bounds.min.x > bounds.max.x || bounds.min.y > bounds.max.y ||
        bounds.min.z > bounds.max.z) {
        return false;
    }

    float near_distance = -std::numeric_limits<float>::infinity();
    float far_distance = std::numeric_limits<float>::infinity();
    const float o[3] = {origin.x, origin.y, origin.z};
    const float d[3] = {direction.x, direction.y, direction.z};
    const float lower[3] = {bounds.min.x, bounds.min.y, bounds.min.z};
    const float upper[3] = {bounds.max.x, bounds.max.y, bounds.max.z};
    for (int axis = 0; axis < 3; ++axis) {
        if (std::fabs(d[axis]) <= 1e-7f) {
            if (o[axis] < lower[axis] - kEpsPosition ||
                o[axis] > upper[axis] + kEpsPosition) {
                return false;
            }
            continue;
        }
        float t0 = (lower[axis] - o[axis]) / d[axis];
        float t1 = (upper[axis] - o[axis]) / d[axis];
        if (t0 > t1) std::swap(t0, t1);
        near_distance = std::max(near_distance, t0);
        far_distance = std::min(far_distance, t1);
        if (near_distance > far_distance + kEpsPosition) return false;
    }
    if (!std::isfinite(far_distance) || far_distance < 0.0f) return false;
    // A ray that starts inside a target has no positive entry distance.  Use
    // the forward exit distance instead of returning zero, so containing
    // interaction proxies cannot win every nearest-target comparison merely
    // because the player's eye overlaps their AABB.
    out_distance = near_distance >= 0.0f ? near_distance : far_distance;
    return std::isfinite(out_distance);
}

WallProjection ProjectWall(const OccludingSegment& seg,
                           float eye_z, float pitch_rad,
                           float focal_px_per_unit, int screen_h) {
    WallProjection out;
    out.distance = seg.distance;
    out.material = seg.material;
    out.light = seg.light;
    out.flag = seg.flag;

    const CameraProjection projection(Vec3{0.0f, 0.0f, eye_z}, 0.0f,
                                      pitch_rad, 1, screen_h,
                                      focal_px_per_unit);
    out.screen_top_y = projection.ScreenYAtHorizontalDepth(
        seg.top_z, seg.distance);
    out.screen_bottom_y = projection.ScreenYAtHorizontalDepth(
        seg.bottom_z, seg.distance);
    out.visible = out.screen_bottom_y >= 0.0f &&
                  out.screen_top_y <= static_cast<float>(screen_h);
    return out;
}

} // namespace writeover
