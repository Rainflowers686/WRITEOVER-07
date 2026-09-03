#include "writeover/world/grid.h"

#include "writeover/common/math.h"

#include <cmath>

namespace writeover {

Grid::Grid(int32_t width, int32_t height, const GridCell& fill)
    : width_(width), height_(height), cells_(static_cast<size_t>(width * height), fill) {}

bool Grid::InBounds(int32_t col, int32_t row) const {
    return col >= 0 && col < width_ && row >= 0 && row < height_;
}

GridCell Grid::GetCell(int32_t col, int32_t row) const {
    if (!InBounds(col, row)) {
        return GridCell{};  // out-of-bounds treated as solid/neutral
    }
    return cells_[static_cast<size_t>(row * width_ + col)];
}

void Grid::SetCell(int32_t col, int32_t row, const GridCell& cell) {
    if (InBounds(col, row)) {
        cells_[static_cast<size_t>(row * width_ + col)] = cell;
    }
}

// ---------------------------------------------------------------------------
// IWorldQuery adapter
// ---------------------------------------------------------------------------
bool GridWorldQuery::IsSolidAt(int32_t col, int32_t row) const {
    return grid_->GetCell(col, row).IsSolid();
}

GridCell GridWorldQuery::GetCell(int32_t col, int32_t row) const {
    return grid_->GetCell(col, row);
}

int32_t GridWorldQuery::Width() const { return grid_->Width(); }
int32_t GridWorldQuery::Height() const { return grid_->Height(); }

float GridWorldQuery::FloorHeightAt(float x, float y) const {
    const int32_t col = static_cast<int32_t>(std::floor(x));
    const int32_t row = static_cast<int32_t>(std::floor(y));
    return grid_->GetCell(col, row).floor_height;
}

float GridWorldQuery::CeilingHeightAt(float x, float y) const {
    const int32_t col = static_cast<int32_t>(std::floor(x));
    const int32_t row = static_cast<int32_t>(std::floor(y));
    return grid_->GetCell(col, row).ceiling_height;
}

bool GridWorldQuery::AabbBlocked(const AABB& box) const {
    const int32_t min_col = static_cast<int32_t>(std::floor(box.min.x));
    const int32_t max_col = static_cast<int32_t>(std::floor(box.max.x));
    const int32_t min_row = static_cast<int32_t>(std::floor(box.min.y));
    const int32_t max_row = static_cast<int32_t>(std::floor(box.max.y));

    for (int32_t row = min_row; row <= max_row; ++row) {
        for (int32_t col = min_col; col <= max_col; ++col) {
            if (!grid_->InBounds(col, row)) {
                return true;  // leaving the map is blocked
            }
            const GridCell cell = grid_->GetCell(col, row);
            if (cell.IsSolid()) {
                return true;
            }
            // Vertical interval must contain the whole box slice.
            if (cell.floor_height > box.min.z + kEpsPosition ||
                cell.ceiling_height < box.max.z - kEpsPosition) {
                return true;
            }
        }
    }
    return false;
}

bool GridWorldQuery::LineOfSight(const Vec3& a, const Vec3& b, float eye_z) const {
    const Vec3 delta = b - a;
    const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    const float step = 0.5f;
    const int steps = static_cast<int>(distance / step);
    for (int i = 0; i <= steps; ++i) {
        const float t = distance > 0.0f ? static_cast<float>(i) * step / distance : 0.0f;
        const float x = a.x + delta.x * t;
        const float y = a.y + delta.y * t;
        const int32_t col = static_cast<int32_t>(std::floor(x));
        const int32_t row = static_cast<int32_t>(std::floor(y));
        if (!grid_->InBounds(col, row)) {
            return false;
        }
        const GridCell cell = grid_->GetCell(col, row);
        if (cell.IsSolid()) {
            return false;
        }
        if (cell.floor_height > eye_z + kEpsPosition ||
            cell.ceiling_height < eye_z - kEpsPosition) {
            return false;
        }
    }
    return true;
}

} // namespace writeover