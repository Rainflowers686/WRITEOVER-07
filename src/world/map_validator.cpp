#include "writeover/world/map_validator.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <string>

namespace writeover {

std::vector<ValidationIssue> MapValidator::Validate(const Room& room) const {
    std::vector<ValidationIssue> issues;
    const Grid& grid = room.grid;

    if (grid.Width() <= 0 || grid.Height() <= 0) {
        issues.push_back({ValidationIssue::Severity::Error,
                          "room grid is empty", "room"});
        return issues;
    }
    if (grid.Width() * grid.Height() > 64 * 64) {
        issues.push_back({ValidationIssue::Severity::Warning,
                          "room exceeds 64x64; content budget risk", "room"});
    }

    // Spawn point must be inside the grid and fit a standing box.
    if (room.spawn_point.x < 0.0f || room.spawn_point.y < 0.0f ||
        room.spawn_point.x >= static_cast<float>(grid.Width()) ||
        room.spawn_point.y >= static_cast<float>(grid.Height())) {
        issues.push_back({ValidationIssue::Severity::Error,
                          "spawn point outside grid", "spawn"});
    }

    // Per-cell sanity: floor below ceiling, non-negative light.
    int solid_count = 0;
    float lowest_floor = 1e9f;
    float highest_ceiling = -1e9f;
    for (int32_t row = 0; row < grid.Height(); ++row) {
        for (int32_t col = 0; col < grid.Width(); ++col) {
            const GridCell cell = grid.GetCell(col, row);
            std::ostringstream loc;
            loc << "cell(" << col << "," << row << ")";
            if (cell.ceiling_height - cell.floor_height < 0.05f) {
                issues.push_back({ValidationIssue::Severity::Error,
                                  "ceiling not above floor", loc.str()});
            }
            if (cell.light > 255) {
                issues.push_back({ValidationIssue::Severity::Error,
                                  "light out of range", loc.str()});
            }
            if (cell.IsSolid()) {
                ++solid_count;
            }
            lowest_floor = std::min(lowest_floor, cell.floor_height);
            highest_ceiling = std::max(highest_ceiling, cell.ceiling_height);
        }
    }
    if (solid_count == 0) {
        issues.push_back({ValidationIssue::Severity::Warning,
                          "room has no solid cells; possibly incomplete",
                          "room"});
    }

    // Spawn clearance: standing box (1.80m) must fit at spawn.
    {
        AABB stand_box;
        stand_box.min = Vec3{room.spawn_point.x - kPlayerRadius,
                             room.spawn_point.y - kPlayerRadius,
                             room.spawn_point.z};
        stand_box.max = Vec3{room.spawn_point.x + kPlayerRadius,
                             room.spawn_point.y + kPlayerRadius,
                             room.spawn_point.z + kColliderStand};
        GridWorldQuery query(&grid);
        if (query.AabbBlocked(stand_box)) {
            issues.push_back({ValidationIssue::Severity::Error,
                              "standing box blocked at spawn point", "spawn"});
        }
    }

    // Reference connectivity: every NPC/storylet ref must be non-empty.
    for (const auto& ref : room.npc_refs) {
        if (!ref.IsValid()) {
            issues.push_back({ValidationIssue::Severity::Error,
                              "empty npc reference", "npc_refs"});
        }
    }
    for (const auto& ref : room.storylet_refs) {
        if (!ref.IsValid()) {
            issues.push_back({ValidationIssue::Severity::Error,
                              "empty storylet reference", "storylet_refs"});
        }
    }

    return issues;
}

bool MapValidator::HasErrors(const std::vector<ValidationIssue>& issues) const {
    for (const auto& issue : issues) {
        if (issue.severity == ValidationIssue::Severity::Error) {
            return true;
        }
    }
    return false;
}

} // namespace writeover