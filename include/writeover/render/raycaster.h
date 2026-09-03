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

// Projects one occluding segment to screen Y using camera eye height + pitch.
// pitch is clamped to +-30 degrees by the controller; negative = looking down.
WallProjection ProjectWall(const OccludingSegment& seg,
                           float eye_z, float pitch_rad,
                           float focal_px_per_unit, int screen_h);

} // namespace writeover