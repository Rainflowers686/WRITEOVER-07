#pragma once
// Height-Span Raycaster — CORRECTED 2.5D contract (M-005 closure).
//
// Per screen column we march a ray in the XY plane only (DDA over the 1m
// grid). At every cell-boundary crossing we compare the two walkable vertical
// intervals:
//     openingBottom = max(floorA, floorB)
//     openingTop    = min(ceilA,  ceilB)
// and classify the boundary:
//   * openingTop <= openingBottom  -> full occlusion (one wall segment)
//   * floor rises (floorB > floorA) -> LOWER wall segment [floorA, floorB]
//   * ceiling drops (ceilB < ceilA) -> UPPER wall segment [ceilB, ceilA]
// Camera Z and pitch are used ONLY at projection time to map these world-Z
// segments to screen Y. Floor/ceiling raster is a separate, explicit
// floor-cast strategy (see 09_HEIGHT_SPAN_RAYCASTER_CONTRACT.md).
//
// Capacity invariant (real, not guessed): maxDistance=50m / 1m cells ->
// at most 50 boundaries per ray, each producing at most 2 segments
// (floor-rise + ceiling-drop), full-wall counted as 1 -> worst case 100.
// kMaxSegmentsPerRay = 128 leaves deterministic headroom; beyond it the ray
// truncates with truncated=true (never silent).

#include "writeover/common/types.h"

#include <cstdint>

namespace writeover {

enum SegmentFlag : uint8_t {
    SegFloorRise = 1,   // lower wall caused by a raised floor
    SegCeilingDrop = 2, // upper wall caused by a lowered ceiling
    SegFullWall = 3,    // boundary with no opening at all
    SegFloorDrop = 4,   // trench wall: far floor lower than near floor
    SegCeilingRise = 5, // far ceiling higher than near ceiling
};

struct OccludingSegment {
    float bottom_z = 0.0f;   // world Z of segment bottom
    float top_z = 0.0f;      // world Z of segment top
    float distance = 0.0f;   // ray distance to this boundary
    uint8_t material = 0;
    uint8_t light = 255;
    uint8_t flag = SegFullWall;
};

inline constexpr uint32_t kMaxSegmentsPerRay = 128;
inline constexpr float kDefaultRayMaxDistance = 50.0f;
inline constexpr float kCellSizeMeters = 1.0f;

struct RayConfig {
    Vec2 origin_xy;              // camera position, XY plane
    float yaw = 0.0f;            // radians; +x axis at 0, CCW-positive
    float max_distance = kDefaultRayMaxDistance;
};

struct RayResult {
    OccludingSegment segments[kMaxSegmentsPerRay];
    uint32_t segment_count = 0;
    bool hit_full_occlusion = false;
    float full_occlusion_distance = 0.0f;
    // Interval of the cell where the ray ended (for floor/ceiling raster).
    float final_floor_z = 0.0f;
    float final_ceiling_z = 4.0f;
    bool truncated = false;  // segment capacity exceeded (must be visible in debug)
};

// Truncates the segment list to the visible front kMax... segments (renderer
// uses this when drawing; keeps capacity deterministic).
RayResult CastColumnRay(const RayConfig& config,
                        const GridCell* grid, int grid_w, int grid_h);

// --- Projection (renderer-side; separate from raycast so the raycaster is
// resolution/FOV independent and unit-testable) ---
struct WallProjection {
    float screen_top_y = 0.0f;    // projected top, in pixel rows
    float screen_bottom_y = 0.0f; // projected bottom, in pixel rows
    float distance = 0.0f;
    uint8_t material = 0;
    uint8_t light = 255;
    uint8_t flag = SegFullWall;
    bool visible = true;
};

// focal_px_per_unit = (0.5 * screen_h) / tan(fov_v / 2).
inline constexpr int kMaxProjectionsPerColumn = kMaxSegmentsPerRay;

// One pinhole camera contract shared by the height-span renderer, character
// sprites, floor/ceiling sampling, and player interaction.  Screen X is
// expressed in terminal cells while focal_y is expressed in terminal rows;
// the cell aspect converts the horizontal focal length accordingly.
struct CameraProjection {
    Vec3 origin;
    float yaw = 0.0f;
    float pitch = 0.0f;
    int screen_width = 0;
    int screen_height = 0;
    float focal_y = 1.0f;
    float focal_x = 2.0f;
    float cell_aspect = 0.5f;

    CameraProjection(const Vec3& camera_origin, float camera_yaw,
                     float camera_pitch, int width, int height,
                     float vertical_focal, float horizontal_cell_aspect = 0.5f);

    float CenterY() const;
    float ColumnYaw(int screen_x) const;
    Vec3 Forward() const;
    Vec3 Right() const;
    Vec3 Up() const;
    Vec3 RayDirectionAt(float screen_x, float screen_y) const;
    Vec3 HorizontalColumnDirection(int screen_x) const;

    // Camera-space depth and terminal-cell projection of an arbitrary point.
    float Depth(const Vec3& world_point) const;
    float HorizontalDepth(const Vec3& world_point) const;
    float ScreenX(const Vec3& world_point) const;
    float ScreenY(const Vec3& world_point) const;

    // Projects a vertical wall/sprite edge whose XY hit is on the camera's
    // horizontal ray. This is the same pinhole basis as ScreenY(), expressed
    // in terms of the height-span ray distance.
    float ScreenYAtHorizontalDepth(float world_z,
                                   float horizontal_depth) const;

    // Ray parameter to a point along a particular terminal column's
    // horizontal world ray. Used for per-cell wall/sprite depth comparison.
    float HorizontalRayDepthToPoint(const Vec3& world_point,
                                    int screen_x) const;
};

// Slab intersection for bounded player-facing interaction targets.
// Returns the nearest non-negative ray parameter, with no allocation.
bool IntersectRayAabb(const Vec3& origin, const Vec3& direction,
                      const AABB& bounds, float& out_distance);

// Projects one occluding segment to screen Y using camera eye height + pitch.
// pitch is clamped to +-30 degrees by the controller; negative = looking down.
WallProjection ProjectWall(const OccludingSegment& seg,
                           float eye_z, float pitch_rad,
                           float focal_px_per_unit, int screen_h);

} // namespace writeover
