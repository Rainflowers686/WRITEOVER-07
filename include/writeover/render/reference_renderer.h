#pragma once
// HK-2 reference renderer: visible character-3D output from the Height-Span
// raycaster. This is the REFERENCE rasterizer semantics — Luna may optimize
// but must not change the projection/raster math contract.
//
// Contract:
//   * XY DDA produces per-column segments (CastColumnRay).
//   * ProjectWall maps world-Z to screen rows; pitch only rotates projection.
//   * Each wall span is rasterized as full-block glyphs with depth shading.
//   * Floor/ceiling are background fills below/above the spans.
//   * A marker sprite is projected and depth-tested against wall spans.
//
// This renderer is intentionally simple, deterministic, and unit-testable:
// the same inputs produce byte-identical CharCell output.

#include "writeover/common/types.h"
#include "writeover/render/raycaster.h"
#include "writeover/render/terminal_backend.h"

#include <cstdint>
#include <string>

namespace writeover {

struct RenderView {
    Vec3 origin;           // camera position (x, y, eye_z)
    float yaw = 0.0f;      // radians
    float pitch = 0.0f;    // radians, clamped to +-30 deg
};

struct ReferenceMarker {
    Vec3 position;         // world position of the marker
    char32_t glyph = U'\u25CF';  // ●
};

// Rasterizes the grid into a CharCell buffer (deterministic).
// Returns the number of wall-span rows drawn (for tests/coverage).
int RenderReferenceFrame(const GridCell* cells, int grid_w, int grid_h,
                         const RenderView& view,
                         const ReferenceMarker* marker,  // may be null
                         CharCell* out, int screen_w, int screen_h,
                         float focal_px_per_unit);

// Renders a synthetic scene to a text/ANSI representation for smoke output
// and screenshots. Resolution presets: 160x45 / 192x54 / 240x67.
std::string RenderReferenceText(const GridCell* cells, int grid_w, int grid_h,
                                const RenderView& view, int screen_w, int screen_h);

} // namespace writeover