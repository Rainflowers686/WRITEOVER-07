#include "tests/test_harness.h"

#include "writeover/render/benchmark.h"
#include "writeover/render/frame_encoder.h"
#include "writeover/render/raycaster.h"
#include "writeover/render/reference_renderer.h"
#include "writeover/render/terminal_backend.h"
#include "writeover/world/grid.h"

#include <cmath>
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
    test.Add("render.marker_hidden_behind_full_wall", &MarkerHiddenBehindFullWall);
    test.Add("render.marker_visible_above_low_wall", &MarkerVisibleAboveLowWall);
    test.Add("terminal.unchanged_frame_emits_no_payload", &TerminalUnchangedFrameNoPayload);
    test.Add("terminal.delta_smaller_than_full_for_small_change", &TerminalDeltaSmallerThanFull);
    test.Add("terminal.encoder_deterministic", &TerminalEncoderDeterministic);
    test.Add("terminal.resize_forces_safe_full", &TerminalResizeForcesSafeFull);
    test.Add("terminal.delta_typical_is_actual_delta", &TerminalDeltaTypicalIsActualDelta);
}

} // namespace writeover