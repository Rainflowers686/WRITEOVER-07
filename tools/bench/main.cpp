#include "writeover/render/benchmark.h"
#include "writeover/render/frame_encoder.h"
#include "writeover/render/raycaster.h"
#include "writeover/world/grid.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

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

// Builds a 240x67 (ULTRA120) synthetic CharCell frame with per-cell color
// variation, resembling a real scene.
void MakeTerminalFrame(std::vector<CharCell>& frame, int w, int h,
                       uint8_t phase) {
    frame.resize(static_cast<size_t>(w) * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            CharCell& c = frame[static_cast<size_t>(y) * w + x];
            const uint8_t v = static_cast<uint8_t>((x * 3 + y * 5 + phase * 7) & 0xFF);
            c.code_point = U'\u2588';  // full block
            c.fg_r = static_cast<uint8_t>(40 + v / 3);
            c.fg_g = static_cast<uint8_t>(60 + v / 2);
            c.fg_b = static_cast<uint8_t>(120 + v / 4);
            c.bg_r = static_cast<uint8_t>(10 + v / 8);
            c.bg_g = static_cast<uint8_t>(12 + v / 8);
            c.bg_b = static_cast<uint8_t>(24 + v / 6);
        }
    }
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
    // Budget gate: 240 columns must complete in <4ms worst-1% avg at
    // foundation scale; a REAL check on the reference machine.
    const bool ray_pass = stats.worst_1pct_avg_ms < 4.0;

    // ---- Terminal encoder benchmarks (Issue C.3) ----
    // ULTRA120 frame: 240x67 CharCells (16080 cells).
    constexpr int kTw = 240;
    constexpr int kTh = 67;
    constexpr int kEncFrames = 120;
    std::vector<writeover::CharCell> frame_a;
    std::vector<writeover::CharCell> frame_b;
    std::string scratch;

    // FULL: every encode is a fresh full frame (encoder reset each frame).
    {
        writeover::FrameTimeSampler enc_sampler;
        size_t max_bytes = 0;
        for (int frame = 0; frame < kEncFrames; ++frame) {
            writeover::MakeTerminalFrame(frame_a, kTw, kTh,
                                         static_cast<uint8_t>(frame));
            writeover::AnsiFrameEncoder enc;
            scratch.clear();
            const auto t0 = Clock::now();
            const auto res = enc.Encode(frame_a.data(), kTw, kTh, scratch,
                                        writeover::EncodeMode::ForceFull);
            const auto t1 = Clock::now();
            enc_sampler.AddSample(
                std::chrono::duration<double, std::milli>(t1 - t0).count());
            max_bytes = std::max(max_bytes, scratch.size());
            (void)res;
        }
        writeover::PrintCsv("terminal_encode_full", enc_sampler.Compute());
        std::printf("terminal_full_max_bytes=%zu\n", max_bytes);
    }

    // DELTA typical: encoder persists; consecutive frames differ slightly
    // (a few cells) — the typical 120Hz steady-state case.
    {
        writeover::FrameTimeSampler enc_sampler;
        size_t max_bytes = 0;
        writeover::AnsiFrameEncoder enc;
        writeover::MakeTerminalFrame(frame_a, kTw, kTh, 0);
        scratch.clear();
        enc.Encode(frame_a.data(), kTw, kTh, scratch);  // prime previous
        for (int frame = 1; frame < kEncFrames; ++frame) {
            writeover::MakeTerminalFrame(frame_a, kTw, kTh,
                                         static_cast<uint8_t>(frame % 64));
            // Typical scene: most cells unchanged; perturb a small region.
            frame_b = frame_a;
            for (int i = 0; i < 16; ++i) {
                const size_t idx = static_cast<size_t>((frame * 37 + i * 101) % frame_b.size());
                frame_b[idx].fg_r = static_cast<uint8_t>(frame_b[idx].fg_r ^ 0x40);
            }
            scratch.clear();
            const auto t0 = Clock::now();
            const auto res = enc.Encode(frame_b.data(), kTw, kTh, scratch);
            const auto t1 = Clock::now();
            enc_sampler.AddSample(
                std::chrono::duration<double, std::milli>(t1 - t0).count());
            max_bytes = std::max(max_bytes, scratch.size());
            frame_a.swap(frame_b);
            (void)res;
        }
        writeover::PrintCsv("terminal_encode_delta_typical", enc_sampler.Compute());
        std::printf("terminal_delta_typical_max_bytes=%zu\n", max_bytes);
    }

    // UNCHANGED: encoder persists; frame is identical every time.
    {
        writeover::FrameTimeSampler enc_sampler;
        size_t max_bytes = 0;
        writeover::AnsiFrameEncoder enc;
        writeover::MakeTerminalFrame(frame_a, kTw, kTh, 7);
        scratch.clear();
        enc.Encode(frame_a.data(), kTw, kTh, scratch);  // prime previous
        for (int frame = 0; frame < kEncFrames; ++frame) {
            scratch.clear();
            const auto t0 = Clock::now();
            const auto res = enc.Encode(frame_a.data(), kTw, kTh, scratch);
            const auto t1 = Clock::now();
            enc_sampler.AddSample(
                std::chrono::duration<double, std::milli>(t1 - t0).count());
            max_bytes = std::max(max_bytes, scratch.size());
            (void)res;
        }
        writeover::PrintCsv("terminal_encode_unchanged", enc_sampler.Compute());
        std::printf("terminal_unchanged_max_bytes=%zu\n", max_bytes);
    }

    std::printf("RAYCAST_BUDGET=%s\n", ray_pass ? "PASS" : "FAIL");
    return ray_pass ? 0 : 2;
}