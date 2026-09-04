#pragma once
// PVS-01 production renderer: Half-block TrueColor Pixel Framebuffer.
//
// Normal world output is a logical pixel buffer composed into terminal cells
// with U+2580 (upper half block). Foreground = upper logical pixel, background
// = lower logical pixel. This renderer is the production path; ReferenceRenderer
// remains only as a test/reference raster.

#include "writeover/common/types.h"
#include "writeover/render/terminal_backend.h"

#include <cstdint>

namespace writeover {

struct ProductionView {
    Vec3 origin;            // world camera position (x, y, eye_z)
    float yaw = 0.0f;       // radians
    float pitch = 0.0f;     // radians
};

enum class ProductionSpriteKind : uint8_t {
    Npc = 0,
    Terminal = 1,
    Lamp = 2,
    Door = 3,
    Medical = 4,
    Crate = 5,
    Elevator = 6,
    Camera = 7,
    Sign = 8,
};

// Rasterizes a Height-Span grid into a logical RGB pixel buffer.
// logical_w == terminal cell width; logical_h == terminal height * 2.
void RenderProductionFrame(const GridCell* cells, int grid_w, int grid_h,
                           const ProductionView& view,
                           Color* out_pixels, int logical_w, int logical_h,
                           float focal_px_per_unit);

// Draws a simple occluded production sprite into the logical framebuffer.
void DrawProductionSprite(const Vec3& camera, float yaw, float pitch,
                          const Vec3& world_pos, float sprite_height,
                          ProductionSpriteKind kind, const Color& tint,
                          const GridCell* cells, int grid_w, int grid_h,
                          Color* logical_pixels, int logical_w, int logical_h,
                          float focal_px_per_unit);

// Draws a simple production weapon viewmodel into the logical framebuffer.
void DrawWeaponViewmodel(Color* logical_pixels,
                         int logical_w, int logical_h,
                         int state, float recoil_offset);

// Composes a logical pixel framebuffer into half-block CharCell terminal frame.
void ComposeHalfBlockFrame(const Color* logical_pixels,
                           int logical_w, int logical_h,
                           CharCell* out_cells, int cell_w, int cell_h);

} // namespace writeover
