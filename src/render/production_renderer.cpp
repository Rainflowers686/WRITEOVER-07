#include "writeover/render/production_renderer.h"

#include "writeover/render/raycaster.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace writeover {

namespace {

constexpr float kMaxPitchRad = 30.0f * 3.14159265f / 180.0f;

uint8_t Clamp8(float v) {
    return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
}

float Saturate(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

uint32_t Hash2(int x, int y) {
    uint32_t h = static_cast<uint32_t>(x) * 0x9E3779B1u;
    h ^= static_cast<uint32_t>(y) * 0x85EBCA6Bu;
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    return h;
}

float Noise01(int x, int y) {
    return static_cast<float>(Hash2(x, y) & 0xFFFF) / 65535.0f;
}

Color MaterialBase(uint8_t material) {
    switch (material) {
    case 1: return {86, 96, 108};    // metal panel
    case 2: return {70, 125, 138};   // stylized glass
    case 3: return {70, 58, 48};     // dirt/grime
    case 4: return {88, 84, 78};     // concrete
    case 5: return {92, 68, 48};     // wood
    case 6: return {74, 92, 100};    // grate
    case 7: return {132, 104, 32};   // hazard
    default: return {100, 92, 82};   // facility wall
    }
}

Color MaterialColor(uint8_t material, float distance, int row, int col, uint8_t light) {
    const Color base = MaterialBase(material);
    float r = static_cast<float>(base.r);
    float g = static_cast<float>(base.g);
    float b = static_cast<float>(base.b);

    // Depth fade: keep dark, never pastel.
    const float depth = Saturate(1.0f - distance * 0.035f);
    r *= (0.35f + 0.65f * depth);
    g *= (0.35f + 0.65f * depth);
    b *= (0.35f + 0.65f * depth);

    // Simple vertical panel seams / texture variation.
    const float pattern = Noise01(col / 6, row / 4);
    const float panel = (row % 12 < 1) ? 0.82f : 1.0f;
    r *= panel * (0.92f + 0.08f * pattern);
    g *= panel * (0.92f + 0.08f * pattern);
    b *= panel * (0.92f + 0.08f * pattern);

    const float l = static_cast<float>(light) / 255.0f;
    return {Clamp8(r * l), Clamp8(g * l), Clamp8(b * l)};
}

Color FloorColor(int row, int logical_h, float distance) {
    // Dark, warm, not bright. Gradient with depth and subtle speckle.
    const float t = static_cast<float>(row) / static_cast<float>(std::max(1, logical_h));
    const float depth = Saturate(1.0f - distance * 0.02f);
    const float n = Noise01(row * 7, static_cast<int>(distance * 13.0f)) * 0.08f;
    const float r = (38.0f + 24.0f * t) * (0.55f + 0.45f * depth) + n * 20.0f;
    const float g = (32.0f + 18.0f * t) * (0.55f + 0.45f * depth) + n * 16.0f;
    const float b = (26.0f + 12.0f * t) * (0.55f + 0.45f * depth) + n * 12.0f;
    return {Clamp8(r), Clamp8(g), Clamp8(b)};
}

Color CeilingColor(int row, int logical_h, float distance) {
    // Even darker than floor.
    const float t = 1.0f - static_cast<float>(row) / static_cast<float>(std::max(1, logical_h));
    const float depth = Saturate(1.0f - distance * 0.02f);
    const float n = Noise01(row * 11, static_cast<int>(distance * 17.0f)) * 0.05f;
    const float r = (18.0f + 12.0f * t) * (0.50f + 0.50f * depth) + n * 10.0f;
    const float g = (22.0f + 14.0f * t) * (0.50f + 0.50f * depth) + n * 10.0f;
    const float b = (34.0f + 18.0f * t) * (0.50f + 0.50f * depth) + n * 12.0f;
    return {Clamp8(r), Clamp8(g), Clamp8(b)};
}

} // namespace

void RenderProductionFrame(const GridCell* cells, int grid_w, int grid_h,
                           const ProductionView& view,
                           Color* out_pixels, int logical_w, int logical_h,
                           float focal_px_per_unit) {
    if (!out_pixels || logical_w <= 0 || logical_h <= 0) {
        return;
    }

    const float fov_per_column =
        2.0f * std::atan(0.5f * static_cast<float>(logical_w) / focal_px_per_unit) /
        static_cast<float>(logical_w);

    for (int x = 0; x < logical_w; ++x) {
        RayConfig cfg;
        cfg.origin_xy = Vec2{view.origin.x, view.origin.y};
        cfg.yaw = view.yaw +
                  (static_cast<float>(x) - static_cast<float>(logical_w) * 0.5f) *
                      fov_per_column;
        const RayResult res = CastColumnRay(cfg, cells, grid_w, grid_h);

        float min_top = static_cast<float>(logical_h);
        float max_bottom = -1.0f;

        // Walls.
        for (uint32_t i = 0; i < res.segment_count; ++i) {
            const WallProjection p =
                ProjectWall(res.segments[i], view.origin.z, view.pitch,
                            focal_px_per_unit, logical_h);
            if (!p.visible) {
                continue;
            }
            min_top = std::min(min_top, p.screen_top_y);
            max_bottom = std::max(max_bottom, p.screen_bottom_y);
            const int top = std::max(0, static_cast<int>(std::ceil(p.screen_top_y)));
            const int bottom = std::min(logical_h - 1,
                                        static_cast<int>(std::floor(p.screen_bottom_y)));
            for (int y = top; y <= bottom; ++y) {
                const Color c = MaterialColor(p.material, p.distance, y, x, res.segments[i].light);
                out_pixels[static_cast<size_t>(y) * logical_w + x] = c;
            }
        }

        // Ceiling and floor fills.
        for (int y = 0; y < logical_h; ++y) {
            const float fy = static_cast<float>(y);
            const float dist = res.full_occlusion_distance > 0.01f
                                   ? res.full_occlusion_distance
                                   : 4.0f;
            if (fy < min_top) {
                out_pixels[static_cast<size_t>(y) * logical_w + x] =
                    CeilingColor(y, logical_h, dist);
            } else if (fy > max_bottom) {
                out_pixels[static_cast<size_t>(y) * logical_w + x] =
                    FloorColor(y, logical_h, dist);
            }
        }
    }
}

void DrawWeaponViewmodel(Color* logical_pixels,
                         int logical_w, int logical_h,
                         int state, float recoil_offset) {
    if (!logical_pixels || logical_w <= 0 || logical_h <= 0) {
        return;
    }
    const int base_y = logical_h - 14;
    const int base_x = logical_w - 42;
    const int recoil = static_cast<int>(recoil_offset * 5.0f);

    auto set = [&](int x, int y, Color c) {
        if (x >= 0 && x < logical_w && y >= 0 && y < logical_h) {
            logical_pixels[static_cast<size_t>(y) * logical_w + x] = c;
        }
    };

    // Gun body: dark gunmetal with a lighter top edge.
    const Color steel{72, 76, 84};
    const Color steel_hi{94, 98, 108};
    const Color grip{58, 48, 44};
    const Color muzzle{210, 150, 60};

    for (int y = 0; y < 12; ++y) {
        for (int x = 0; x < 30; ++x) {
            int px = base_x + x;
            int py = base_y + y - recoil;
            if (y < 3) {
                set(px, py, steel_hi);
            } else {
                set(px, py, steel);
            }
        }
    }
    // Barrel / muzzle.
    for (int y = 3; y < 6; ++y) {
        for (int x = 30; x < 38; ++x) {
            set(base_x + x, base_y + y - recoil, steel_hi);
        }
    }
    if (state == 1) {
        for (int i = 0; i < 6; ++i) {
            set(base_x + 38 + (i % 2), base_y + 3 + (i / 2) - recoil, muzzle);
        }
    }
    // Grip.
    for (int y = 12; y < 20; ++y) {
        for (int x = 4; x < 14; ++x) {
            set(base_x + x, base_y + y - recoil, grip);
        }
    }
}

void ComposeHalfBlockFrame(const Color* logical_pixels,
                           int logical_w, int logical_h,
                           CharCell* out_cells, int cell_w, int cell_h) {
    if (!logical_pixels || !out_cells ||
        cell_w <= 0 || cell_h <= 0 || logical_h < cell_h * 2) {
        return;
    }
    for (int cy = 0; cy < cell_h; ++cy) {
        const int upper_row = cy * 2;
        const int lower_row = cy * 2 + 1;
        for (int cx = 0; cx < cell_w; ++cx) {
            const Color upper =
                logical_pixels[static_cast<size_t>(upper_row) * logical_w + cx];
            const Color lower =
                logical_pixels[static_cast<size_t>(lower_row) * logical_w + cx];
            CharCell& cell = out_cells[static_cast<size_t>(cy) * cell_w + cx];
            cell.code_point = U'\u2580';
            cell.fg_r = upper.r;
            cell.fg_g = upper.g;
            cell.fg_b = upper.b;
            cell.bg_r = lower.r;
            cell.bg_g = lower.g;
            cell.bg_b = lower.b;
            cell.flags = 0;
        }
    }
}

} // namespace writeover
