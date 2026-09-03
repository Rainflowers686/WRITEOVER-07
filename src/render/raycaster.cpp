#include "writeover/render/raycaster.h"

#include "writeover/common/math.h"

#include <cmath>

namespace writeover {

namespace {

struct CellInfo {
    float floor_z = 0.0f;
    float ceiling_z = 4.0f;
    bool solid = false;
    uint8_t material = 0;
    uint8_t light = 255;
};

CellInfo CellAt(const GridCell* grid, int grid_w, int grid_h, int col, int row) {
    if (col < 0 || row < 0 || col >= grid_w || row >= grid_h) {
        CellInfo out;
        out.solid = true;  // out-of-bounds = solid wall
        out.floor_z = -1.0f;
        out.ceiling_z = 5.0f;
        return out;
    }
    const GridCell& c = grid[row * grid_w + col];
    CellInfo out;
    out.floor_z = c.floor_height;
    out.ceiling_z = c.ceiling_height;
    out.solid = c.IsSolid();
    out.material = c.material;
    out.light = c.light;
    return out;
}

void PushSegment(RayResult& result, float bottom, float top, float distance,
                 uint8_t material, uint8_t light, uint8_t flag) {
    if (result.segment_count >= kMaxSegmentsPerRay) {
        result.truncated = true;
        return;
    }
    if (top <= bottom) {
        return;  // degenerate, deterministic skip
    }
    OccludingSegment& seg = result.segments[result.segment_count++];
    seg.bottom_z = bottom;
    seg.top_z = top;
    seg.distance = distance;
    seg.material = material;
    seg.light = light;
    seg.flag = flag;
}

} // namespace

RayResult CastColumnRay(const RayConfig& config,
                        const GridCell* grid, int grid_w, int grid_h) {
    RayResult result;

    const float dx = std::cos(config.yaw);
    const float dy = std::sin(config.yaw);

    int col = static_cast<int>(std::floor(config.origin_xy.x));
    int row = static_cast<int>(std::floor(config.origin_xy.y));
    CellInfo start = CellAt(grid, grid_w, grid_h, col, row);

    const float step_x = dx >= 0.0f ? 1.0f : -1.0f;
    const float step_y = dy >= 0.0f ? 1.0f : -1.0f;
    constexpr float kInf = 1e30f;
    const float t_delta_x = std::fabs(dx) > 1e-9f ? std::fabs(1.0f / dx) : kInf;
    const float t_delta_y = std::fabs(dy) > 1e-9f ? std::fabs(1.0f / dy) : kInf;

    float t_max_x = t_delta_x >= kInf
                        ? kInf
                        : (step_x > 0.0f ? (static_cast<float>(col) + 1.0f - config.origin_xy.x)
                                         : (config.origin_xy.x - static_cast<float>(col))) * t_delta_x;
    float t_max_y = t_delta_y >= kInf
                        ? kInf
                        : (step_y > 0.0f ? (static_cast<float>(row) + 1.0f - config.origin_xy.y)
                                         : (config.origin_xy.y - static_cast<float>(row))) * t_delta_y;

    result.final_floor_z = start.floor_z;
    result.final_ceiling_z = start.ceiling_z;

    while (true) {
        if (t_max_x >= config.max_distance && t_max_y >= config.max_distance) {
            break;
        }

        int next_col = col;
        int next_row = row;
        float t_boundary = 0.0f;
        if (t_max_x <= t_max_y) {
            t_boundary = t_max_x;
            t_max_x += t_delta_x;
            next_col = col + (step_x > 0.0f ? 1 : -1);
        } else {
            t_boundary = t_max_y;
            t_max_y += t_delta_y;
            next_row = row + (step_y > 0.0f ? 1 : -1);
        }

        if (t_boundary > config.max_distance) {
            break;
        }

        const CellInfo near_cell = CellAt(grid, grid_w, grid_h, col, row);
        const CellInfo far_cell = CellAt(grid, grid_w, grid_h, next_col, next_row);

        const float opening_bottom = std::max(near_cell.floor_z, far_cell.floor_z);
        const float opening_top = std::min(near_cell.ceiling_z, far_cell.ceiling_z);
        const float distance = t_boundary;

        if (opening_top <= opening_bottom || far_cell.solid || near_cell.solid) {
            const float b = std::min(near_cell.floor_z, far_cell.floor_z);
            const float t = std::max(near_cell.ceiling_z, far_cell.ceiling_z);
            PushSegment(result, b, t, distance, far_cell.solid ? far_cell.material : near_cell.material,
                        near_cell.light, SegFullWall);
            result.hit_full_occlusion = true;
            result.full_occlusion_distance = distance;
            result.final_floor_z = near_cell.floor_z;
            result.final_ceiling_z = near_cell.ceiling_z;
            break;
        }

        if (far_cell.floor_z > near_cell.floor_z) {
            PushSegment(result, near_cell.floor_z, far_cell.floor_z, distance,
                        far_cell.material, far_cell.light, SegFloorRise);
        }
        if (far_cell.ceiling_z < near_cell.ceiling_z) {
            PushSegment(result, far_cell.ceiling_z, near_cell.ceiling_z, distance,
                        far_cell.material, far_cell.light, SegCeilingDrop);
        }

        col = next_col;
        row = next_row;
        result.final_floor_z = far_cell.floor_z;
        result.final_ceiling_z = far_cell.ceiling_z;
        if (far_cell.solid) {
            break;  // loop re-checks from solid cell next iteration
        }
    }

    return result;
}

WallProjection ProjectWall(const OccludingSegment& seg,
                           float eye_z, float pitch_rad,
                           float focal_px_per_unit, int screen_h) {
    WallProjection out;
    out.distance = seg.distance;
    out.material = seg.material;
    out.light = seg.light;
    out.flag = seg.flag;

    const float tan_pitch = std::tan(pitch_rad);
    const float center_row = static_cast<float>(screen_h) * 0.5f +
                             tan_pitch * focal_px_per_unit;
    const float scale = focal_px_per_unit / std::max(seg.distance, 0.001f);
    out.screen_top_y = center_row - (seg.top_z - eye_z) * scale;
    out.screen_bottom_y = center_row - (seg.bottom_z - eye_z) * scale;
    out.visible = out.screen_bottom_y >= 0.0f &&
                  out.screen_top_y <= static_cast<float>(screen_h);
    return out;
}

} // namespace writeover