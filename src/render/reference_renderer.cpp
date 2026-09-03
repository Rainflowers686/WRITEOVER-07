#include "writeover/render/reference_renderer.h"

#include "writeover/render/terminal_backend.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace writeover {

namespace {

constexpr float kMaxPitchRad = 30.0f * 3.14159265f / 180.0f;

// Per-material base color (shared by renderer + reference palette).
Color MaterialColor(uint8_t material) {
    switch (material) {
    case 1: return {160, 160, 170};   // metal
    case 2: return {110, 190, 200};   // glass
    case 3: return {120, 90, 60};     // dirt
    case 4: return {130, 130, 135};   // concrete
    case 5: return {110, 80, 50};     // wood
    case 6: return {90, 110, 120};    // grate
    case 7: return {200, 160, 40};    // hazard
    default: return {170, 140, 110};  // default wall
    }
}

uint8_t DepthShade(float distance, uint8_t channel) {
    const float shade = std::max(0.35f, 1.0f - distance * 0.045f);
    return static_cast<uint8_t>(std::clamp(
        static_cast<float>(channel) * shade, 0.0f, 255.0f));
}

Color ShadedMaterialColor(uint8_t material, float distance) {
    const Color base = MaterialColor(material);
    return {DepthShade(distance, base.r),
            DepthShade(distance, base.g),
            DepthShade(distance, base.b)};
}

} // namespace

int RenderReferenceFrame(const GridCell* cells, int grid_w, int grid_h,
                         const RenderView& view,
                         const ReferenceMarker* marker,
                         CharCell* out, int screen_w, int screen_h,
                         float focal_px_per_unit) {
    int wall_rows = 0;
    const float eye_z = view.origin.z;
    const float fov_per_column = 2.0f * std::atan(0.5f * screen_w / focal_px_per_unit)
                                     / static_cast<float>(screen_w);

    // Clear: ceiling (dark) top half / floor (darker) bottom half is done per
    // column after walls; start with a neutral clear.
    for (int y = 0; y < screen_h; ++y) {
        for (int x = 0; x < screen_w; ++x) {
            CharCell& cell = out[static_cast<size_t>(y) * screen_w + x];
            cell = CharCell{};
            cell.bg_r = 8;
            cell.bg_g = 10;
            cell.bg_b = 24;
        }
    }

    for (int x = 0; x < screen_w; ++x) {
        RayConfig cfg;
        cfg.origin_xy = Vec2{view.origin.x, view.origin.y};
        cfg.yaw = view.yaw +
                  (static_cast<float>(x) - static_cast<float>(screen_w) * 0.5f) *
                      fov_per_column;
        const RayResult res = CastColumnRay(cfg, cells, grid_w, grid_h);

        // 1) Wall spans: rasterize every segment.
        for (uint32_t i = 0; i < res.segment_count; ++i) {
            const WallProjection p =
                ProjectWall(res.segments[i], eye_z, view.pitch,
                            focal_px_per_unit, screen_h);
            if (!p.visible) {
                continue;
            }
            const int top = std::max(0, static_cast<int>(std::ceil(p.screen_top_y)));
            const int bottom = std::min(screen_h - 1,
                                        static_cast<int>(std::floor(p.screen_bottom_y)));
            if (top > bottom) {
                continue;
            }
            const Color shade = ShadedMaterialColor(p.material, p.distance);
            for (int y = top; y <= bottom; ++y) {
                CharCell& cell = out[static_cast<size_t>(y) * screen_w + x];
                cell.code_point = U'\u2588';  // full block
                cell.fg_r = shade.r;
                cell.fg_g = shade.g;
                cell.fg_b = shade.b;
                cell.bg_r = shade.r;
                cell.bg_g = shade.g;
                cell.bg_b = shade.b;
                ++wall_rows;
            }
        }

        // 2) Floor/ceiling fills around the spans.
        // The first segment's bottom edge separates floor from ceiling in
        // the simple reference: everything below the lowest wall bottom is
        // floor; everything above the highest wall top is ceiling.
        float min_top_y = static_cast<float>(screen_h);   // smallest row
        float max_bottom_y = -1.0f;                       // largest row
        for (uint32_t i = 0; i < res.segment_count; ++i) {
            const WallProjection p =
                ProjectWall(res.segments[i], eye_z, view.pitch,
                            focal_px_per_unit, screen_h);
            if (p.visible) {
                min_top_y = std::min(min_top_y, p.screen_top_y);
                max_bottom_y = std::max(max_bottom_y, p.screen_bottom_y);
            }
        }
        const float floor_light = std::max(
            0.20f, 0.40f - res.full_occlusion_distance * 0.01f);
        const float ceil_light = std::max(
            0.12f, 0.26f - res.full_occlusion_distance * 0.01f);
        for (int y = 0; y < screen_h; ++y) {
            CharCell& cell = out[static_cast<size_t>(y) * screen_w + x];
            const float fy = static_cast<float>(y);
            if (fy > max_bottom_y) {
                cell.bg_r = static_cast<uint8_t>(90 * floor_light);
                cell.bg_g = static_cast<uint8_t>(70 * floor_light);
                cell.bg_b = static_cast<uint8_t>(40 * floor_light);
                if (cell.code_point == U' ') {
                    cell.code_point = U'\u2591';  // light shade
                }
            } else if (fy < min_top_y) {
                cell.bg_r = static_cast<uint8_t>(30 * ceil_light);
                cell.bg_g = static_cast<uint8_t>(36 * ceil_light);
                cell.bg_b = static_cast<uint8_t>(70 * ceil_light);
            }
        }
    }

    // 3) Marker sprite: project to screen and depth-test against the wall
    // span at that column (occlusion).
    if (marker != nullptr) {
        const float dx = marker->position.x - view.origin.x;
        const float dy = marker->position.y - view.origin.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance > 0.001f) {
            const float to_marker_yaw = std::atan2(dy, dx);
            float rel = to_marker_yaw - view.yaw;
            while (rel > 3.14159265f) rel -= 2.0f * 3.14159265f;
            while (rel < -3.14159265f) rel += 2.0f * 3.14159265f;
            const float col_float =
                static_cast<float>(screen_w) * 0.5f + rel / fov_per_column;
            const int col = static_cast<int>(std::lround(col_float));
            if (col >= 0 && col < screen_w) {
                // Height in screen space.
                const float dz = marker->position.z - eye_z;
                const float scale = focal_px_per_unit / std::max(distance, 0.01f);
                const float row_center =
                    static_cast<float>(screen_h) * 0.5f +
                    std::tan(view.pitch) * focal_px_per_unit;
                const int row = static_cast<int>(std::lround(
                    row_center - dz * scale));
                if (row >= 0 && row < screen_h) {
                    CharCell& cell = out[static_cast<size_t>(row) * screen_w + col];
                    // Depth test (Issue G): only a wall segment that is BOTH
                    // closer than the marker AND vertically overlapping the
                    // marker's screen row occludes it. A low wall below the
                    // marker (marker glyph above the wall top) stays visible.
                    bool occluded = false;
                    RayConfig cfg;
                    cfg.origin_xy = Vec2{view.origin.x, view.origin.y};
                    cfg.yaw = view.yaw +
                              (static_cast<float>(col) -
                               static_cast<float>(screen_w) * 0.5f) *
                                  fov_per_column;
                    const RayResult col_res = CastColumnRay(cfg, cells, grid_w, grid_h);
                    for (uint32_t i = 0; i < col_res.segment_count; ++i) {
                        const OccludingSegment& seg = col_res.segments[i];
                        if (seg.distance >= distance - 0.05f) {
                            continue;  // not closer than the marker
                        }
                        const WallProjection p =
                            ProjectWall(seg, eye_z, view.pitch,
                                        focal_px_per_unit, screen_h);
                        // Vertical overlap with the marker glyph row.
                        const float row_f = static_cast<float>(row);
                        if (row_f >= p.screen_top_y - 0.5f &&
                            row_f <= p.screen_bottom_y + 0.5f) {
                            occluded = true;
                            break;
                        }
                    }
                    if (!occluded) {
                        cell.code_point = marker->glyph;
                        cell.fg_r = 255;
                        cell.fg_g = 220;
                        cell.fg_b = 80;
                    }
                }
            }
        }
    }

    return wall_rows;
}

std::string RenderReferenceText(const GridCell* cells, int grid_w, int grid_h,
                                const RenderView& view, int screen_w, int screen_h) {
    std::vector<CharCell> frame(static_cast<size_t>(screen_w) * screen_h);
    const float focal = 0.5f * static_cast<float>(screen_h) /
                        std::tan(60.0f * 3.14159265f / 360.0f);
    RenderReferenceFrame(cells, grid_w, grid_h, view, nullptr,
                         frame.data(), screen_w, screen_h, focal);
    std::string out;
    out.reserve(static_cast<size_t>(screen_w) * (screen_h + 1));
    for (int y = 0; y < screen_h; ++y) {
        for (int x = 0; x < screen_w; ++x) {
            out += CharCellToUtf8(frame[static_cast<size_t>(y) * screen_w + x].code_point);
        }
        out += '\n';
    }
    return out;
}

} // namespace writeover