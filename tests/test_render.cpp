#include "tests/test_harness.h"

#include "writeover/render/benchmark.h"
#include "writeover/render/raycaster.h"
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
    return std::fabs(stats.p99_ms - 20.0) < 0.001 &&
           std::fabs(stats.avg_ms - 2.18) < 0.05;
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
}

} // namespace writeover