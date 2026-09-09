#include "tests/test_harness.h"

#include "writeover/render/benchmark.h"
#include "writeover/render/character_renderer.h"
#include "writeover/render/frame_encoder.h"
#include "writeover/render/raycaster.h"
#include "writeover/render/reference_renderer.h"
#include "writeover/render/production_renderer.h"
#include "writeover/render/terminal_backend.h"
#include "writeover/world/grid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <vector>

namespace writeover {

namespace {

// Open 10x6 grid with a low wall (floor rise) across the middle.
Grid MakeRayGrid() {
    Grid grid(10, 6);
    for (int32_t r = 0; r < 6; ++r) {
        for (int32_t c = 0; c < 10; ++c) {
            GridCell cell;
            grid.SetCell(c, r, cell);
        }
    }
    // Low wall on row 3: floor rises to 1.2m -> lower boundary wall.
    for (int32_t c = 2; c <= 6; ++c) {
        GridCell wall = grid.GetCell(c, 3);
        wall.floor_height = 1.2f;
        grid.SetCell(c, 3, wall);
    }
    // Outer solid wall at col 9.
    for (int32_t r = 0; r < 6; ++r) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        wall.ceiling_height = 4.0f;
        grid.SetCell(9, r, wall);
    }
    return grid;
}

bool RayFlatHitsWall() {
    const Grid grid = MakeRayGrid();
    RayConfig cfg;
    cfg.origin_xy = Vec2{0.5f, 4.5f};  // row 4, facing +x
    cfg.yaw = 0.0f;
    const RayResult res = CastColumnRay(cfg, grid.Data().data(),
                                        grid.Width(), grid.Height());
    // Outer wall face sits at x=9.0; distance from x=0.5 is 8.5m.
    WO_CHECK(res.hit_full_occlusion);
    WO_CHECK_NEAR(res.full_occlusion_distance, 8.5f, 0.02f);
    if (res.hit_full_occlusion) {
        const OccludingSegment& last = res.segments[res.segment_count - 1];
        return last.flag == SegFullWall;
    }
    return false;
}

bool RayLowWallSegment() {
    const Grid grid = MakeRayGrid();
    RayConfig cfg;
    cfg.origin_xy = Vec2{1.5f, 3.5f};  // row 3 (the low-wall row), facing +x
    cfg.yaw = 0.0f;
    const RayResult res = CastColumnRay(cfg, grid.Data().data(),
                                        grid.Width(), grid.Height());
    // Low wall starts at col 2: boundary col1->col2 (distance 0.5) raises
    // floor 0.0 -> 1.2, so a SegFloorRise appears there.
    bool found_floor_rise = false;
    for (uint32_t i = 0; i < res.segment_count; ++i) {
        if (res.segments[i].flag == SegFloorRise) {
            found_floor_rise = true;
            WO_CHECK_NEAR(res.segments[i].bottom_z, 0.0f, 0.01f);
            WO_CHECK_NEAR(res.segments[i].top_z, 1.2f, 0.01f);
            WO_CHECK_NEAR(res.segments[i].distance, 0.5f, 0.02f);
            if (res.segments[i].distance < 0.45f ||
                res.segments[i].distance > 0.55f) {
                return false;
            }
        }
    }
    return found_floor_rise;
}

bool RayFullOcclusion() {
    const Grid grid = MakeRayGrid();
    RayConfig cfg;
    cfg.origin_xy = Vec2{0.5f, 4.5f};
    cfg.yaw = 0.0f;  // +x -> hits outer wall at distance 8.5
    const RayResult res = CastColumnRay(cfg, grid.Data().data(), 10, 6);
    return res.hit_full_occlusion;
}

bool ProjectionMapsUp() {
    // Eye at 1.6, look level; a segment above eye (2.0..2.5) projects UP
    // (smaller row), below (0.5..1.0) projects DOWN (larger row).
    OccludingSegment upper;
    upper.bottom_z = 2.0f;
    upper.top_z = 2.5f;
    upper.distance = 3.0f;
    OccludingSegment lower;
    lower.bottom_z = 0.5f;
    lower.top_z = 1.0f;
    lower.distance = 3.0f;
    const float focal = 240.0f;
    const WallProjection pu = ProjectWall(upper, 1.6f, 0.0f, focal, 67);
    const WallProjection pl = ProjectWall(lower, 1.6f, 0.0f, focal, 67);
    return pu.screen_top_y < pu.screen_bottom_y &&
           pl.screen_top_y < pl.screen_bottom_y &&
           pu.screen_top_y < pl.screen_top_y;
}

bool CharCellToUtf8Ascii() {
    return CharCellToUtf8(char32_t('A')) == "A";
}

bool IsSingleWidthCjk() {
    // 3D layer must reject CJK wide glyphs; accept ASCII.
    return IsSingleWidthGlyph(char32_t('x')) &&
           !IsSingleWidthGlyph(char32_t(0x4E2D));  // U+4E2D 中
}

bool BenchPercentile() {
    FrameTimeSampler sampler;
    for (int i = 0; i < 100; ++i) {
        sampler.AddSample(2.0);   // 99% fast frames
    }
    sampler.AddSample(20.0);      // the slow 1%
    const FrameStats stats = sampler.Compute();
    // 1% low == average of the slowest 1% == the 20ms outlier.
    return std::fabs(stats.worst_1pct_avg_ms - 20.0) < 0.001 &&
           std::fabs(stats.avg_ms - 2.18) < 0.05;
}

// ---------------------------------------------------------------------------
// HK-2 golden scene tests (G01-G18). Each scene is a small deterministic grid;
// the assertion checks segment presence/type/geometry, not pixel output.
// ---------------------------------------------------------------------------

Grid MakeOpenGrid(int w, int h) {
    Grid grid(w, h);
    for (int32_t r = 0; r < h; ++r) {
        for (int32_t c = 0; c < w; ++c) {
            GridCell cell;
            grid.SetCell(c, r, cell);
        }
    }
    return grid;
}

RayResult CastFrom(const Grid& grid, float x, float y, float yaw) {
    RayConfig cfg;
    cfg.origin_xy = Vec2{x, y};
    cfg.yaw = yaw;
    return CastColumnRay(cfg, grid.Data().data(), grid.Width(), grid.Height());
}

int CountFlag(const RayResult& res, uint8_t flag) {
    int count = 0;
    for (uint32_t i = 0; i < res.segment_count; ++i) {
        if (res.segments[i].flag == flag) {
            ++count;
        }
    }
    return count;
}

// G01: flat corridor — ray reaches the far wall with exactly one full wall.
bool G01FlatCorridor() {
    Grid grid = MakeOpenGrid(8, 4);
    for (int32_t r = 0; r < 4; ++r) {
        GridCell solid;
        solid.flags = CellFlag_Solid;
        grid.SetCell(7, r, solid);
    }
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    WO_CHECK(res.hit_full_occlusion);
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegFullWall)), 1);
    WO_CHECK_NEAR(res.full_occlusion_distance, 6.5f, 0.02f);
    return true;
}

// G02: floor rise — lower wall segment appears at the step.
bool G02FloorRise() {
    Grid grid = MakeOpenGrid(8, 4);
    for (int32_t r = 0; r < 4; ++r) {
        GridCell step = grid.GetCell(3, r);
        step.floor_height = 1.0f;
        grid.SetCell(3, r, step);
    }
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    // Ray continues to the far out-of-bounds wall (full occlusion there);
    // the step segment must be present BEFORE the wall.
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegFloorRise)), 1);
    bool found = false;
    for (uint32_t i = 0; i < res.segment_count; ++i) {
        if (res.segments[i].flag == SegFloorRise) {
            found = true;
            WO_CHECK_NEAR(res.segments[i].bottom_z, 0.0f, 0.01f);
            WO_CHECK_NEAR(res.segments[i].top_z, 1.0f, 0.01f);
        }
    }
    return found;
}

// G03: floor drop / trench — far floor lower than near floor produces the
// trench wall segment (F-20 closure).
bool G03FloorDrop() {
    Grid grid = MakeOpenGrid(8, 4);
    for (int32_t r = 0; r < 4; ++r) {
        GridCell trench = grid.GetCell(3, r);
        trench.floor_height = -0.8f;  // trench below the starting floor
        grid.SetCell(3, r, trench);
    }
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegFloorDrop)), 1);
    bool found = false;
    for (uint32_t i = 0; i < res.segment_count; ++i) {
        if (res.segments[i].flag == SegFloorDrop) {
            found = true;
            WO_CHECK_NEAR(res.segments[i].bottom_z, -0.8f, 0.01f);
            WO_CHECK_NEAR(res.segments[i].top_z, 0.0f, 0.01f);
        }
    }
    return found;
}

// G04: ceiling drop — upper wall segment from the lower ceiling.
bool G04CeilingDrop() {
    Grid grid = MakeOpenGrid(8, 4);
    for (int32_t r = 0; r < 4; ++r) {
        GridCell low = grid.GetCell(3, r);
        low.ceiling_height = 2.5f;
        grid.SetCell(3, r, low);
    }
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegCeilingDrop)), 1);
    bool found = false;
    for (uint32_t i = 0; i < res.segment_count; ++i) {
        if (res.segments[i].flag == SegCeilingDrop) {
            found = true;
            WO_CHECK_NEAR(res.segments[i].bottom_z, 2.5f, 0.01f);
            WO_CHECK_NEAR(res.segments[i].top_z, 4.0f, 0.01f);
        }
    }
    return found;
}

// G05: ceiling rise — far ceiling higher than near ceiling (F-20 closure).
bool G05CeilingRise() {
    Grid grid = MakeOpenGrid(8, 4);
    for (int32_t r = 0; r < 4; ++r) {
        GridCell tall = grid.GetCell(3, r);
        tall.ceiling_height = 5.0f;
        grid.SetCell(3, r, tall);
    }
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegCeilingRise)), 1);
    bool found = false;
    for (uint32_t i = 0; i < res.segment_count; ++i) {
        if (res.segments[i].flag == SegCeilingRise) {
            found = true;
            WO_CHECK_NEAR(res.segments[i].bottom_z, 4.0f, 0.01f);
            WO_CHECK_NEAR(res.segments[i].top_z, 5.0f, 0.01f);
        }
    }
    return found;
}

// G06: floor+ceiling narrow — both change; opening narrows to [1.0, 2.5].
bool G06FloorCeilingNarrow() {
    Grid grid = MakeOpenGrid(8, 4);
    for (int32_t r = 0; r < 4; ++r) {
        GridCell narrow = grid.GetCell(3, r);
        narrow.floor_height = 1.0f;
        narrow.ceiling_height = 2.5f;
        grid.SetCell(3, r, narrow);
    }
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegFloorRise)), 1);
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegCeilingDrop)), 1);
    return true;
}

// G07: full closure — floor above ceiling => single full wall.
bool G07FullClosure() {
    Grid grid = MakeOpenGrid(8, 4);
    for (int32_t r = 0; r < 4; ++r) {
        GridCell closed = grid.GetCell(3, r);
        closed.floor_height = 3.0f;   // floor above ceiling
        closed.ceiling_height = 2.0f;
        grid.SetCell(3, r, closed);
    }
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    WO_CHECK(res.hit_full_occlusion);
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegFullWall)), 1);
    return true;
}

// G08: alternating heights — two floor rises then a drop produce 3 segments.
bool G08AlternatingHeights() {
    Grid grid = MakeOpenGrid(10, 4);
    for (int32_t r = 0; r < 4; ++r) {
        GridCell a = grid.GetCell(3, r);
        a.floor_height = 0.5f;
        grid.SetCell(3, r, a);
        GridCell b = grid.GetCell(5, r);
        b.floor_height = 1.0f;
        grid.SetCell(5, r, b);
        GridCell c = grid.GetCell(7, r);
        c.floor_height = 0.5f;
        grid.SetCell(7, r, c);
    }
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    // Sequence: rise 0->0.5, drop 0.5->0, rise 0->1.0, drop 1.0->0,
    // rise 0->0.5, drop 0.5->0 (then the far OOB wall).
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegFloorRise)), 3);
    WO_CHECK_EQ(static_cast<int64_t>(CountFlag(res, SegFloorDrop)), 3);
    return true;
}

// G09: diagonal corner — 45-degree ray; solid corner-adjacent cells are not
// hit; the diagonal path stays open.
bool G09DiagonalCorner() {
    Grid grid = MakeOpenGrid(8, 8);
    // Cells adjacent to the corner path (1,0) and (0,1) are solid, but the
    // diagonal cell (1,1) is open.
    for (int32_t r = 0; r < 8; ++r) {
        GridCell solid;
        solid.flags = CellFlag_Solid;
        grid.SetCell(1, 0, solid);
        grid.SetCell(0, 1, solid);
        // Far wall beyond the diagonal path.
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(7, 7, wall);
    }
    const float yaw = 3.14159265f * 0.25f;  // 45 deg
    const RayResult res = CastFrom(grid, 0.5f, 0.5f, yaw);
    // Must NOT hit the corner-adjacent solids; must reach the far wall.
    WO_CHECK(res.hit_full_occlusion);
    WO_CHECK_NEAR(res.full_occlusion_distance,
                  (7.0f - 0.5f) * 1.41421356f, 0.05f);
    return true;
}

// G10: exact corner tie — t_max_x == t_max_y; the two-axis advance must not
// visit a spurious corner-only cell. Same geometry as G09 but asserted at
// the DDA level: the first boundary distance equals the diagonal cell's.
bool G10ExactCornerTie() {
    Grid grid = MakeOpenGrid(8, 8);
    // Solid corner-adjacent cells only; diagonal open.
    for (int32_t r = 0; r < 8; ++r) {
        GridCell solid;
        solid.flags = CellFlag_Solid;
        grid.SetCell(1, 0, solid);
        grid.SetCell(0, 1, solid);
    }
    // Wall far away so the ray continues past the corner.
    for (int32_t r = 0; r < 8; ++r) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(7, r, wall);
    }
    const float yaw = 3.14159265f * 0.25f;
    const RayResult res = CastFrom(grid, 0.5f, 0.5f, yaw);
    // First segment (if any) must be the far wall, not a corner-adjacent hit.
    WO_CHECK(res.hit_full_occlusion);
    WO_CHECK_NEAR(res.full_occlusion_distance, 6.5f * 1.41421356f, 0.05f);
    return true;
}

// G11: out of bounds — ray beyond grid edge hits a solid boundary.
bool G11OutOfBounds() {
    Grid grid = MakeOpenGrid(4, 4);
    const RayResult res = CastFrom(grid, 0.5f, 1.5f, 0.0f);
    // Outside col 4 boundary is solid; distance 3.5m to the edge.
    WO_CHECK(res.hit_full_occlusion);
    WO_CHECK_NEAR(res.full_occlusion_distance, 3.5f, 0.02f);
    return true;
}

// G12: near-zero direction — sub-1e-6 direction components must not produce
// NaN/Inf and must march cleanly.
bool G12NearZeroDirection() {
    Grid grid = MakeOpenGrid(6, 6);
    for (int32_t r = 0; r < 6; ++r) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(5, r, wall);
    }
    RayResult res = CastFrom(grid, 0.5f, 1.5f, 1e-7f);  // nearly +x
    WO_CHECK(!std::isnan(res.full_occlusion_distance));
    WO_CHECK(!std::isinf(res.full_occlusion_distance));
    WO_CHECK_NEAR(res.full_occlusion_distance, 4.5f, 0.05f);
    res = CastFrom(grid, 0.5f, 1.5f, 3.14159265f * 0.5f + 1e-7f);  // nearly +y
    WO_CHECK(!std::isnan(res.full_occlusion_distance));
    WO_CHECK(!std::isinf(res.full_occlusion_distance));
    return true;
}

// G13/G14: pitch projection — +30 looks up (segments move down-screen),
// -30 looks down (segments move up-screen).
bool G13PitchPlus30() {
    OccludingSegment seg;
    seg.bottom_z = 1.0f;
    seg.top_z = 2.0f;
    seg.distance = 3.0f;
    const float focal = 120.0f;
    const WallProjection level = ProjectWall(seg, 1.6f, 0.0f, focal, 67);
    const WallProjection up = ProjectWall(seg, 1.6f, 30.0f * 3.14159265f / 180.0f,
                                          focal, 67);
    // Looking up moves everything DOWN the screen (larger rows).
    WO_CHECK(up.screen_top_y > level.screen_top_y);
    return true;
}

bool G14PitchMinus30() {
    OccludingSegment seg;
    seg.bottom_z = 1.0f;
    seg.top_z = 2.0f;
    seg.distance = 3.0f;
    const float focal = 120.0f;
    const WallProjection level = ProjectWall(seg, 1.6f, 0.0f, focal, 67);
    const WallProjection down = ProjectWall(seg, 1.6f, -30.0f * 3.14159265f / 180.0f,
                                            focal, 67);
    // Looking down moves everything UP the screen (smaller rows).
    WO_CHECK(down.screen_top_y < level.screen_top_y);
    return true;
}

// G15/G16: low wall 1.2m — crouch eye (1.0) sees the wall top above the
// horizon (occluding); stand eye (1.6) sees it below the horizon (over the
// wall).
bool G15CrouchBehindLowWall() {
    OccludingSegment wall;  // floor rise 0 -> 1.2
    wall.bottom_z = 0.0f;
    wall.top_z = 1.2f;
    wall.distance = 2.0f;
    const WallProjection p = ProjectWall(wall, 1.0f, 0.0f, 120.0f, 67);
    // Crouch eye is below the wall top: wall top projects ABOVE the horizon.
    WO_CHECK(p.screen_top_y < 33.5f);
    return true;
}

bool G16StandSeesOverSameWall() {
    OccludingSegment wall;
    wall.bottom_z = 0.0f;
    wall.top_z = 1.2f;
    wall.distance = 2.0f;
    const WallProjection p = ProjectWall(wall, 1.6f, 0.0f, 120.0f, 67);
    // Stand eye is above the wall top: wall top projects BELOW the horizon.
    WO_CHECK(p.screen_top_y > 33.5f);
    return true;
}

// G17/G18: player below/above target segment.
bool G17PlayerBelowTarget() {
    OccludingSegment seg;
    seg.bottom_z = 2.0f;
    seg.top_z = 2.5f;
    seg.distance = 3.0f;
    const WallProjection p = ProjectWall(seg, 1.0f, 0.0f, 120.0f, 67);
    // Target is above the eye: projects above the horizon.
    WO_CHECK(p.screen_top_y < 33.5f);
    WO_CHECK(p.screen_bottom_y < 33.5f);
    return true;
}

bool G18PlayerAboveTarget() {
    OccludingSegment seg;
    seg.bottom_z = 1.0f;
    seg.top_z = 1.5f;
    seg.distance = 3.0f;
    const WallProjection p = ProjectWall(seg, 3.0f, 0.0f, 120.0f, 67);
    // Target is below the eye: projects below the horizon.
    WO_CHECK(p.screen_top_y > 33.5f);
    WO_CHECK(p.screen_bottom_y > 33.5f);
    return true;
}

// Reference renderer must produce visible non-trivial output: wall spans
// drawn, not all-space frame (F-22 closure).
bool ReferenceRendererVisible() {
    Grid grid = MakeOpenGrid(12, 6);
    // Low wall across the middle with a gap; far solid wall.
    for (int32_t r = 0; r < 6; ++r) {
        GridCell wall = grid.GetCell(9, r);
        wall.flags = CellFlag_Solid;
        grid.SetCell(9, r, wall);
    }
    for (int32_t c = 2; c <= 8; ++c) {
        GridCell step = grid.GetCell(c, 2);
        step.floor_height = 1.2f;
        grid.SetCell(c, 2, step);
    }
    const int w = 160, h = 45;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    RenderView view;
    view.origin = Vec3{0.5f, 3.5f, kEyeStand};
    view.yaw = 0.0f;
    view.pitch = 0.0f;
    ReferenceMarker marker;
    marker.position = Vec3{4.0f, 3.0f, 1.0f};
    const float focal = 0.5f * static_cast<float>(h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    const int wall_rows = RenderReferenceFrame(
        grid.Data().data(), grid.Width(), grid.Height(), view, &marker,
        frame.data(), w, h, focal);
    WO_CHECK(wall_rows > 100);  // many wall-span rows rasterized
    int non_space = 0;
    for (const auto& cell : frame) {
        if (cell.code_point != U' ') {
            ++non_space;
        }
    }
    WO_CHECK(non_space > w * h / 4);  // a real picture, not a blank frame
    // Marker must be drawn somewhere.
    bool marker_drawn = false;
    for (const auto& cell : frame) {
        if (cell.code_point == marker.glyph) {
            marker_drawn = true;
            break;
        }
    }
    WO_CHECK(marker_drawn);
    return true;
}

// Reference renderer is deterministic: same inputs -> same glyphs.
bool ReferenceRendererDeterministic() {
    Grid grid = MakeOpenGrid(10, 5);
    for (int32_t r = 0; r < 5; ++r) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(8, r, wall);
    }
    const int w = 120, h = 36;
    std::vector<CharCell> a(static_cast<size_t>(w) * h);
    std::vector<CharCell> b(static_cast<size_t>(w) * h);
    RenderView view;
    view.origin = Vec3{0.5f, 2.5f, kEyeStand};
    const float focal = 0.5f * static_cast<float>(h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    RenderReferenceFrame(grid.Data().data(), 10, 5, view, nullptr,
                         a.data(), w, h, focal);
    RenderReferenceFrame(grid.Data().data(), 10, 5, view, nullptr,
                         b.data(), w, h, focal);
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].code_point != b[i].code_point ||
            a[i].fg_r != b[i].fg_r || a[i].bg_r != b[i].bg_r) {
            return false;
        }
    }
    return true;
}

namespace {
// Counts how many cells in the frame equal the given glyph.
int CountGlyph(const CharCell* frame, size_t count, char32_t glyph) {
    int n = 0;
    for (size_t i = 0; i < count; ++i) {
        if (frame[i].code_point == glyph) {
            ++n;
        }
    }
    return n;
}
} // namespace

// Issue G: a marker behind a full-height wall must be hidden.
bool MarkerHiddenBehindFullWall() {
    Grid grid = MakeOpenGrid(12, 6);
    // Full solid wall across row 2 at col 5.
    for (int32_t r = 0; r < 6; ++r) {
        GridCell w;
        w.flags = CellFlag_Solid;
        grid.SetCell(5, r, w);
    }
    const int w = 160, h = 45;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    RenderView view;
    view.origin = Vec3{1.0f, 3.0f, kEyeStand};
    view.yaw = 0.0f;  // facing +x; wall at col 5 is between camera and marker
    ReferenceMarker marker;
    marker.position = Vec3{8.0f, 3.0f, 1.0f};  // behind the wall
    const float focal = 0.5f * static_cast<float>(h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    RenderReferenceFrame(grid.Data().data(), grid.Width(), grid.Height(),
                         view, &marker, frame.data(), w, h, focal);
    // The marker must not be drawn (hidden behind the full wall).
    WO_CHECK_EQ(CountGlyph(frame.data(), frame.size(), marker.glyph), 0);
    return true;
}

// Issue G: a marker whose glyph sits ABOVE a low wall must remain visible.
bool MarkerVisibleAboveLowWall() {
    Grid grid = MakeOpenGrid(12, 6);
    // Low wall (floor rises to 1.2m) across col 5; marker at 2.0m world Z.
    for (int32_t r = 0; r < 6; ++r) {
        GridCell w = grid.GetCell(5, r);
        w.floor_height = 1.2f;
        grid.SetCell(5, r, w);
    }
    const int w = 160, h = 45;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    RenderView view;
    view.origin = Vec3{1.0f, 3.0f, kEyeStand};
    view.yaw = 0.0f;
    ReferenceMarker marker;
    marker.position = Vec3{8.0f, 3.0f, 2.0f};  // above the 1.2m wall
    const float focal = 0.5f * static_cast<float>(h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    RenderReferenceFrame(grid.Data().data(), grid.Width(), grid.Height(),
                         view, &marker, frame.data(), w, h, focal);
    // The marker glyph above the low wall must be visible.
    WO_CHECK(CountGlyph(frame.data(), frame.size(), marker.glyph) > 0);
    return true;
}

// Issue C: an unchanged frame must emit no payload (fast path).
bool TerminalUnchangedFrameNoPayload() {
    const int w = 40, h = 10;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    for (auto& c : frame) {
        c.fg_r = 200;
        c.fg_g = 200;
        c.fg_b = 200;
        c.bg_r = 10;
        c.bg_g = 10;
        c.bg_b = 30;
    }
    AnsiFrameEncoder enc;
    std::string out1;
    const EncodeResult r1 = enc.Encode(frame.data(), w, h, out1);
    WO_CHECK(r1.full);                      // first frame is full
    WO_CHECK(out1.size() > 0);
    std::string out2;
    const EncodeResult r2 = enc.Encode(frame.data(), w, h, out2);
    WO_CHECK(r2.unchanged);                 // no payload
    WO_CHECK_EQ(static_cast<int64_t>(out2.size()), 0);
    return true;
}

// Issue C: a small change produces a DELTA smaller than a full frame.
bool TerminalDeltaSmallerThanFull() {
    const int w = 40, h = 10;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    for (auto& c : frame) {
        c.fg_r = 200; c.fg_g = 200; c.fg_b = 200;
        c.bg_r = 10;  c.bg_g = 10;  c.bg_b = 30;
    }
    AnsiFrameEncoder enc;
    std::string full_out;
    enc.Encode(frame.data(), w, h, full_out);  // prime previous frame
    // Small change: one cell.
    frame[5].code_point = U'X';
    frame[5].fg_r = 255; frame[5].fg_g = 0; frame[5].fg_b = 0;
    std::string delta_out;
    const EncodeResult r2 = enc.Encode(frame.data(), w, h, delta_out);
    WO_CHECK(!r2.unchanged);
    WO_CHECK(!r2.full);                       // delta, not full
    WO_CHECK_EQ(static_cast<int64_t>(r2.changed_cells), 1);
    return delta_out.size() < full_out.size();
}

// Issue C: the encoder is deterministic for identical input.
bool TerminalEncoderDeterministic() {
    const int w = 40, h = 10;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    for (auto& c : frame) {
        c.fg_r = 150; c.fg_g = 180; c.fg_b = 220;
        c.bg_r = 20;  c.bg_g = 30;  c.bg_b = 50;
    }
    AnsiFrameEncoder a, b;
    std::string out_a, out_b;
    a.Encode(frame.data(), w, h, out_a);
    b.Encode(frame.data(), w, h, out_b);
    return out_a == out_b;
}

// Issue D: a dimension change must reset the previous frame and force a
// full-frame encode (never index the old snapshot with the new layout).
bool TerminalResizeForcesSafeFull() {
    AnsiFrameEncoder enc;
    std::vector<CharCell> small(40 * 10);
    std::vector<CharCell> large(60 * 15);
    std::string out;
    // Prime with a small frame.
    auto r1 = enc.Encode(small.data(), 40, 10, out);
    WO_CHECK(r1.full);
    WO_CHECK_EQ(enc.PreviousWidth(), 40);
    WO_CHECK_EQ(enc.PreviousHeight(), 10);
    // Change dimensions: must force a full frame and update the snapshot.
    out.clear();
    auto r2 = enc.Encode(large.data(), 60, 15, out);
    WO_CHECK(r2.full);
    WO_CHECK(!out.empty());
    WO_CHECK_EQ(enc.PreviousWidth(), 60);
    WO_CHECK_EQ(enc.PreviousHeight(), 15);
    // Back to the original size: still a safe full frame after change.
    out.clear();
    auto r3 = enc.Encode(small.data(), 40, 10, out);
    WO_CHECK(r3.full);
    return enc.PreviousWidth() == 40 && enc.PreviousHeight() == 10;
}

// Issue D: a "typical delta" benchmark must measure a SMALL actual change
// (16-64 cells), not a full-screen color phase flip.
bool TerminalDeltaTypicalIsActualDelta() {
    const int w = 60, h = 15;
    std::vector<CharCell> base(static_cast<size_t>(w) * h);
    for (auto& c : base) {
        c.fg_r = 200; c.fg_g = 180; c.fg_b = 160;
        c.bg_r = 30;  c.bg_g = 28;  c.bg_b = 24;
    }
    AnsiFrameEncoder enc;
    std::string out;
    enc.Encode(base.data(), w, h, out);
    // Small local change: 16 cells.
    std::vector<CharCell> changed = base;
    for (int i = 0; i < 16; ++i) {
        changed[static_cast<size_t>(i)].fg_r = 255;
    }
    out.clear();
    const auto res = enc.Encode(changed.data(), w, h, out);
    WO_CHECK_EQ(static_cast<int64_t>(res.changed_cells), 16);
    WO_CHECK(res.changed_cells < static_cast<size_t>(w) * h / 4);
    // Compare with a full frame: delta must be far smaller.
    out.clear();
    AnsiFrameEncoder full_enc;
    full_enc.Encode(changed.data(), w, h, out);
    WO_CHECK(out.size() < 2048);
    return true;
}

} // namespace

bool ProductionHalfBlockFrame() {
    const Grid grid = MakeRayGrid();
    const int cell_w = 40;
    const int cell_h = 20;
    const int logical_w = cell_w;
    const int logical_h = cell_h * 2;
    std::vector<Color> pixels(static_cast<size_t>(logical_w) * logical_h, Color{0,0,0});
    ProductionView view;
    view.origin = Vec3{1.5f, 4.5f, kEyeStand};
    view.yaw = 0.0f;
    const float focal = 0.5f * static_cast<float>(logical_h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    RenderProductionFrame(grid.Data().data(), grid.Width(), grid.Height(),
                          view, pixels.data(), logical_w, logical_h, focal);
    std::vector<CharCell> cells(static_cast<size_t>(cell_w) * cell_h);
    ComposeHalfBlockFrame(pixels.data(), logical_w, logical_h,
                          cells.data(), cell_w, cell_h);
    int half_block_count = 0;
    int dark_count = 0;
    for (const auto& c : cells) {
        if (c.code_point == U'\u2580') ++half_block_count;
        if (c.fg_r < 200 && c.bg_r < 200) ++dark_count;
    }
    WO_CHECK(half_block_count > 0);
    WO_CHECK(dark_count > 0);
    return true;
}

bool CharacterRendererUsesSemanticCells() {
    const Grid grid = MakeRayGrid();
    const int w = 96;
    const int h = 36;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    CharacterView view;
    view.origin = Vec3{1.5f, 4.5f, kEyeStand};
    view.yaw = 0.0f;
    const float focal = 0.5f * static_cast<float>(h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    RenderCharacterFrame(grid.Data().data(), grid.Width(), grid.Height(),
                         view, frame.data(), w, h, focal);
    int visible_glyphs = 0;
    int pixel_packing_glyphs = 0;
    for (const auto& cell : frame) {
        if (cell.code_point != U' ') ++visible_glyphs;
        if (cell.code_point == U'\u2580') {
            ++pixel_packing_glyphs;
        }
    }
    // The character path must carry the scene in glyphs and must not regress
    // to the old logical-pixel/half-block composition contract.
    WO_CHECK(visible_glyphs > 0);
    WO_CHECK_EQ(pixel_packing_glyphs, 0);
    return true;
}

bool CharacterRendererAssetsAndLod() {
    CharacterArtBank bank;
    WO_CHECK(bank.Find(CharacterSpriteKind::SecurityGuard,
                       CharacterLod::Far) != nullptr);
    WO_CHECK(bank.Find(CharacterSpriteKind::FullHuman,
                       CharacterLod::Near) != nullptr);
    WO_CHECK(bank.FindPistol(PistolFrame::IdleA) != nullptr);
    WO_CHECK(bank.FindPistol(PistolFrame::Fire) != nullptr);
    WO_CHECK(SelectCharacterLod(2.0f) == CharacterLod::Near);
    WO_CHECK(SelectCharacterLod(6.0f) == CharacterLod::Mid);
    WO_CHECK(SelectCharacterLod(20.0f) == CharacterLod::Far);
    return true;
}

bool LoadProductionCharacterArtBank(CharacterArtBank& bank) {
    std::filesystem::path root = std::filesystem::current_path();
    for (int level = 0; level < 7; ++level) {
        if (bank.Load((root / "data/characters/b1_character_art.txt").string())) {
            return true;
        }
        if (root == root.root_path()) break;
        root = root.parent_path();
    }
    const std::filesystem::path source_root =
        std::filesystem::path(__FILE__).parent_path().parent_path();
    return bank.Load(
        (source_root / "data/characters/b1_character_art.txt").string());
}

bool CharacterArtPolishHasAuthoredResolution() {
    CharacterArtBank bank;
    WO_CHECK(LoadProductionCharacterArtBank(bank));
    const CharacterArtAsset* mid =
        bank.Find(CharacterSpriteKind::FullHuman, CharacterLod::Mid);
    const CharacterArtAsset* near =
        bank.Find(CharacterSpriteKind::FullHuman, CharacterLod::Near);
    WO_CHECK(mid != nullptr);
    WO_CHECK(near != nullptr);
    WO_CHECK(mid->Height() >= 14 && mid->Height() <= 18);
    WO_CHECK(near->Height() >= 22 && near->Height() <= 28);

    for (const auto frame : {PistolFrame::IdleA, PistolFrame::IdleB,
                             PistolFrame::Fire, PistolFrame::Reload}) {
        const CharacterArtAsset* pistol = bank.FindPistol(frame);
        WO_CHECK(pistol != nullptr);
        WO_CHECK(pistol->Height() >= 24 && pistol->Height() <= 28);
        WO_CHECK(pistol->Width() >= 50);
    }
    return true;
}

bool CharacterDirectionalAssetsAndLodHysteresis() {
    CharacterArtBank bank;
    WO_CHECK(LoadProductionCharacterArtBank(bank));
    const CharacterSpriteKind kinds[] = {
        CharacterSpriteKind::SecurityGuard,
        CharacterSpriteKind::FullHuman,
        CharacterSpriteKind::MaintenanceWorker,
    };
    const CharacterLod lods[] = {
        CharacterLod::Far, CharacterLod::Mid, CharacterLod::Near,
    };
    for (const auto kind : kinds) {
        for (const auto lod : lods) {
            const CharacterArtAsset* front =
                bank.Find(kind, lod, CharacterFacing::Front);
            const CharacterArtAsset* back =
                bank.Find(kind, lod, CharacterFacing::Back);
            const CharacterArtAsset* side =
                bank.Find(kind, lod, CharacterFacing::SideRight);
            WO_CHECK(front != nullptr);
            WO_CHECK(back != nullptr);
            WO_CHECK(side != nullptr);
            if (front == nullptr || back == nullptr || side == nullptr) continue;
            // Directional art must be an authored lookup, not the old
            // camera-facing asset with an eye suppression transform.
            WO_CHECK(front->rows != back->rows || front->ink != back->ink);
            WO_CHECK(front->rows != side->rows || front->ink != side->ink);
        }
    }

    WO_CHECK(SelectCharacterLodHysteretic(4.2f, CharacterLod::Near) ==
             CharacterLod::Near);
    WO_CHECK(SelectCharacterLodHysteretic(4.6f, CharacterLod::Near) ==
             CharacterLod::Mid);
    WO_CHECK(SelectCharacterLodHysteretic(3.4f, CharacterLod::Mid) ==
             CharacterLod::Near);
    WO_CHECK(SelectCharacterLodHysteretic(12.2f, CharacterLod::Mid) ==
             CharacterLod::Mid);
    WO_CHECK(SelectCharacterLodHysteretic(12.6f, CharacterLod::Mid) ==
             CharacterLod::Far);
    WO_CHECK(SelectCharacterLodHysteretic(11.6f, CharacterLod::Far) ==
             CharacterLod::Far);
    WO_CHECK(SelectCharacterLodHysteretic(11.0f, CharacterLod::Far) ==
             CharacterLod::Mid);
    return true;
}

bool CharacterWeaponSlotsHaveDistinctAuthoredArt() {
    CharacterArtBank bank;
    WO_CHECK(LoadProductionCharacterArtBank(bank));
    const auto frames = {PistolFrame::IdleA, PistolFrame::IdleB,
                         PistolFrame::Fire, PistolFrame::Reload};
    for (const auto frame : frames) {
        const CharacterArtAsset* pistol =
            bank.FindWeapon(WeaponSlot::Pistol, frame);
        const CharacterArtAsset* smg = bank.FindWeapon(WeaponSlot::Smg, frame);
        const CharacterArtAsset* stunner =
            bank.FindWeapon(WeaponSlot::Stunner, frame);
        WO_CHECK(pistol != nullptr);
        WO_CHECK(smg != nullptr);
        WO_CHECK(stunner != nullptr);
        if (pistol == nullptr || smg == nullptr || stunner == nullptr) continue;
        WO_CHECK(pistol->rows != smg->rows);
        WO_CHECK(pistol->rows != stunner->rows);
        WO_CHECK(smg->rows != stunner->rows);
    }
    return true;
}

bool CharacterPistolUsesSceneGripAnchor() {
    CharacterArtBank bank;
    WO_CHECK(LoadProductionCharacterArtBank(bank));
    constexpr int width = 240;
    constexpr int height = 67;
    std::vector<CharCell> frame(static_cast<size_t>(width) * height);
    for (auto& cell : frame) {
        cell.code_point = U' ';
        cell.bg_r = 5;
        cell.bg_g = 9;
        cell.bg_b = 14;
    }
    DrawPistolViewmodel(frame.data(), width, height, bank, PistolFrame::IdleA,
                        0.0f);
    int first_x = width;
    int last_x = -1;
    int first_y = height;
    int last_y = -1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (frame[static_cast<size_t>(y) * width + x].code_point == U' ') {
                continue;
            }
            first_x = std::min(first_x, x);
            last_x = std::max(last_x, x);
            first_y = std::min(first_y, y);
            last_y = std::max(last_y, y);
        }
    }
    std::printf("PISTOL_VIEWMODEL_BOUNDS x=%d..%d y=%d..%d\n",
                first_x, last_x, first_y, last_y);
    // The grip/hand anchor places the full authored pose inward from the
    // lower-right edge.  The old right-rectangle placement would start near
    // the 80% width line and leave the muzzle detached from the scene centre.
    WO_CHECK(first_x >= 135 && first_x <= 168);
    WO_CHECK(last_x >= 190 && last_x <= 212);
    WO_CHECK(first_y >= 35 && first_y <= 44);
    WO_CHECK(last_y >= 64 && last_y < height);
    WO_CHECK(last_x - first_x >= 40);
    return true;
}

struct CharacterProjectionMeasurement {
    CharacterLod lod = CharacterLod::Far;
    float raw_scale = 0.0f;
    float effective_scale = 0.0f;
    float projected_height = 0.0f;
    int projected_rows = 0;
    int occupied_rows = 0;
    int first_row = -1;
    int last_row = -1;
    int cap = 0;
    float top = 0.0f;
    float bottom = 0.0f;
    float screen_percent = 0.0f;
};

int CharacterProjectionCap(CharacterLod lod) {
    return lod == CharacterLod::Near ? 44
         : lod == CharacterLod::Mid ? 48
                                    : 24;
}

void PrintCharacterCapStudy(float distance, float focal) {
    constexpr float sprite_height = 1.8f;
    for (const int cap : {40, 44, 48, 52}) {
        const float raw_scale = focal / distance;
        const float effective_scale = std::min(
            raw_scale, static_cast<float>(cap) / sprite_height);
        const float projected_height = sprite_height * effective_scale;
        const int projected_rows = std::clamp(static_cast<int>(std::lround(
            projected_height)), 1, cap);
        std::printf("CHARACTER_CAP_STUDY distance_m=%.2f cap=%d raw_scale=%.3f "
                    "effective_scale=%.3f projected_rows=%d screen_pct=%.1f\n",
                    static_cast<double>(distance), cap,
                    static_cast<double>(raw_scale),
                    static_cast<double>(effective_scale), projected_rows,
                    static_cast<double>(100.0f * projected_height / 67.0f));
    }
}

CharacterProjectionMeasurement MeasureCharacterProjection(
    float distance, const CharacterArtBank& bank) {
    constexpr int cell_w = 240;
    constexpr int cell_h = 67;
    constexpr float sprite_height = 1.8f;
    const float focal = 0.5f * static_cast<float>(cell_h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    const CharacterLod lod = SelectCharacterLod(distance);
    const int cap = CharacterProjectionCap(lod);
    const float raw_scale = focal / distance;
    const float effective_scale = std::min(
        raw_scale, static_cast<float>(cap) / sprite_height);
    const float projected_height = sprite_height * effective_scale;
    const int projected = std::clamp(static_cast<int>(std::lround(
        projected_height)), 1, cap);

    const Grid grid = MakeOpenGrid(16, 8);
    CharacterView view;
    view.origin = Vec3{1.5f, 3.5f, kEyeStand};
    view.yaw = 0.0f;
    std::vector<CharCell> frame(static_cast<size_t>(cell_w) * cell_h);
    for (auto& cell : frame) {
        cell.code_point = U' ';
        cell.bg_r = 5;
        cell.bg_g = 9;
        cell.bg_b = 14;
    }
    DrawCharacterSprites(view,
                         {{Vec3{1.5f + distance, 3.5f, 0.0f},
                           sprite_height, CharacterSpriteKind::FullHuman}},
                         bank, grid.Data().data(), grid.Width(),
                         grid.Height(), frame.data(), cell_w, cell_h, focal);

    CharacterProjectionMeasurement result;
    result.lod = lod;
    result.raw_scale = raw_scale;
    result.effective_scale = effective_scale;
    result.projected_height = projected_height;
    result.projected_rows = projected;
    result.cap = cap;
    result.bottom = static_cast<float>(cell_h) * 0.5f +
                    (kEyeStand - 0.0f) * effective_scale;
    result.top = result.bottom - projected_height;
    result.screen_percent = 100.0f * projected_height /
                            static_cast<float>(cell_h);
    for (int row = 0; row < cell_h; ++row) {
        bool occupied = false;
        for (int col = 0; col < cell_w; ++col) {
            if (frame[static_cast<size_t>(row) * cell_w + col].code_point != U' ') {
                occupied = true;
                break;
            }
        }
        if (occupied) {
            if (result.first_row < 0) result.first_row = row;
            result.last_row = row;
        }
    }
    if (result.first_row >= 0) {
        result.occupied_rows = result.last_row - result.first_row + 1;
    }
    std::printf("CHARACTER_SPRITE_PROJECTION distance_m=%.2f lod=%u "
                "raw_scale=%.3f effective_scale=%.3f projected_rows=%d "
                "visible_rows=%d top=%.2f bottom=%.2f bounds=%d..%d cap=%d "
                "screen_pct=%.1f\n",
                static_cast<double>(distance), static_cast<unsigned>(lod),
                static_cast<double>(result.raw_scale),
                static_cast<double>(result.effective_scale),
                result.projected_rows, result.occupied_rows,
                static_cast<double>(result.top),
                static_cast<double>(result.bottom), result.first_row,
                result.last_row,
                result.cap, static_cast<double>(result.screen_percent));
    return result;
}

bool CharacterSpriteProjectionAtReviewDistances() {
    CharacterArtBank bank;
    std::filesystem::path root = std::filesystem::current_path();
    bool loaded = false;
    for (int level = 0; level < 7 && !loaded; ++level) {
        loaded = bank.Load((root / "data/characters/b1_character_art.txt").string());
        if (root == root.root_path()) break;
        root = root.parent_path();
    }
    if (!loaded) {
        std::filesystem::path source_root =
            std::filesystem::path(__FILE__).parent_path().parent_path();
        loaded = bank.Load(
            (source_root / "data/characters/b1_character_art.txt").string());
    }
    WO_CHECK(loaded);
    if (!loaded) return false;

    constexpr int cell_h = 67;
    const float focal = 0.5f * static_cast<float>(cell_h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    PrintCharacterCapStudy(0.5f, focal);
    PrintCharacterCapStudy(1.0f, focal);

    const std::array<float, 10> distances = {
        0.4f, 0.5f, 0.75f, 1.0f, 1.5f,
        2.0f, 3.0f, 4.0f, 8.0f, 12.0f};
    for (const float distance : distances) {
        const auto measurement = MeasureCharacterProjection(distance, bank);
        WO_CHECK(std::isfinite(measurement.raw_scale));
        WO_CHECK(std::isfinite(measurement.effective_scale));
        WO_CHECK(std::isfinite(measurement.top));
        WO_CHECK(std::isfinite(measurement.bottom));
        WO_CHECK(measurement.projected_rows >= 1);
        WO_CHECK(measurement.projected_rows <= measurement.cap);
        WO_CHECK(measurement.occupied_rows > 0);
        WO_CHECK(measurement.occupied_rows <= cell_h);
        WO_CHECK(measurement.first_row >= 0);
        WO_CHECK(measurement.last_row < cell_h);
        WO_CHECK(measurement.top < static_cast<float>(cell_h));
        WO_CHECK(measurement.bottom >= 0.0f);
        WO_CHECK(measurement.cap == CharacterProjectionCap(measurement.lod));
    }

    const auto near_before_boundary = MeasureCharacterProjection(3.9f, bank);
    const auto mid_at_boundary = MeasureCharacterProjection(4.0f, bank);
    const auto mid_after_boundary = MeasureCharacterProjection(4.1f, bank);
    WO_CHECK(near_before_boundary.lod == CharacterLod::Near);
    WO_CHECK(mid_at_boundary.lod == CharacterLod::Mid);
    WO_CHECK(mid_after_boundary.lod == CharacterLod::Mid);
    WO_CHECK(std::abs(near_before_boundary.projected_rows -
                      mid_at_boundary.projected_rows) <= 1);
    WO_CHECK(std::abs(mid_at_boundary.projected_rows -
                      mid_after_boundary.projected_rows) <= 1);
    std::printf("CHARACTER_SPRITE_LOD_BOUNDARY d=3.9/4.0/4.1 "
                "rows=%d/%d/%d top=%.2f/%.2f/%.2f bottom=%.2f/%.2f/%.2f\n",
                near_before_boundary.projected_rows,
                mid_at_boundary.projected_rows,
                mid_after_boundary.projected_rows,
                static_cast<double>(near_before_boundary.top),
                static_cast<double>(mid_at_boundary.top),
                static_cast<double>(mid_after_boundary.top),
                static_cast<double>(near_before_boundary.bottom),
                static_cast<double>(mid_at_boundary.bottom),
                static_cast<double>(mid_after_boundary.bottom));
    return true;
}

bool CharacterWallPatternUsesWorldCoordinates() {
    Grid grid = MakeOpenGrid(12, 8);
    for (int32_t row = 0; row < grid.Height(); ++row) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        wall.material = 1;
        wall.light = 255;
        grid.SetCell(8, row, wall);
    }

    constexpr int width = 80;
    constexpr int height = 36;
    const float focal = 0.5f * static_cast<float>(height) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    const auto render = [&](float y) {
        std::vector<CharCell> frame(static_cast<size_t>(width) * height);
        CharacterView view;
        view.origin = Vec3{2.5f, y, kEyeStand};
        view.yaw = 0.0f;
        RenderCharacterFrame(grid.Data().data(), grid.Width(), grid.Height(),
                             view, frame.data(), width, height, focal);
        return frame;
    };

    const auto seam_frame = render(2.96f);
    const auto panel_frame = render(3.18f);
    const char32_t seam_glyph = seam_frame[18 * width + width / 2].code_point;
    const char32_t panel_glyph = panel_frame[18 * width + width / 2].code_point;
    std::printf("CHARACTER_WALL_ANCHOR surface_u=2.96/3.18 glyph=U+%04X/U+%04X\n",
                static_cast<unsigned>(seam_glyph),
                static_cast<unsigned>(panel_glyph));
    // The camera translates along the same wall face while the destination
    // screen cell stays fixed. A screen-space pattern would be identical;
    // a world-anchored pattern changes when the hit coordinate crosses a
    // panel seam.
    WO_CHECK(seam_glyph != panel_glyph);
    return seam_glyph == U'║' || seam_glyph == U'╫';
}

bool CharacterPistolKeepsTransparentWhitespace() {
    CharacterArtBank bank;
    const int w = 80;
    const int h = 36;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    for (auto& cell : frame) {
        cell.code_point = U' ';
        cell.bg_r = 11;
        cell.bg_g = 22;
        cell.bg_b = 33;
    }
    DrawPistolViewmodel(frame.data(), w, h, bank, PistolFrame::IdleA,
                        0.0f);
    int weapon_glyphs = 0;
    int untouched_spaces = 0;
    for (const auto& cell : frame) {
        if (cell.code_point == U'=' || cell.code_point == U'>') ++weapon_glyphs;
        if (cell.code_point == U' ' && cell.bg_r == 11 &&
            cell.bg_g == 22 && cell.bg_b == 33) ++untouched_spaces;
    }
    WO_CHECK(weapon_glyphs > 0);
    WO_CHECK(untouched_spaces > 0);
    return true;
}

bool CharacterPortraitIsBoundedCharArt() {
    CharacterArtBank bank;
    const int w = 40;
    const int h = 24;
    std::vector<CharCell> frame(static_cast<size_t>(w) * h);
    for (auto& cell : frame) cell = CharCell{};
    DrawCharacterPortrait(frame.data(), w, h, bank,
                          CharacterSpriteKind::FullHuman, 3, 2, 20, 16);
    bool has_corner = false;
    bool has_face_stroke = false;
    for (const auto& cell : frame) {
        if (cell.code_point == U'╔' || cell.code_point == U'╝') has_corner = true;
        if (cell.code_point == U'o' || cell.code_point == U'O') {
            has_face_stroke = true;
        }
    }
    WO_CHECK(has_corner);
    WO_CHECK(has_face_stroke);
    return true;
}

constexpr int kSpatialTestWidth = 240;
constexpr int kSpatialTestHeight = 67;

float SpatialTestFocal() {
    return 0.5f * static_cast<float>(kSpatialTestHeight) /
           std::tan(60.0f * 3.14159265f / 360.0f);
}

void FillSpatialSentinel(std::vector<CharCell>& frame, char32_t glyph = U'.') {
    for (auto& cell : frame) {
        cell = CharCell{};
        cell.code_point = glyph;
        cell.bg_r = 41;
        cell.bg_g = 47;
        cell.bg_b = 53;
    }
}

bool IsSpatialSpriteCell(const CharCell& cell) {
    return cell.bg_r == 5 && cell.bg_g == 9 && cell.bg_b == 14;
}

int CountSpatialSpriteCells(const std::vector<CharCell>& frame) {
    int count = 0;
    for (const auto& cell : frame) {
        if (IsSpatialSpriteCell(cell)) ++count;
    }
    return count;
}

struct SpatialSpriteBounds {
    int first_x = kSpatialTestWidth;
    int last_x = -1;
    int first_y = kSpatialTestHeight;
    int last_y = -1;
};

SpatialSpriteBounds FindSpatialSpriteBounds(
    const std::vector<CharCell>& frame) {
    SpatialSpriteBounds bounds;
    for (int y = 0; y < kSpatialTestHeight; ++y) {
        for (int x = 0; x < kSpatialTestWidth; ++x) {
            if (!IsSpatialSpriteCell(frame[static_cast<size_t>(y) *
                                           kSpatialTestWidth + x])) {
                continue;
            }
            bounds.first_x = std::min(bounds.first_x, x);
            bounds.last_x = std::max(bounds.last_x, x);
            bounds.first_y = std::min(bounds.first_y, y);
            bounds.last_y = std::max(bounds.last_y, y);
        }
    }
    return bounds;
}

void DrawSpatialSprite(const Grid& grid, const CharacterView& view,
                       CharacterSpriteKind kind, const Vec3& position,
                       float height, std::vector<CharCell>& frame) {
    FillSpatialSentinel(frame);
    DrawCharacterSprites(
        view, {{position, height, kind, 0.0f}}, CharacterArtBank{},
        grid.Data().data(), grid.Width(), grid.Height(), frame.data(),
        kSpatialTestWidth, kSpatialTestHeight, SpatialTestFocal());
}

bool SpatialCameraProjectionContract() {
    const Vec3 camera_position{2.5f, 3.5f, kEyeStand};
    const Vec3 actor{7.5f, 3.5f, 0.9f};
    const Vec3 same_surface{7.5f, 3.5f, 2.2f};
    const std::array<float, 5> yaw_sweep = {
        -0.65f, -0.30f, 0.0f, 0.30f, 0.65f};
    float previous_actor_x = std::numeric_limits<float>::quiet_NaN();
    for (const float yaw : yaw_sweep) {
        const CameraProjection projection(
            camera_position, yaw, 0.0f, kSpatialTestWidth,
            kSpatialTestHeight, SpatialTestFocal(), kCharacterCellAspect);
        const float actor_x = projection.ScreenX(actor);
        const float surface_x = projection.ScreenX(same_surface);
        WO_CHECK(std::isfinite(actor_x));
        WO_CHECK(std::isfinite(surface_x));
        // Points sharing one world XY location must share the same screen-X;
        // their vertical separation is not allowed to create sprite/wall
        // horizontal drift.
        WO_CHECK_NEAR(actor_x, surface_x, 0.001f);
        if (std::isfinite(previous_actor_x)) {
            WO_CHECK(std::fabs(actor_x - previous_actor_x) > 0.01f);
        }
        previous_actor_x = actor_x;

        const int nearest_column = std::clamp(
            static_cast<int>(std::lround(actor_x - 0.5f)), 0,
            kSpatialTestWidth - 1);
        const Vec3 column_ray =
            projection.HorizontalColumnDirection(nearest_column);
        const Vec3 to_actor{actor.x - camera_position.x,
                            actor.y - camera_position.y, 0.0f};
        const float cross = to_actor.x * column_ray.y -
                            to_actor.y * column_ray.x;
        const float dot = to_actor.x * column_ray.x +
                          to_actor.y * column_ray.y;
        // ScreenX and the ray-column mapping are the same pinhole contract;
        // one terminal cell of quantisation is the only allowed error.
        WO_CHECK(std::fabs(std::atan2(cross, dot)) <=
                 std::atan(1.0f / projection.focal_x) + 0.01f);
    }

    const CharacterFacing fixed_actor_facing = SelectCharacterFacing(
        0.0f, actor, camera_position);
    for (const float camera_yaw : yaw_sweep) {
        (void)camera_yaw;
        // Rotating the camera does not change an actor's world-facing pose.
        WO_CHECK(SelectCharacterFacing(0.0f, actor, camera_position) ==
                 fixed_actor_facing);
    }
    return true;
}

bool SpatialActorFacingOrbit() {
    const Vec3 actor{5.5f, 4.5f, 0.0f};
    const float half_pi = 3.14159265f * 0.5f;
    WO_CHECK(SelectCharacterFacing(0.0f, actor,
                                   Vec3{actor.x + 3.0f, actor.y, 1.6f}) ==
             CharacterFacing::Front);
    WO_CHECK(SelectCharacterFacing(0.0f, actor,
                                   Vec3{actor.x, actor.y + 3.0f, 1.6f}) ==
             CharacterFacing::SideLeft);
    WO_CHECK(SelectCharacterFacing(0.0f, actor,
                                   Vec3{actor.x - 3.0f, actor.y, 1.6f}) ==
             CharacterFacing::Back);
    WO_CHECK(SelectCharacterFacing(0.0f, actor,
                                   Vec3{actor.x, actor.y - 3.0f, 1.6f}) ==
             CharacterFacing::SideRight);
    (void)half_pi;
    return true;
}

bool SpatialLowWallPartialOcclusion() {
    Grid clear_grid = MakeOpenGrid(12, 8);
    Grid low_wall = clear_grid;
    for (int32_t col = 2; col <= 10; ++col) {
        GridCell cell = low_wall.GetCell(col, 3);
        cell.floor_height = 1.0f;
        cell.material = 4;
        low_wall.SetCell(col, 3, cell);
    }

    CharacterView view;
    view.origin = Vec3{1.5f, 2.5f, kEyeStand};
    view.yaw = 0.0f;
    std::vector<CharCell> clear_frame(
        static_cast<size_t>(kSpatialTestWidth) * kSpatialTestHeight);
    std::vector<CharCell> wall_frame = clear_frame;
    DrawSpatialSprite(clear_grid, view, CharacterSpriteKind::FullHuman,
                      Vec3{7.5f, 3.5f, 0.0f}, 1.8f, clear_frame);
    DrawSpatialSprite(low_wall, view, CharacterSpriteKind::FullHuman,
                      Vec3{7.5f, 3.5f, 0.0f}, 1.8f, wall_frame);
    const int clear_count = CountSpatialSpriteCells(clear_frame);
    const int wall_count = CountSpatialSpriteCells(wall_frame);
    WO_CHECK(clear_count > 0);
    WO_CHECK(wall_count > 0);
    WO_CHECK(wall_count < clear_count);

    bool upper_survives = false;
    bool lower_is_occluded = false;
    for (int y = 0; y < kSpatialTestHeight; ++y) {
        int clear_row = 0;
        int wall_row = 0;
        for (int x = 0; x < kSpatialTestWidth; ++x) {
            clear_row += IsSpatialSpriteCell(
                clear_frame[static_cast<size_t>(y) * kSpatialTestWidth + x]);
            wall_row += IsSpatialSpriteCell(
                wall_frame[static_cast<size_t>(y) * kSpatialTestWidth + x]);
        }
        if (y < kSpatialTestHeight / 2 && wall_row > 0) upper_survives = true;
        if (clear_row > wall_row && clear_row > 0) lower_is_occluded = true;
    }
    WO_CHECK(upper_survives);
    WO_CHECK(lower_is_occluded);
    return true;
}

bool SpatialPillarPartialOcclusion() {
    Grid clear_grid = MakeOpenGrid(12, 8);
    Grid pillar_grid = clear_grid;
    GridCell pillar = pillar_grid.GetCell(5, 3);
    pillar.flags = CellFlag_Solid;
    pillar.material = 1;
    pillar_grid.SetCell(5, 3, pillar);

    CharacterView view;
    view.origin = Vec3{1.5f, 3.5f, kEyeStand};
    view.yaw = 0.0f;
    const Vec3 actor_position{7.5f, 4.2f, 0.0f};
    std::vector<CharCell> clear_frame(
        static_cast<size_t>(kSpatialTestWidth) * kSpatialTestHeight);
    std::vector<CharCell> pillar_frame = clear_frame;
    DrawSpatialSprite(clear_grid, view, CharacterSpriteKind::FullHuman,
                      actor_position, 1.8f, clear_frame);
    DrawSpatialSprite(pillar_grid, view, CharacterSpriteKind::FullHuman,
                      actor_position, 1.8f, pillar_frame);
    const int clear_count = CountSpatialSpriteCells(clear_frame);
    const int pillar_count = CountSpatialSpriteCells(pillar_frame);
    WO_CHECK(clear_count > 0);
    WO_CHECK(pillar_count > 0);
    WO_CHECK(pillar_count < clear_count);
    const SpatialSpriteBounds bounds = FindSpatialSpriteBounds(pillar_frame);
    WO_CHECK(bounds.last_x - bounds.first_x >= 2);
    return true;
}

bool SpatialCharacterOpacity() {
    CharacterArtBank bank;
    WO_CHECK(LoadProductionCharacterArtBank(bank));
    const CharacterArtAsset* near =
        bank.Find(CharacterSpriteKind::FullHuman, CharacterLod::Near);
    WO_CHECK(near != nullptr);
    bool has_opaque_empty = false;
    bool has_transparent = false;
    bool has_enclosed_gap = false;
    for (int y = 0; y < near->Height(); ++y) {
        bool occupied_before = false;
        for (int x = 0; x < near->Width(); ++x) {
            const CharacterCellOpacity opacity = near->OpacityAt(x, y);
            has_opaque_empty = has_opaque_empty ||
                               opacity == CharacterCellOpacity::OpaqueEmpty;
            has_transparent = has_transparent ||
                              opacity == CharacterCellOpacity::Transparent;
            if (opacity == CharacterCellOpacity::Transparent &&
                occupied_before) {
                for (int right = x + 1; right < near->Width(); ++right) {
                    if (near->OpacityAt(right, y) !=
                        CharacterCellOpacity::Transparent) {
                        has_enclosed_gap = true;
                        break;
                    }
                }
            }
            if (opacity != CharacterCellOpacity::Transparent) {
                occupied_before = true;
            }
        }
    }
    WO_CHECK(has_opaque_empty);
    WO_CHECK(has_transparent);
    WO_CHECK(has_enclosed_gap);

    Grid grid = MakeOpenGrid(10, 8);
    CharacterView view;
    view.origin = Vec3{1.5f, 3.5f, 1.0f};
    view.yaw = 0.0f;
    std::vector<CharCell> frame(
        static_cast<size_t>(kSpatialTestWidth) * kSpatialTestHeight);
    FillSpatialSentinel(frame);
    DrawCharacterSprites(
        view, {{Vec3{3.5f, 3.5f, 0.0f}, 1.8f,
                CharacterSpriteKind::FullHuman, 0.0f}},
        bank, grid.Data().data(), grid.Width(), grid.Height(), frame.data(),
        kSpatialTestWidth, kSpatialTestHeight, SpatialTestFocal());
    const SpatialSpriteBounds bounds = FindSpatialSpriteBounds(frame);
    WO_CHECK(bounds.last_x >= bounds.first_x);
    WO_CHECK(bounds.last_y >= bounds.first_y);
    int opaque_blank_cells = 0;
    int visible_glyph_cells = 0;
    int preserved_negative_space = 0;
    for (int y = bounds.first_y; y <= bounds.last_y; ++y) {
        for (int x = bounds.first_x; x <= bounds.last_x; ++x) {
            const CharCell& cell =
                frame[static_cast<size_t>(y) * kSpatialTestWidth + x];
            if (!IsSpatialSpriteCell(cell)) {
                if (cell.code_point == U'.') ++preserved_negative_space;
                continue;
            }
            if (cell.code_point == U' ') ++opaque_blank_cells;
            else ++visible_glyph_cells;
        }
    }
    WO_CHECK(opaque_blank_cells > 0);
    WO_CHECK(visible_glyph_cells > 0);
    WO_CHECK(preserved_negative_space > 0);
    return true;
}

bool SpatialInteractionRayPitchAndOcclusion() {
    const int width = kSpatialTestWidth;
    const int height = kSpatialTestHeight;
    const float focal = SpatialTestFocal();
    const float center_x = static_cast<float>(width - 1) * 0.5f;
    const float center_y = static_cast<float>(height - 1) * 0.5f;
    const Vec3 eye{0.5f, 1.5f, kEyeStand};

    const CameraProjection up_camera(eye, 0.0f, 0.35f, width, height,
                                     focal, kCharacterCellAspect);
    const Vec3 up_ray = up_camera.RayDirectionAt(center_x, center_y);
    float distance = 0.0f;
    const AABB ceiling_target{{1.8f, 1.30f, 2.0f},
                              {2.3f, 1.70f, 2.8f}};
    WO_CHECK(IntersectRayAabb(eye, up_ray, ceiling_target, distance));
    // A containing interaction proxy must return its forward exit distance,
    // not zero.  Zero would make a nearby proxy win every nearest-target
    // comparison merely because the player's eye starts inside its AABB.
    const AABB containing_target{{0.0f, 1.0f, 1.2f},
                                 {1.0f, 2.0f, 2.0f}};
    const Vec3 forward_ray{1.0f, 0.0f, 0.0f};
    WO_CHECK(IntersectRayAabb(eye, forward_ray, containing_target, distance));
    WO_CHECK_NEAR(distance, 0.5f, 0.01f);
    const CameraProjection down_camera(eye, 0.0f, -0.35f, width, height,
                                       focal, kCharacterCellAspect);
    const Vec3 down_ray = down_camera.RayDirectionAt(center_x, center_y);
    const AABB floor_body{{2.7f, 1.25f, 0.0f}, {3.3f, 1.75f, 0.8f}};
    WO_CHECK(IntersectRayAabb(eye, down_ray, floor_body, distance));
    WO_CHECK(!IntersectRayAabb(eye, down_ray, ceiling_target, distance));
    WO_CHECK(!IntersectRayAabb(eye, up_ray, floor_body, distance));

    Grid open_grid = MakeOpenGrid(8, 3);
    GridWorldQuery open_query(&open_grid);
    const Vec3 target{4.5f, 1.5f, 1.0f};
    WO_CHECK(open_query.LineOfSight(eye, target, target.z));
    Grid blocked_grid = open_grid;
    GridCell wall = blocked_grid.GetCell(2, 1);
    wall.flags = CellFlag_Solid;
    blocked_grid.SetCell(2, 1, wall);
    GridWorldQuery blocked_query(&blocked_grid);
    WO_CHECK(!blocked_query.LineOfSight(eye, target, target.z));
    return true;
}

bool SpatialBodyWorldPresentation() {
    CharacterArtBank bank;
    WO_CHECK(LoadProductionCharacterArtBank(bank));
    WO_CHECK(bank.Find(CharacterSpriteKind::BodyUnconscious,
                       CharacterLod::Near) != nullptr);
    WO_CHECK(bank.Find(CharacterSpriteKind::BodyDead,
                       CharacterLod::Near) != nullptr);

    Grid grid = MakeOpenGrid(10, 8);
    CharacterView view;
    view.origin = Vec3{1.5f, 3.5f, 1.0f};
    view.yaw = 0.0f;
    const auto render = [&](CharacterSpriteKind kind, const Vec3& position,
                            float height) {
        std::vector<CharCell> frame(
            static_cast<size_t>(kSpatialTestWidth) * kSpatialTestHeight);
        FillSpatialSentinel(frame);
        DrawCharacterSprites(
            view, {{position, height, kind, 0.0f}}, bank,
            grid.Data().data(), grid.Width(), grid.Height(), frame.data(),
            kSpatialTestWidth, kSpatialTestHeight, SpatialTestFocal());
        return frame;
    };
    const auto body_near = render(CharacterSpriteKind::BodyUnconscious,
                                  Vec3{4.5f, 3.5f, 0.0f}, 0.45f);
    const auto body_far = render(CharacterSpriteKind::BodyUnconscious,
                                 Vec3{5.5f, 3.5f, 0.0f}, 0.45f);
    const auto standing = render(CharacterSpriteKind::FullHuman,
                                 Vec3{4.5f, 3.5f, 0.0f}, 1.8f);
    const SpatialSpriteBounds near_bounds = FindSpatialSpriteBounds(body_near);
    const SpatialSpriteBounds far_bounds = FindSpatialSpriteBounds(body_far);
    const SpatialSpriteBounds standing_bounds =
        FindSpatialSpriteBounds(standing);
    WO_CHECK(near_bounds.last_x >= near_bounds.first_x);
    WO_CHECK(near_bounds.last_y >= near_bounds.first_y);
    WO_CHECK(far_bounds.last_x >= far_bounds.first_x);
    WO_CHECK(near_bounds.first_y != far_bounds.first_y ||
             near_bounds.last_y != far_bounds.last_y);
    WO_CHECK(standing_bounds.last_y >= standing_bounds.first_y);
    WO_CHECK(near_bounds.last_y - near_bounds.first_y <
             standing_bounds.last_y - standing_bounds.first_y);

    std::vector<CharCell> hidden(
        static_cast<size_t>(kSpatialTestWidth) * kSpatialTestHeight);
    FillSpatialSentinel(hidden);
    DrawCharacterSprites(view, {}, bank, grid.Data().data(), grid.Width(),
                         grid.Height(), hidden.data(), kSpatialTestWidth,
                         kSpatialTestHeight, SpatialTestFocal());
    WO_CHECK_EQ(CountSpatialSpriteCells(hidden), 0);
    return true;
}

void RegisterRenderTests(TestHarness& test) {
    test.Add("ray.flat_hits_wall", &RayFlatHitsWall);
    test.Add("ray.low_wall_floor_rise", &RayLowWallSegment);
    test.Add("ray.full_occlusion", &RayFullOcclusion);
    test.Add("ray.projection_up_down", &ProjectionMapsUp);
    test.Add("utf.ascii_single_width", &CharCellToUtf8Ascii);
    test.Add("utf.cjk_blocked_in_3d", &IsSingleWidthCjk);
    test.Add("bench.p99_is_one_percent_low", &BenchPercentile);
    // HK-2 golden scenes (G01-G18).
    test.Add("g01_flat_corridor", &G01FlatCorridor);
    test.Add("g02_floor_rise", &G02FloorRise);
    test.Add("g03_floor_drop", &G03FloorDrop);
    test.Add("g04_ceiling_drop", &G04CeilingDrop);
    test.Add("g05_ceiling_rise", &G05CeilingRise);
    test.Add("g06_floor_ceiling_narrow", &G06FloorCeilingNarrow);
    test.Add("g07_full_closure", &G07FullClosure);
    test.Add("g08_alternating_heights", &G08AlternatingHeights);
    test.Add("g09_diagonal_corner", &G09DiagonalCorner);
    test.Add("g10_exact_corner_tie", &G10ExactCornerTie);
    test.Add("g11_out_of_bounds", &G11OutOfBounds);
    test.Add("g12_near_zero_direction", &G12NearZeroDirection);
    test.Add("g13_pitch_plus_30", &G13PitchPlus30);
    test.Add("g14_pitch_minus_30", &G14PitchMinus30);
    test.Add("g15_crouch_behind_low_wall", &G15CrouchBehindLowWall);
    test.Add("g16_stand_sees_over_wall", &G16StandSeesOverSameWall);
    test.Add("g17_player_below_target", &G17PlayerBelowTarget);
    test.Add("g18_player_above_target", &G18PlayerAboveTarget);
    test.Add("reference_renderer_visible", &ReferenceRendererVisible);
    test.Add("reference_renderer_deterministic", &ReferenceRendererDeterministic);
    test.Add("production.half_block_frame", &ProductionHalfBlockFrame);
    test.Add("character.semantic_cells", &CharacterRendererUsesSemanticCells);
    test.Add("character.assets_and_lod", &CharacterRendererAssetsAndLod);
    test.Add("character.art_polish_authored_resolution",
             &CharacterArtPolishHasAuthoredResolution);
    test.Add("character.directional_assets_and_lod_hysteresis",
             &CharacterDirectionalAssetsAndLodHysteresis);
    test.Add("character.weapon_slots_have_distinct_art",
             &CharacterWeaponSlotsHaveDistinctAuthoredArt);
    test.Add("character.pistol_scene_grip_anchor",
             &CharacterPistolUsesSceneGripAnchor);
    test.Add("character.sprite_projection_review_distances",
             &CharacterSpriteProjectionAtReviewDistances);
    test.Add("character.wall_pattern_world_anchor",
             &CharacterWallPatternUsesWorldCoordinates);
    test.Add("character.pistol_transparency", &CharacterPistolKeepsTransparentWhitespace);
    test.Add("character.portrait_bounded_char_art", &CharacterPortraitIsBoundedCharArt);
    test.Add("spatial.camera_projection_contract",
             &SpatialCameraProjectionContract);
    test.Add("spatial.actor_facing_orbit", &SpatialActorFacingOrbit);
    test.Add("spatial.low_wall_partial_occlusion",
             &SpatialLowWallPartialOcclusion);
    test.Add("spatial.pillar_partial_occlusion",
             &SpatialPillarPartialOcclusion);
    test.Add("spatial.character_opacity", &SpatialCharacterOpacity);
    test.Add("spatial.interaction_ray_pitch_and_wall",
             &SpatialInteractionRayPitchAndOcclusion);
    test.Add("spatial.body_world_presentation",
             &SpatialBodyWorldPresentation);
    test.Add("render.marker_hidden_behind_full_wall", &MarkerHiddenBehindFullWall);
    test.Add("render.marker_visible_above_low_wall", &MarkerVisibleAboveLowWall);
    test.Add("terminal.unchanged_frame_emits_no_payload", &TerminalUnchangedFrameNoPayload);
    test.Add("terminal.delta_smaller_than_full_for_small_change", &TerminalDeltaSmallerThanFull);
    test.Add("terminal.encoder_deterministic", &TerminalEncoderDeterministic);
    test.Add("terminal.resize_forces_safe_full", &TerminalResizeForcesSafeFull);
    test.Add("terminal.delta_typical_is_actual_delta", &TerminalDeltaTypicalIsActualDelta);
}

} // namespace writeover
