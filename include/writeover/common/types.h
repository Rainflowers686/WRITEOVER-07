#pragma once
// Basic shared value types: coordinates, colors, grid cells, AABBs.
// Units: meters. Z is up. All gameplay-scale code uses these types only;
// a second Vec library is a forbidden pattern.

#include <cstdint>

namespace writeover {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
};

// Grid coordinates. World position of a cell center is (col+0.5, row+0.5).
struct GridCoord {
    int32_t col = 0;
    int32_t row = 0;

    bool operator==(const GridCoord& o) const { return col == o.col && row == o.row; }
    bool operator<(const GridCoord& o) const {
        return (col < o.col) || (col == o.col && row < o.row);
    }
};

// CellFlags bitmask stored in GridCell::flags.
enum CellFlags : uint8_t {
    CellFlag_Solid     = 1 << 0,  // not traversable
    CellFlag_Door      = 1 << 1,  // door cell
    CellFlag_Breakable = 1 << 2,  // can be destroyed
    CellFlag_Special   = 1 << 3,  // special interaction target
};

// One 1m x 1m grid cell. Height field expresses floor/ceiling.
struct GridCell {
    float floor_height = 0.0f;    // world Z of the floor
    float ceiling_height = 4.0f;  // world Z of the ceiling
    uint8_t material = 0;         // MaterialId, see material table in grid.h
    uint8_t light = 255;          // 0-255 light level
    uint8_t flags = 0;            // CellFlags bitmask

    bool IsSolid() const { return (flags & CellFlag_Solid) != 0; }
    float Clearance() const { return ceiling_height - floor_height; }
};

// Material table (indices shared by content + renderer).
// 0=default wall, 1=metal, 2=glass, 3=dirt, 4=concrete,
// 5=wood, 6=grate, 7=hazard.

// Axis-aligned bounding box in world space.
struct AABB {
    Vec3 min;
    Vec3 max;

    bool Overlaps(const AABB& o) const {
        return min.x < o.max.x && max.x > o.min.x &&
               min.y < o.max.y && max.y > o.min.y &&
               min.z < o.max.z && max.z > o.min.z;
    }
};

// Player dimensions (frozen by round-3 corrections; may be tuned ±small at R0
// hand-feel but the interval ordering below must not conflict).
inline constexpr float kEyeStand   = 1.60f;
inline constexpr float kEyeCrouch  = 1.00f;
inline constexpr float kEyeProne   = 0.42f;
inline constexpr float kColliderStand  = 1.80f;
inline constexpr float kColliderCrouch = 1.20f;
inline constexpr float kColliderProne  = 0.55f;
inline constexpr float kLeanOffset = 0.35f;
inline constexpr float kPlayerRadius = 0.35f;

// Clearance windows (net headroom):
// prone-only  0.65-0.85m, crouch-only 1.30-1.55m, standing >= 1.90m.
inline constexpr float kClearanceProneMax   = 0.85f;
inline constexpr float kClearanceCrouchMin  = 1.30f;
inline constexpr float kClearanceCrouchMax  = 1.55f;
inline constexpr float kClearanceStandMin   = 1.90f;

// Epsilon policy.
inline constexpr float kEpsPosition = 0.001f;
inline constexpr float kEpsVelocity = 0.0001f;
inline constexpr float kEpsTest     = 0.0001f;

} // namespace writeover