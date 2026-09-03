#pragma once
// World grid + read-only query interfaces. IWorldQuery is the ONLY way
// render/AI/narrative/player read world geometry; it never mutates.

#include "writeover/common/player_types.h"
#include "writeover/common/types.h"

#include <cstdint>
#include <vector>

namespace writeover {

class IWorldQuery {
public:
    virtual ~IWorldQuery() = default;

    virtual bool IsSolidAt(int32_t col, int32_t row) const = 0;
    virtual GridCell GetCell(int32_t col, int32_t row) const = 0;
    virtual int32_t Width() const = 0;
    virtual int32_t Height() const = 0;

    virtual float FloorHeightAt(float x, float y) const = 0;
    virtual float CeilingHeightAt(float x, float y) const = 0;

    // True iff the box is penetrated by a solid cell or the vertical
    // interval is violated at any covered cell (checks ALL covered cells).
    virtual bool AabbBlocked(const AABB& box) const = 0;

    // True iff every sample along the segment keeps the given eye height
    // inside the walkable interval.
    virtual bool LineOfSight(const Vec3& a, const Vec3& b, float eye_z) const = 0;
};

class Grid {
public:
    Grid() = default;
    Grid(int32_t width, int32_t height, const GridCell& fill = GridCell{});

    bool InBounds(int32_t col, int32_t row) const;
    GridCell GetCell(int32_t col, int32_t row) const;
    void SetCell(int32_t col, int32_t row, const GridCell& cell);

    int32_t Width() const { return width_; }
    int32_t Height() const { return height_; }
    std::vector<GridCell>& Data() { return cells_; }
    const std::vector<GridCell>& Data() const { return cells_; }

private:
    int32_t width_ = 0;
    int32_t height_ = 0;
    std::vector<GridCell> cells_;
};

// Concrete adapter satisfying IWorldQuery over a Grid*
class GridWorldQuery : public IWorldQuery {
public:
    explicit GridWorldQuery(const Grid* grid) : grid_(grid) {}

    bool IsSolidAt(int32_t col, int32_t row) const override;
    GridCell GetCell(int32_t col, int32_t row) const override;
    int32_t Width() const override;
    int32_t Height() const override;
    float FloorHeightAt(float x, float y) const override;
    float CeilingHeightAt(float x, float y) const override;
    bool AabbBlocked(const AABB& box) const override;
    bool LineOfSight(const Vec3& a, const Vec3& b, float eye_z) const override;

private:
    const Grid* grid_;
};

} // namespace writeover