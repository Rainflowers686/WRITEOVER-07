#include "writeover/render/benchmark.h"
#include "writeover/render/raycaster.h"
#include "writeover/world/grid.h"

#include <chrono>
#include <cstdio>

namespace writeover {

namespace {
// Synthetic stress grid: 64x64 arena with scattered solid pillars.
Grid MakeStressGrid() {
    Grid grid(64, 64);
    for (int32_t r = 0; r < 64; ++r) {
        for (int32_t c = 0; c < 64; ++c) {
            GridCell cell;
            if ((c % 7 == 0 && r % 5 == 0) || c == 63 || r == 63 || c == 0 || r == 0) {
                cell.flags = CellFlag_Solid;
            }
            grid.SetCell(c, r, cell);
        }
    }
    return grid;
}
} // namespace

} // namespace writeover

int main() {
    using Clock = std::chrono::steady_clock;

    writeover::Grid grid = writeover::MakeStressGrid();
    const int grid_w = grid.Width();
    const int grid_h = grid.Height();
    const writeover::GridCell* cells = grid.Data().data();

    constexpr int kColumns = 240;
    constexpr int kFrames = 240;  // ~2s of render at 120Hz
    writeover::FrameTimeSampler ray_sampler;

    for (int frame = 0; frame < kFrames; ++frame) {
        const auto t0 = Clock::now();
        for (int col = 0; col < kColumns; ++col) {
            writeover::RayConfig cfg;
            cfg.origin_xy = writeover::Vec2{
                2.5f + static_cast<float>(frame % 3),
                2.5f + static_cast<float>(frame % 5)};
            cfg.yaw = 0.02f * static_cast<float>(col) +
                      0.001f * static_cast<float>(frame);
            (void)writeover::CastColumnRay(cfg, cells, grid_w, grid_h);
        }
        const auto t1 = Clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        ray_sampler.AddSample(ms);
    }

    writeover::PrintCsv("raycast_column_sweep", ray_sampler.Compute());
    const writeover::FrameStats stats = ray_sampler.Compute();
    // Budget gate: 240 columns must complete in <4ms p99 at foundation scale;
    // this is a REAL check on the reference machine, not a fabricated number.
    const bool pass = stats.p99_ms < 4.0;
    std::printf("BUDGET=%s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 2;
}