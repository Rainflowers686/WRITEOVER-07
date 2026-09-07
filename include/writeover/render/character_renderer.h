#pragma once
// Character-art production renderer.
//
// The world path writes semantic CharCell values directly.  It deliberately
// does not allocate a logical RGB framebuffer and does not use half-block
// composition for normal world surfaces.  Height-Span ray geometry remains
// the visibility source; this layer resolves it into authored glyphs, colour,
// and transparent character sprites.

#include "writeover/common/types.h"
#include "writeover/render/raycaster.h"
#include "writeover/render/terminal_backend.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

enum class CharacterSpriteKind : uint8_t {
    SecurityGuard = 0,
    FullHuman = 1,
    MaintenanceWorker = 2,
    Terminal = 3,
    Camera = 4,
    Crate = 5,
    Door = 6,
};

enum class CharacterLod : uint8_t {
    Far = 0,
    Mid = 1,
    Near = 2,
};

enum class CharacterInk : uint8_t {
    Security = 0,
    FullHuman = 1,
    Maintenance = 2,
    Terminal = 3,
    Prop = 4,
    Weapon = 5,
};

enum class PistolFrame : uint8_t {
    IdleA = 0,
    IdleB = 1,
    Fire = 2,
    Reload = 3,
};

struct CharacterArtAsset {
    CharacterSpriteKind sprite_kind = CharacterSpriteKind::SecurityGuard;
    CharacterLod lod = CharacterLod::Far;
    CharacterInk ink = CharacterInk::Prop;
    PistolFrame pistol_frame = PistolFrame::IdleA;
    bool is_pistol = false;
    std::vector<std::u32string> rows;

    int Width() const;
    int Height() const { return static_cast<int>(rows.size()); }
};

// Lightweight bounded authoring bank.  The file format is intentionally
// plain text so an artist can curate glyphs without a new asset engine:
//
//   sprite security_guard near security
//   ...glyph rows; spaces are transparent...
//   end
//   weapon pistol fire weapon
//   ...glyph rows...
//   end
class CharacterArtBank {
public:
    CharacterArtBank();

    // On failure the previous/default bank remains available.  A successful
    // load replaces it and records that production assets came from disk.
    bool Load(const std::string& path);
    bool LoadedFromFile() const { return loaded_from_file_; }

    const CharacterArtAsset* Find(CharacterSpriteKind kind,
                                  CharacterLod lod) const;
    const CharacterArtAsset* FindPistol(PistolFrame frame) const;

private:
    std::vector<CharacterArtAsset> assets_;
    bool loaded_from_file_ = false;
};

struct CharacterView {
    Vec3 origin;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct CharacterRenderOptions {
    bool high_contrast = false;
};

struct CharacterSpriteInstance {
    Vec3 position;
    float height = 1.7f;
    CharacterSpriteKind kind = CharacterSpriteKind::SecurityGuard;
};

// Character-cell aspect convention: a terminal cell is treated as half as
// wide as it is high for horizontal projection.  The vertical focal value is
// supplied by the caller from the frozen vertical FOV.
inline constexpr float kCharacterCellAspect = 0.5f;

CharacterLod SelectCharacterLod(float distance_meters);

void RenderCharacterFrame(const GridCell* cells, int grid_w, int grid_h,
                          const CharacterView& view,
                          CharCell* out_cells, int cell_w, int cell_h,
                          float focal_cells_per_unit,
                          const CharacterRenderOptions& options = {});

void DrawCharacterSprites(const CharacterView& view,
                          const std::vector<CharacterSpriteInstance>& sprites,
                          const CharacterArtBank& art,
                          const GridCell* cells, int grid_w, int grid_h,
                          CharCell* out_cells, int cell_w, int cell_h,
                          float focal_cells_per_unit,
                          const CharacterRenderOptions& options = {});

// Optional dialogue/inspect presentation. This is a bounded CharCell
// overlay, not a second renderer or an asset engine. World sprites remain
// transparent; the portrait owns only its explicit panel rectangle.
void DrawCharacterPortrait(CharCell* cells, int cell_w, int cell_h,
                           const CharacterArtBank& art,
                           CharacterSpriteKind kind, int left, int top,
                           int max_width, int max_height,
                           const CharacterRenderOptions& options = {});

void DrawPistolViewmodel(CharCell* cells, int cell_w, int cell_h,
                         const CharacterArtBank& art, PistolFrame frame,
                         float recoil_amount,
                         const CharacterRenderOptions& options = {});

// Character-first effects: glyphs carry the event while colour/background
// changes remain restrained.  reduce_* keep the information readable.
void DrawCharacterEffects(CharCell* cells, int cell_w, int cell_h,
                          uint64_t frame_index, bool shot_flash,
                          bool hit_flash, bool explosion,
                          bool reduce_flicker, bool reduce_shake);

// --dump-frame serializes the final CharCell frame as a terminal-shaped SVG.
// It is a frame dump of the actual production buffer, not a second gameplay
// renderer; external tooling may convert it to PNG for visual review.
bool WriteCharacterFrameSvg(const CharCell* cells, int cell_w, int cell_h,
                            const std::string& path);

} // namespace writeover
