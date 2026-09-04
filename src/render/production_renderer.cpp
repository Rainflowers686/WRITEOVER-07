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

Color ScaleColor(const Color& color, float scale) {
    return {Clamp8(static_cast<float>(color.r) * scale),
            Clamp8(static_cast<float>(color.g) * scale),
            Clamp8(static_cast<float>(color.b) * scale)};
}

Color SurfaceColor(uint8_t material, uint8_t light, float distance,
                   int cell_x, int cell_y, bool ceiling) {
    const Color base = MaterialBase(material);
    const float light_level = 0.28f + 0.72f * (static_cast<float>(light) / 255.0f);
    const float fog = ceiling
        ? (0.42f + 0.58f * Saturate(1.0f - distance * 0.025f))
        : (0.48f + 0.52f * Saturate(1.0f - distance * 0.022f));
    const float material_scale = ceiling ? 0.34f : 0.46f;
    Color out = ScaleColor(base, material_scale * light_level * fog);

    // A restrained industrial tile rhythm makes the ground readable without
    // turning the large area into a bright checkerboard.
    const bool seam_x = (static_cast<int>(std::floor(distance * 0.55f)) + cell_x) % 4 == 0;
    const bool seam_y = (static_cast<int>(std::floor(distance * 0.35f)) + cell_y) % 3 == 0;
    if (seam_x || seam_y) out = ScaleColor(out, ceiling ? 0.72f : 0.78f);
    if (material == 7 && !ceiling) {
        out.r = Clamp8(static_cast<float>(out.r) + 18.0f);
        out.g = Clamp8(static_cast<float>(out.g) + 10.0f);
    }
    return out;
}

GridCell SampleCell(const GridCell* cells, int grid_w, int grid_h,
                    float x, float y) {
    const int col = static_cast<int>(std::floor(x));
    const int row = static_cast<int>(std::floor(y));
    if (cells == nullptr || col < 0 || row < 0 || col >= grid_w || row >= grid_h) {
        GridCell edge;
        edge.material = 1;
        edge.light = 70;
        return edge;
    }
    return cells[row * grid_w + col];
}

} // namespace

void RenderProductionFrame(const GridCell* cells, int grid_w, int grid_h,
                           const ProductionView& view,
                           Color* out_pixels, int logical_w, int logical_h,
                           float focal_px_per_unit) {
    if (!out_pixels || logical_w <= 0 || logical_h <= 0 ||
        grid_w <= 0 || grid_h <= 0 || focal_px_per_unit <= 0.01f) {
        return;
    }

    const float fov_per_column =
        2.0f * std::atan(0.5f * static_cast<float>(logical_w) / focal_px_per_unit) /
        static_cast<float>(logical_w);

    const float screen_center = static_cast<float>(logical_h) * 0.5f;
    const float horizon = screen_center + std::tan(view.pitch) * focal_px_per_unit;
    for (int x = 0; x < logical_w; ++x) {
        RayConfig cfg;
        cfg.origin_xy = Vec2{view.origin.x, view.origin.y};
        cfg.yaw = view.yaw +
                  (static_cast<float>(x) - static_cast<float>(logical_w) * 0.5f) *
                      fov_per_column;
        const RayResult res = CastColumnRay(cfg, cells, grid_w, grid_h);

        const float ray_dx = std::cos(cfg.yaw);
        const float ray_dy = std::sin(cfg.yaw);
        const float far_distance = 50.0f;

        // Explicit floor/ceiling casting. This is deliberately a small,
        // deterministic raster policy: the raycaster still owns Height-Span
        // wall semantics, while planes sample the final world cell.
        for (int y = 0; y < logical_h; ++y) {
            const float delta = static_cast<float>(y) - horizon;
            const bool ceiling = delta < 0.0f;
            const float plane_z = ceiling ? res.final_ceiling_z : res.final_floor_z;
            const float height_delta = std::fabs(view.origin.z - plane_z);
            const float abs_delta = std::fabs(delta);
            const float distance = abs_delta > 0.5f
                ? std::min(far_distance, std::max(0.25f,
                    height_delta * focal_px_per_unit / abs_delta))
                : far_distance;
            const GridCell sample = SampleCell(cells, grid_w, grid_h,
                view.origin.x + ray_dx * distance,
                view.origin.y + ray_dy * distance);
            out_pixels[static_cast<size_t>(y) * logical_w + x] =
                SurfaceColor(sample.material, sample.light, distance,
                             static_cast<int>(std::floor(view.origin.x + ray_dx * distance)),
                             static_cast<int>(std::floor(view.origin.y + ray_dy * distance)),
                             ceiling);
        }

        // Walls.
        // Segments are recorded near-to-far. Draw in reverse so a nearer
        // face is the final visible surface when spans overlap.
        for (uint32_t i = res.segment_count; i > 0; --i) {
            const WallProjection p =
                ProjectWall(res.segments[i - 1], view.origin.z, view.pitch,
                            focal_px_per_unit, logical_h);
            if (!p.visible) {
                continue;
            }
            const int top = std::max(0, static_cast<int>(std::ceil(p.screen_top_y)));
            const int bottom = std::min(logical_h - 1,
                                        static_cast<int>(std::floor(p.screen_bottom_y)));
            for (int y = top; y <= bottom; ++y) {
                Color c = MaterialColor(p.material, p.distance, y, x,
                                        res.segments[i - 1].light);
                if (res.segments[i - 1].flag == SegFloorRise ||
                    res.segments[i - 1].flag == SegFloorDrop) {
                    c = ScaleColor(c, 0.82f);
                } else if (res.segments[i - 1].flag == SegCeilingRise ||
                           res.segments[i - 1].flag == SegCeilingDrop) {
                    c = ScaleColor(c, 0.68f);
                }
                out_pixels[static_cast<size_t>(y) * logical_w + x] = c;
            }
        }
    }
}

void DrawProductionSprite(const Vec3& camera, float yaw, float pitch,
                          const Vec3& world_pos, float sprite_height,
                          ProductionSpriteKind kind, const Color& tint,
                          const GridCell* cells, int grid_w, int grid_h,
                          Color* logical_pixels, int logical_w, int logical_h,
                          float focal_px_per_unit) {
    if (!logical_pixels || logical_w <= 0 || logical_h <= 0 ||
        focal_px_per_unit <= 0.01f || sprite_height <= 0.01f) return;
    const float dx = world_pos.x - camera.x;
    const float dy = world_pos.y - camera.y;
    const float euclidean_dist = std::sqrt(dx * dx + dy * dy);
    if (euclidean_dist < 0.05f) return;
    const float to_yaw = std::atan2(dy, dx);
    float rel = to_yaw - yaw;
    while (rel > 3.14159265f) rel -= 2.0f * 3.14159265f;
    while (rel < -3.14159265f) rel += 2.0f * 3.14159265f;
    const float fov_per_column =
        2.0f * std::atan(0.5f * static_cast<float>(logical_w) / focal_px_per_unit) /
        static_cast<float>(logical_w);
    const float col_f = static_cast<float>(logical_w) * 0.5f + rel / fov_per_column;
    if (col_f < -100.0f || col_f > static_cast<float>(logical_w) + 100.0f) return;
    const float depth = euclidean_dist * std::cos(rel);
    if (depth <= 0.05f) return;

    // Vertical projection.
    const float scale = focal_px_per_unit / depth;
    const float row_center =
        static_cast<float>(logical_h) * 0.5f + std::tan(pitch) * focal_px_per_unit;
    const float ground_row = row_center - (world_pos.z - camera.z) * scale;
    const float top_row = ground_row - sprite_height * scale;
    const float bottom_row = ground_row;
    if (bottom_row < 0.0f || top_row >= static_cast<float>(logical_h)) return;
    const int half_width = std::max(2, static_cast<int>(sprite_height * scale * 0.26f));
    const int top = std::max(0, static_cast<int>(std::floor(top_row)));
    const int bottom = std::min(logical_h - 1, static_cast<int>(std::ceil(bottom_row)));
    auto set = [&](int x, int y, Color c) {
        if (x >= 0 && x < logical_w && y >= 0 && y < logical_h) {
            logical_pixels[static_cast<size_t>(y) * logical_w + x] = c;
        }
    };

    auto sprite_pixel = [&](float u, float v) -> Color {
        const Color shadow = ScaleColor(tint, 0.42f);
        const Color mid = ScaleColor(tint, 0.78f);
        const Color hi = ScaleColor(tint, 1.18f);
        const Color glow{80, 210, 198};
        const Color amber{224, 156, 54};
        const float cx = std::fabs(u - 0.5f);
        if (kind == ProductionSpriteKind::Npc) {
            if (v < 0.22f && cx < 0.19f) return hi;
            if (v >= 0.22f && v < 0.68f && cx < (0.30f - v * 0.08f)) return mid;
            if (v >= 0.68f && v < 0.96f && (cx < 0.13f || (cx > 0.18f && cx < 0.28f))) return shadow;
            return Color{0, 0, 0};
        }
        if (kind == ProductionSpriteKind::Terminal) {
            if (v > 0.10f && v < 0.88f && cx < 0.42f) {
                if (v > 0.22f && v < 0.53f && cx < 0.31f) return glow;
                return v < 0.18f ? hi : mid;
            }
            return Color{0, 0, 0};
        }
        if (kind == ProductionSpriteKind::Medical) {
            if (v > 0.12f && v < 0.88f && cx < 0.38f) return mid;
            if (cx < 0.11f && v > 0.22f && v < 0.78f) return hi;
            if (v > 0.42f && v < 0.58f && cx < 0.32f) return hi;
            return Color{0, 0, 0};
        }
        if (kind == ProductionSpriteKind::Crate) {
            if (v > 0.30f && v < 0.92f && cx < 0.42f) {
                return ((u > 0.45f && u < 0.55f) || (v > 0.59f && v < 0.65f)) ? hi : mid;
            }
            return Color{0, 0, 0};
        }
        if (kind == ProductionSpriteKind::Door || kind == ProductionSpriteKind::Elevator) {
            if (v > 0.03f && v < 0.98f && cx < 0.40f) {
                if (kind == ProductionSpriteKind::Elevator && v > 0.32f && v < 0.39f && cx < 0.12f) return glow;
                return (cx < 0.04f || cx > 0.36f) ? shadow : mid;
            }
            return Color{0, 0, 0};
        }
        if (kind == ProductionSpriteKind::Camera) {
            if (v > 0.28f && v < 0.68f && cx < 0.36f) return mid;
            if (cx < 0.14f && v > 0.40f && v < 0.58f) return amber;
            return Color{0, 0, 0};
        }
        if (kind == ProductionSpriteKind::Sign) {
            if (v > 0.18f && v < 0.58f && cx < 0.46f) return hi;
            if (cx < 0.04f && v > 0.55f) return shadow;
            return Color{0, 0, 0};
        }
        // Lamp / unknown: a compact glowing head and stem.
        if (v < 0.20f && cx < 0.28f) return amber;
        if (cx < 0.06f && v > 0.18f) return shadow;
        return Color{0, 0, 0};
    };

    for (int x = static_cast<int>(std::floor(col_f)) - half_width;
         x <= static_cast<int>(std::ceil(col_f)) + half_width; ++x) {
        if (x < 0 || x >= logical_w) continue;
        const float ray_rel = (static_cast<float>(x) + 0.5f -
                               static_cast<float>(logical_w) * 0.5f) * fov_per_column;
        const float ray_depth = euclidean_dist * std::cos(ray_rel);
        if (ray_depth <= 0.05f) continue;
        RayConfig cfg;
        cfg.origin_xy = Vec2{camera.x, camera.y};
        cfg.yaw = yaw + ray_rel;
        const RayResult ray = CastColumnRay(cfg, cells, grid_w, grid_h);
        bool occluded = false;
        for (uint32_t i = 0; i < ray.segment_count; ++i) {
            const OccludingSegment& seg = ray.segments[i];
            if (seg.distance >= ray_depth - 0.05f) continue;
            const WallProjection wall = ProjectWall(seg, camera.z, pitch,
                                                    focal_px_per_unit, logical_h);
            if (wall.visible && bottom_row >= wall.screen_top_y - 0.5f &&
                top_row <= wall.screen_bottom_y + 0.5f) {
                occluded = true;
                break;
            }
        }
        if (occluded) continue;
        const float u = (static_cast<float>(x) - col_f + half_width) /
                        static_cast<float>(std::max(1, half_width * 2));
        if (u < 0.0f || u > 1.0f) continue;
        for (int y = top; y <= bottom; ++y) {
            const float v = (static_cast<float>(y) - top_row) /
                            std::max(1.0f, bottom_row - top_row);
            const Color pixel = sprite_pixel(u, v);
            if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0) set(x, y, pixel);
        }
    }
}

void DrawWeaponViewmodel(Color* logical_pixels,
                         int logical_w, int logical_h,
                         int state, float recoil_offset) {
    if (!logical_pixels || logical_w <= 0 || logical_h <= 0) {
        return;
    }
    constexpr int kRightSafe = 24;
    constexpr int kBottomSafe = 12;
    // Keep the authored silhouette within the Visual Bible's 22% x 30%
    // maximum occupation at the 240x134 ULTRA framebuffer.
    constexpr int kArtworkWidth = 64;
    constexpr int kArtworkHeight = 48;
    constexpr int kWeaponWidth = 52;
    constexpr int kWeaponHeight = 40;
    const int base_x = logical_w - kRightSafe - kWeaponWidth;
    const int base_y = logical_h - kBottomSafe - kWeaponHeight;
    const int recoil = static_cast<int>(std::clamp(recoil_offset, 0.0f, 1.0f) * 7.0f);

    auto set = [&](int x, int y, Color c) {
        if (x >= 0 && x < logical_w && y >= 0 && y < logical_h) {
            logical_pixels[static_cast<size_t>(y) * logical_w + x] = c;
        }
    };

    // Pixel-art sidearm silhouette. The safe-zone anchor is part of the
    // contract: its rightmost pixel stops 24px before the logical edge and
    // its lowest pixel stops 12px above the logical edge.
    const Color shadow{18, 22, 28};
    const Color steel{58, 68, 80};
    const Color steel_hi{118, 132, 144};
    const Color grip{48, 38, 40};
    const Color grip_hi{92, 62, 54};
    const Color muzzle{244, 170, 48};
    const int kick = -recoil;
    auto rect = [&](int x0, int y0, int x1, int y1, Color color) {
        const int scaled_x0 = static_cast<int>(std::lround(
            static_cast<float>(x0) * kWeaponWidth / kArtworkWidth));
        const int scaled_y0 = static_cast<int>(std::lround(
            static_cast<float>(y0) * kWeaponHeight / kArtworkHeight));
        const int scaled_x1 = static_cast<int>(std::lround(
            static_cast<float>(x1) * kWeaponWidth / kArtworkWidth));
        const int scaled_y1 = static_cast<int>(std::lround(
            static_cast<float>(y1) * kWeaponHeight / kArtworkHeight));
        for (int y = scaled_y0; y <= scaled_y1; ++y) {
            for (int x = scaled_x0; x <= scaled_x1; ++x) {
                set(base_x + x, base_y + y + kick, color);
            }
        }
    };
    rect(4, 9, 48, 28, shadow);
    rect(8, 7, 42, 20, steel);
    rect(10, 7, 39, 9, steel_hi);
    rect(39, 11, 58, 16, steel);
    rect(56, 12, 63, 15, steel_hi);
    rect(12, 20, 28, 25, steel_hi);
    rect(15, 27, 27, 43, grip);
    rect(17, 30, 25, 39, grip_hi);
    rect(15, 42, 29, 46, shadow);
    rect(15, 47, 29, 47, shadow);
    rect(20, 21, 28, 25, shadow);
    // A short sight and ejection slot keep the weapon readable at 1:1.
    rect(19, 4, 27, 7, shadow);
    rect(29, 11, 35, 13, shadow);
    if (state == 1) {
        rect(62, 7, 63, 20, muzzle);
        rect(60, 9, 61, 18, muzzle);
        rect(59, 12, 59, 15, Color{255, 218, 100});
    }
}

void ComposeHalfBlockFrame(const Color* logical_pixels,
                           int logical_w, int logical_h,
                           CharCell* out_cells, int cell_w, int cell_h) {
    if (!logical_pixels || !out_cells || logical_w <= 0 ||
        cell_w <= 0 || cell_h <= 0 || logical_w < cell_w ||
        logical_h < cell_h * 2) {
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
