#include "writeover/render/character_renderer.h"

#include "writeover/common/math.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace writeover {

namespace {

constexpr float kCharacterPi = 3.14159265358979323846f;
constexpr float kMaxSpriteDistance = 50.0f;
// World sprites remain readable at close range without consuming the whole
// 67-row terminal viewport.  This is a screen-space projection cap, not an
// art-scale shortcut: DrawOneSprite applies it to the effective world scale.
constexpr int kNearCharacterScreenCap = 44;
constexpr int kMidCharacterScreenCap = 48;
constexpr int kFarCharacterScreenCap = 24;
constexpr size_t kMaxArtAssets = 64;
// Weapon viewmodels may use a taller authored hand/forearm silhouette than
// world sprites, but remain bounded well below an unbounded text asset.
constexpr size_t kMaxArtRows = 48;
constexpr size_t kMaxArtColumns = 64;
// The authored pistol is a held viewmodel: its grip/hand is the stable
// screen-space anchor, while the muzzle remains several cells inward toward
// the gameplay centre.  These normalized values are intentionally local to
// the existing pistol path rather than a general viewmodel framework.
constexpr float kPistolGripAnchorU = 0.86f;
constexpr float kPistolGripAnchorV = 0.82f;
constexpr float kPistolGripScreenU = 0.82f;
constexpr float kPistolGripScreenV = 0.92f;

struct InkPalette {
    Color base;
    Color highlight;
    Color shadow;
    Color accent;
};

uint8_t Clamp8(float value) {
    return static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
}

float Saturate(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

Color ScaleColor(const Color& value, float scale, uint8_t max_value = 255) {
    return {static_cast<uint8_t>(std::min<float>(max_value,
                                                  value.r * scale)),
            static_cast<uint8_t>(std::min<float>(max_value,
                                                  value.g * scale)),
            static_cast<uint8_t>(std::min<float>(max_value,
                                                  value.b * scale))};
}

CharCell MakeCell(char32_t glyph, const Color& fg, const Color& bg,
                  uint8_t flags = 0) {
    CharCell cell;
    cell.code_point = glyph;
    cell.fg_r = fg.r;
    cell.fg_g = fg.g;
    cell.fg_b = fg.b;
    cell.bg_r = bg.r;
    cell.bg_g = bg.g;
    cell.bg_b = bg.b;
    cell.flags = flags;
    return cell;
}

InkPalette Palette(CharacterInk ink) {
    switch (ink) {
    case CharacterInk::Security:
        return {{106, 132, 148}, {214, 232, 224}, {37, 57, 72}, {224, 164, 58}};
    case CharacterInk::FullHuman:
        return {{166, 124, 151}, {246, 218, 202}, {54, 42, 60}, {102, 214, 199}};
    case CharacterInk::Maintenance:
        return {{142, 116, 68}, {224, 190, 106}, {55, 48, 40}, {89, 205, 181}};
    case CharacterInk::Terminal:
        return {{63, 162, 157}, {185, 246, 211}, {22, 66, 70}, {230, 176, 66}};
    case CharacterInk::Weapon:
        return {{128, 146, 156}, {232, 242, 232}, {34, 42, 52}, {230, 164, 57}};
    case CharacterInk::Prop:
    default:
        return {{128, 116, 102}, {220, 198, 155}, {42, 43, 48}, {231, 161, 58}};
    }
}

bool IsContinuation(uint8_t value) {
    return (value & 0xC0u) == 0x80u;
}

std::u32string DecodeUtf8(const std::string& text) {
    std::u32string out;
    out.reserve(std::min(text.size(), kMaxArtColumns));
    for (size_t i = 0; i < text.size() && out.size() < kMaxArtColumns;) {
        const uint8_t first = static_cast<uint8_t>(text[i++]);
        if (first < 0x80u) {
            out.push_back(static_cast<char32_t>(first));
            continue;
        }
        int extra = 0;
        char32_t value = 0;
        if ((first & 0xE0u) == 0xC0u) {
            extra = 1;
            value = first & 0x1Fu;
        } else if ((first & 0xF0u) == 0xE0u) {
            extra = 2;
            value = first & 0x0Fu;
        } else if ((first & 0xF8u) == 0xF0u) {
            extra = 3;
            value = first & 0x07u;
        } else {
            out.push_back(U'?');
            continue;
        }
        if (i + static_cast<size_t>(extra) > text.size()) {
            out.push_back(U'?');
            break;
        }
        bool valid = true;
        for (int n = 0; n < extra; ++n) {
            const uint8_t next = static_cast<uint8_t>(text[i++]);
            if (!IsContinuation(next)) {
                valid = false;
                break;
            }
            value = (value << 6) | (next & 0x3Fu);
        }
        if (!valid || value > 0x10FFFFu ||
            (value >= 0xD800u && value <= 0xDFFFu)) {
            out.push_back(U'?');
        } else {
            out.push_back(value);
        }
    }
    return out;
}

std::string TrimRight(std::string value) {
    while (!value.empty() && (value.back() == '\r' || value.back() == ' ' ||
                              value.back() == '\t')) {
        value.pop_back();
    }
    return value;
}

void DecodeArtRow(const std::string& text, std::u32string& glyphs,
                  std::vector<CharacterCellOpacity>& opacity) {
    glyphs = DecodeUtf8(text);
    opacity.clear();
    opacity.reserve(glyphs.size());
    for (auto& glyph : glyphs) {
        if (glyph == U'~') {
            // '~' is an authoring-only marker for an occupied blank cell. It
            // is never emitted as a visible glyph.
            glyph = U' ';
            opacity.push_back(CharacterCellOpacity::OpaqueEmpty);
        } else if (glyph == U' ') {
            opacity.push_back(CharacterCellOpacity::Transparent);
        } else {
            opacity.push_back(CharacterCellOpacity::Glyph);
        }
    }
}

bool ParseKind(const std::string& value, CharacterSpriteKind& out) {
    if (value == "security_guard") {
        out = CharacterSpriteKind::SecurityGuard;
    } else if (value == "full_human") {
        out = CharacterSpriteKind::FullHuman;
    } else if (value == "maintenance_worker") {
        out = CharacterSpriteKind::MaintenanceWorker;
    } else if (value == "terminal") {
        out = CharacterSpriteKind::Terminal;
    } else if (value == "camera") {
        out = CharacterSpriteKind::Camera;
    } else if (value == "crate") {
        out = CharacterSpriteKind::Crate;
    } else if (value == "door") {
        out = CharacterSpriteKind::Door;
    } else if (value == "body_unconscious") {
        out = CharacterSpriteKind::BodyUnconscious;
    } else if (value == "body_dead") {
        out = CharacterSpriteKind::BodyDead;
    } else {
        return false;
    }
    return true;
}

bool ParseLod(const std::string& value, CharacterLod& out) {
    if (value == "far") {
        out = CharacterLod::Far;
    } else if (value == "mid") {
        out = CharacterLod::Mid;
    } else if (value == "near") {
        out = CharacterLod::Near;
    } else {
        return false;
    }
    return true;
}

bool ParseFacing(const std::string& value, CharacterFacing& out) {
    if (value == "front") {
        out = CharacterFacing::Front;
    } else if (value == "back") {
        out = CharacterFacing::Back;
    } else if (value == "side_left") {
        out = CharacterFacing::SideLeft;
    } else if (value == "side_right" || value == "side") {
        out = CharacterFacing::SideRight;
    } else {
        return false;
    }
    return true;
}

bool ParseWeaponSlot(const std::string& value, WeaponSlot& out) {
    if (value == "pistol") {
        out = WeaponSlot::Pistol;
    } else if (value == "smg") {
        out = WeaponSlot::Smg;
    } else if (value == "stunner") {
        out = WeaponSlot::Stunner;
    } else {
        return false;
    }
    return true;
}

bool ParseInk(const std::string& value, CharacterInk& out) {
    if (value == "security") {
        out = CharacterInk::Security;
    } else if (value == "full") {
        out = CharacterInk::FullHuman;
    } else if (value == "maintenance") {
        out = CharacterInk::Maintenance;
    } else if (value == "terminal") {
        out = CharacterInk::Terminal;
    } else if (value == "weapon") {
        out = CharacterInk::Weapon;
    } else if (value == "prop") {
        out = CharacterInk::Prop;
    } else {
        return false;
    }
    return true;
}

bool ParsePistolFrame(const std::string& value, PistolFrame& out) {
    if (value == "idle_a") {
        out = PistolFrame::IdleA;
    } else if (value == "idle_b") {
        out = PistolFrame::IdleB;
    } else if (value == "fire") {
        out = PistolFrame::Fire;
    } else if (value == "reload") {
        out = PistolFrame::Reload;
    } else {
        return false;
    }
    return true;
}

// The fallback only protects startup diagnostics if a package is incomplete.
// Production evidence uses the authored data/characters file.
CharacterArtAsset FallbackAsset(CharacterSpriteKind kind, CharacterLod lod) {
    CharacterArtAsset asset;
    asset.sprite_kind = kind;
    asset.lod = lod;
    asset.facing = CharacterFacing::Front;
    asset.ink = kind == CharacterSpriteKind::SecurityGuard
                   ? CharacterInk::Security
                   : CharacterInk::Prop;
    asset.rows = {U" /\\ ", U"|oo|", U"/||\\", U" /\\ "};
    return asset;
}

CharacterArtAsset FallbackPistol(PistolFrame frame) {
    CharacterArtAsset asset;
    asset.is_pistol = true;
    asset.pistol_frame = frame;
    asset.weapon_slot = WeaponSlot::Pistol;
    asset.ink = CharacterInk::Weapon;
    asset.rows = {U"  ____====>", U" /___/", U"  ||", U" /__\\"};
    if (frame == PistolFrame::Fire) asset.rows[0] += U"*";
    return asset;
}

void BuildFallbackBank(std::vector<CharacterArtAsset>& assets) {
    const CharacterSpriteKind kinds[] = {
        CharacterSpriteKind::SecurityGuard,
        CharacterSpriteKind::FullHuman,
        CharacterSpriteKind::MaintenanceWorker,
        CharacterSpriteKind::Terminal,
        CharacterSpriteKind::Camera,
        CharacterSpriteKind::Crate,
        CharacterSpriteKind::Door,
        CharacterSpriteKind::BodyUnconscious,
        CharacterSpriteKind::BodyDead,
    };
    for (const auto kind : kinds) {
        for (const auto lod : {CharacterLod::Far, CharacterLod::Mid,
                               CharacterLod::Near}) {
            assets.push_back(FallbackAsset(kind, lod));
        }
    }
    for (const auto frame : {PistolFrame::IdleA, PistolFrame::IdleB,
                             PistolFrame::Fire, PistolFrame::Reload}) {
        assets.push_back(FallbackPistol(frame));
    }
}

GridCell SampleCell(const GridCell* cells, int grid_w, int grid_h,
                    float x, float y) {
    const int col = static_cast<int>(std::floor(x));
    const int row = static_cast<int>(std::floor(y));
    if (cells == nullptr || col < 0 || row < 0 || col >= grid_w || row >= grid_h) {
        GridCell edge;
        edge.material = 1;
        edge.light = 75;
        edge.flags = CellFlag_Solid;
        return edge;
    }
    return cells[row * grid_w + col];
}

Color MaterialBase(uint8_t material) {
    switch (material) {
    case 1: return {76, 105, 119};  // metal
    case 2: return {52, 116, 132};  // glass
    case 3: return {78, 59, 50};    // dirt
    case 4: return {106, 96, 88};   // concrete
    case 5: return {96, 68, 51};    // wood
    case 6: return {70, 92, 98};    // grate
    case 7: return {143, 91, 29};   // hazard
    default: return {96, 104, 110}; // facility wall
    }
}

Color SurfaceColor(uint8_t material, uint8_t light, float distance,
                   bool ceiling, bool high_contrast) {
    const Color base = MaterialBase(material);
    const float light_level = 0.30f + 0.70f * (light / 255.0f);
    const float fog = 0.40f + 0.60f * Saturate(1.0f - distance / 40.0f);
    // Planes stay near the facility dark base. Their seams and sparse glyphs
    // carry the spatial information; a broad bright fill must not become a
    // substitute for character art.
    const float scale = (ceiling ? 0.16f : 0.22f) * light_level * fog;
    const uint8_t cap = ceiling ? 22 : 36;
    Color out = ScaleColor(base, scale * (high_contrast ? 1.12f : 1.0f), cap);
    if (material == 7 && !ceiling) {
        out.r = Clamp8(std::min<float>(cap, out.r + 10.0f));
        out.g = Clamp8(std::min<float>(cap, out.g + 5.0f));
    }
    return out;
}

char32_t PlaneGlyph(bool ceiling, float world_x, float world_y,
                    float distance, int screen_x, int screen_y) {
    (void)distance;
    (void)screen_x;
    (void)screen_y;
    const int grid_x = static_cast<int>(std::lround(world_x));
    const int grid_y = static_cast<int>(std::lround(world_y));
    const bool major_x = (grid_x % 2) == 0;
    const bool major_y = (grid_y % 2) == 0;
    if (ceiling) {
        const bool beam_x = std::fabs(world_x - std::round(world_x)) < 0.055f;
        const bool beam_y = std::fabs(world_y - std::round(world_y)) < 0.055f;
        if (beam_x && beam_y && major_x && major_y) return U'┼';
        if (beam_x && major_x) return U'│';
        if (beam_y && major_y) return U'─';
        return U' ';
    }
    const bool seam_x = std::fabs(world_x - std::round(world_x)) < 0.065f;
    const bool seam_y = std::fabs(world_y - std::round(world_y)) < 0.065f;
    if (seam_x && seam_y && major_x && major_y) return U'┼';
    if (seam_x && major_x) return U'│';
    if (seam_y && major_y) return U'─';
    return U' ';
}

int PositiveModulo(int value, int period) {
    const int remainder = value % period;
    return remainder < 0 ? remainder + period : remainder;
}

bool NearWorldLine(float value, float spacing, float tolerance) {
    return std::fabs(value - std::round(value / spacing) * spacing) < tolerance;
}

char32_t WallGlyph(uint8_t material, float distance, float surface_u,
                   float surface_z, uint8_t flag) {
    const int u_band = static_cast<int>(std::floor(surface_u));
    const int z_band = static_cast<int>(std::floor(surface_z));
    const bool vertical_seam = NearWorldLine(surface_u, 1.0f, 0.065f);
    const bool horizontal_seam = NearWorldLine(surface_z, 1.0f, 0.055f);
    const bool near_detail = distance <= 18.0f;
    const bool mid_detail = distance <= 30.0f;
    if (flag == SegFloorRise || flag == SegFloorDrop) {
        if (horizontal_seam) return U'═';
        return near_detail && PositiveModulo(u_band * 3 + z_band * 5, 7) == 0
                   ? U'▒'
                   : U' ';
    }
    if (flag == SegCeilingRise || flag == SegCeilingDrop) {
        if (horizontal_seam) return U'─';
        return near_detail && PositiveModulo(u_band * 5 + z_band * 3, 11) == 0
                   ? U'░'
                   : U' ';
    }

    // Panel seams are the primary orientation cue.  Their coordinates come
    // from the world surface, not the destination screen cell, so moving or
    // turning the camera does not make the wall pattern swim.
    if (material == 1) {
        if (vertical_seam && horizontal_seam) return U'╫';
        if (vertical_seam) return U'║';
        if (horizontal_seam) return U'═';
        return near_detail && PositiveModulo(u_band * 3 + z_band * 7, 13) == 0
                   ? U'·'
                   : U' ';
    }
    if (material == 2) {
        if (vertical_seam && horizontal_seam) return U'┼';
        if (vertical_seam) return U'│';
        if (horizontal_seam) return U'─';
        return mid_detail && PositiveModulo(u_band * 5 + z_band * 11, 19) == 0
                   ? U'╱'
                   : U' ';
    }
    if (material == 6) {
        if (vertical_seam && horizontal_seam) return U'╬';
        if (vertical_seam) return U'╫';
        if (horizontal_seam) return U'┼';
        return near_detail && PositiveModulo(u_band * 7 + z_band * 2, 17) == 0
                   ? U'+'
                   : U' ';
    }
    if (material == 7) {
        return near_detail && PositiveModulo(u_band + z_band, 7) == 0
                   ? U'╱'
                   : (near_detail && PositiveModulo(u_band - z_band, 11) == 0
                          ? U'╲'
                          : U' ');
    }
    if (material == 3 || material == 5) {
        if (vertical_seam && horizontal_seam) return U'┼';
        if (vertical_seam) return U'│';
        if (horizontal_seam) return U'─';
        return near_detail && PositiveModulo(u_band * 3 + z_band, 17) == 0
                   ? U'·'
                   : U' ';
    }
    if (vertical_seam && horizontal_seam) return U'┼';
    if (vertical_seam) return U'│';
    if (horizontal_seam) return U'─';
    return near_detail && PositiveModulo(u_band * 3 + z_band * 5, 19) == 0
               ? U'·'
               : U' ';
}

CharCell WallCell(const OccludingSegment& segment, float surface_u,
                  float surface_z,
                  const CharacterRenderOptions& options) {
    const Color base = MaterialBase(segment.material);
    const float light_level = 0.27f + 0.73f * (segment.light / 255.0f);
    const float fog = 0.38f + 0.62f * Saturate(1.0f - segment.distance / 40.0f);
    float scale = light_level * fog * (options.high_contrast ? 1.10f : 1.0f);
    if (segment.flag == SegFloorRise || segment.flag == SegFloorDrop) scale *= 0.86f;
    if (segment.flag == SegCeilingRise || segment.flag == SegCeilingDrop) scale *= 0.70f;
    const Color fg = ScaleColor(base, 0.72f * scale, 170);
    const Color bg = {6, 10, 15};
    const char32_t glyph = WallGlyph(segment.material, segment.distance,
                                      surface_u, surface_z, segment.flag);
    return MakeCell(glyph, fg, bg, glyph == U'╬' || glyph == U'═' ? 0x01 : 0);
}

float NormalizeAngle(float angle) {
    while (angle > kCharacterPi) angle -= 2.0f * kCharacterPi;
    while (angle < -kCharacterPi) angle += 2.0f * kCharacterPi;
    return angle;
}

float WallSurfaceCoordinate(const CharacterView& view, const RayConfig& ray,
                            float distance) {
    const float hit_x = view.origin.x + std::cos(ray.yaw) * distance;
    const float hit_y = view.origin.y + std::sin(ray.yaw) * distance;
    const float edge_x = std::fabs(hit_x - std::round(hit_x));
    const float edge_y = std::fabs(hit_y - std::round(hit_y));
    // A vertical grid face runs along Y; a horizontal face runs along X.
    // Corner ties are deterministic and use the Y-running face.
    return edge_x <= edge_y ? hit_y : hit_x;
}

Color ApplyFullHumanForegroundFloor(const CharacterArtAsset& asset,
                                     float distance, const Color& color) {
    // Keep the accepted environment palette unchanged while preventing the
    // authored Full Human lower silhouette from disappearing into B1's dark
    // walls at MID/NEAR range.  This is a local foreground floor, not a
    // global outline or scene-brightening pass.
    if (asset.ink == CharacterInk::FullHuman &&
        asset.lod != CharacterLod::Far && distance < 12.0f) {
        const float luma = 0.2126f * color.r + 0.7152f * color.g +
                           0.0722f * color.b;
        const float floor = distance < 4.0f ? 76.0f : 66.0f;
        if (luma > 0.0f && luma < floor) {
            return ScaleColor(color, floor / luma, 220);
        }
    }
    return color;
}

Color SpriteForeground(const CharacterArtAsset& asset, char32_t glyph,
                       float distance, uint8_t light,
                       const CharacterRenderOptions& options) {
    const InkPalette palette = Palette(asset.ink);
    const float depth = 0.52f + 0.48f * Saturate(1.0f - distance / 32.0f);
    const float light_level = 0.45f + 0.55f * (light / 255.0f);
    const Color shaded = ScaleColor(palette.base, depth * light_level *
                                             (options.high_contrast ? 1.08f : 1.0f),
                                    210);
    if (glyph == U'o' || glyph == U'O' || glyph == U'@' || glyph == U'=' ||
        glyph == U'[' || glyph == U']') {
        return ApplyFullHumanForegroundFloor(
            asset, distance, ScaleColor(palette.highlight, depth * light_level, 235));
    }
    if (glyph == U'!' || glyph == U'*' || glyph == U'+' || glyph == U'>' ||
        glyph == U'<' || glyph == U'╳') {
        return ApplyFullHumanForegroundFloor(
            asset, distance, ScaleColor(palette.accent, depth * light_level, 240));
    }
    if (glyph == U'_' || glyph == U'.' || glyph == U'/' || glyph == U'\\') {
        return ApplyFullHumanForegroundFloor(
            asset, distance, ScaleColor(palette.shadow, depth * light_level, 150));
    }
    return ApplyFullHumanForegroundFloor(asset, distance, shaded);
}

void SetEffectCell(CharCell* cells, int width, int height, int x, int y,
                   char32_t glyph, const Color& fg, const Color& bg) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    cells[static_cast<size_t>(y) * width + x] = MakeCell(glyph, fg, bg, 0x01);
}

bool IsXmlSafe(char32_t cp) {
    return cp == U'\t' || cp == U'\n' || cp == U'\r' || cp >= 0x20;
}

std::string XmlEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

bool IsEyeGlyph(char32_t glyph) {
    return glyph == U'o' || glyph == U'O' || glyph == U'@';
}

void ResolveFacingCell(const CharacterArtAsset& asset, int source_x,
                       int source_y, CharacterFacing fallback_facing,
                       char32_t& glyph, CharacterCellOpacity& opacity) {
    opacity = asset.OpacityAt(source_x, source_y);
    if (opacity == CharacterCellOpacity::Transparent ||
        source_y < 0 || source_y >= asset.Height() ||
        source_x < 0 || source_x >= static_cast<int>(
            asset.rows[static_cast<size_t>(source_y)].size())) {
        opacity = CharacterCellOpacity::Transparent;
        return;
    }
    glyph = asset.rows[static_cast<size_t>(source_y)][static_cast<size_t>(source_x)];
    if (opacity == CharacterCellOpacity::Glyph && !IsSingleWidthGlyph(glyph)) {
        opacity = CharacterCellOpacity::Transparent;
        return;
    }

    // Directional art is authored per facing whenever available.  The
    // fallback transform is retained only for an incomplete/old bank so a
    // missing optional direction does not make a production sprite vanish.
    const bool face_band = source_y < std::max(1, asset.Height() / 3);
    if (face_band && IsEyeGlyph(glyph)) {
        if (fallback_facing == CharacterFacing::Back) {
            // A back view keeps the authored silhouette but does not expose
            // the front-facing eyes.
            glyph = U'=';
        } else if (fallback_facing == CharacterFacing::SideLeft &&
                   source_x > asset.Width() / 2) {
            opacity = CharacterCellOpacity::Transparent;
        } else if (fallback_facing == CharacterFacing::SideRight &&
                   source_x < asset.Width() / 2) {
            opacity = CharacterCellOpacity::Transparent;
        }
    }
}

using CharacterDepthBuffer = std::vector<float>;

void BuildWallDepthBuffer(const CameraProjection& projection,
                          const GridCell* cells, int grid_w, int grid_h,
                          CharacterDepthBuffer& depths) {
    const size_t expected = static_cast<size_t>(std::max(0, projection.screen_width)) *
                            static_cast<size_t>(std::max(0, projection.screen_height));
    depths.assign(expected, std::numeric_limits<float>::infinity());
    if (cells == nullptr || grid_w <= 0 || grid_h <= 0 || expected == 0) return;

    for (int x = 0; x < projection.screen_width; ++x) {
        RayConfig ray_config;
        ray_config.origin_xy = Vec2{projection.origin.x, projection.origin.y};
        ray_config.yaw = projection.ColumnYaw(x);
        const RayResult ray = CastColumnRay(ray_config, cells, grid_w, grid_h);
        for (uint32_t i = 0; i < ray.segment_count; ++i) {
            const OccludingSegment& segment = ray.segments[i];
            const WallProjection wall = ProjectWall(
                segment, projection.origin.z, projection.pitch,
                projection.focal_y, projection.screen_height);
            if (!wall.visible || !std::isfinite(wall.screen_top_y) ||
                !std::isfinite(wall.screen_bottom_y) ||
                wall.screen_top_y > wall.screen_bottom_y) {
                continue;
            }
            const int top = std::max(0, static_cast<int>(std::ceil(
                wall.screen_top_y)));
            const int bottom = std::min(
                projection.screen_height - 1,
                static_cast<int>(std::floor(wall.screen_bottom_y)));
            for (int y = top; y <= bottom; ++y) {
                float& depth = depths[static_cast<size_t>(y) *
                                      projection.screen_width + x];
                depth = std::min(depth, segment.distance);
            }
        }
    }
}

void DrawOneSprite(const CharacterView& view,
                   const CharacterSpriteInstance& instance,
                   const CharacterArtBank& art,
                   const GridCell* cells, int grid_w, int grid_h,
                   CharCell* out_cells, int cell_w, int cell_h,
                   float focal_cells_per_unit,
                   const CharacterRenderOptions& options,
                   const CharacterDepthBuffer& wall_depths,
                   CharacterDepthBuffer& sprite_depths) {
    (void)cells;
    (void)grid_w;
    (void)grid_h;
    const CameraProjection projection(
        view.origin, view.yaw, view.pitch, cell_w, cell_h,
        focal_cells_per_unit, kCharacterCellAspect);
    const float dx = instance.position.x - view.origin.x;
    const float dy = instance.position.y - view.origin.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance < 0.05f || distance > kMaxSpriteDistance) return;
    const Vec3 actor_center{instance.position.x, instance.position.y,
                            instance.position.z + instance.height * 0.5f};
    const float depth = projection.Depth(actor_center);
    const float horizontal_depth = projection.HorizontalDepth(instance.position);
    if (!std::isfinite(depth) || depth <= 0.05f ||
        !std::isfinite(horizontal_depth) || horizontal_depth <= 0.05f) return;

    const CharacterLod lod = instance.has_lod_hint
                                 ? instance.lod_hint
                                 : SelectCharacterLod(distance);
    const CharacterFacing facing = SelectCharacterFacing(
        instance.yaw, instance.position, view.origin);
    const CharacterArtAsset* asset = art.Find(instance.kind, lod, facing);
    bool fallback_facing = false;
    bool mirror_side = false;
    if (asset == nullptr) {
        asset = art.Find(instance.kind, lod);
        fallback_facing = asset != nullptr && facing != CharacterFacing::Front;
        mirror_side = fallback_facing && facing == CharacterFacing::SideLeft;
    }
    if (asset == nullptr || asset->Height() <= 0 || asset->Width() <= 0) return;

    const float center_x = projection.ScreenX(actor_center);
    if (!std::isfinite(center_x)) return;
    const int lod_cap = lod == CharacterLod::Near ? kNearCharacterScreenCap
                        : lod == CharacterLod::Mid ? kMidCharacterScreenCap
                                                   : kFarCharacterScreenCap;
    const float actor_height = std::max(instance.height, 0.01f);
    const float raw_scale = focal_cells_per_unit / depth;
    const float max_scale = static_cast<float>(lod_cap) / actor_height;
    const float effective_scale = std::min(raw_scale, max_scale);
    if (!std::isfinite(raw_scale) || !std::isfinite(effective_scale) ||
        effective_scale <= 0.0f) return;
    const float projected_height = actor_height * effective_scale;
    const int dst_h = std::clamp(static_cast<int>(std::lround(projected_height)),
                                 1, lod_cap);
    const int dst_w = std::max(1, static_cast<int>(std::lround(
        static_cast<float>(asset->Width()) * dst_h /
        static_cast<float>(asset->Height()))));
    const float ground_raw = projection.ScreenY(instance.position);
    if (!std::isfinite(ground_raw)) return;
    // Every vertical placement uses the same effective scale as the capped
    // sprite height.  In particular, do not anchor the feet with raw_scale
    // after a close-range cap has bound.  Scaling the camera-space foot
    // offset keeps the pinhole pitch relationship while applying the cap.
    const float ground_row = projection.CenterY() +
        (ground_raw - projection.CenterY()) * (effective_scale / raw_scale);
    const float top_row = ground_row - projected_height;
    const float bottom_row = ground_row;
    if (bottom_row < 0.0f || top_row >= static_cast<float>(cell_h)) return;

    const int left = static_cast<int>(std::floor(center_x - dst_w * 0.5f));
    const int right = left + dst_w - 1;
    if (right < 0 || left >= cell_w) return;
    const int top = std::max(0, static_cast<int>(std::floor(top_row)));
    const int bottom = std::min(cell_h - 1,
                                static_cast<int>(std::ceil(bottom_row)));
    for (int x = left; x <= right; ++x) {
        if (x < 0 || x >= cell_w) continue;
        const float ray_depth = projection.HorizontalRayDepthToPoint(
            instance.position, x);
        if (!std::isfinite(ray_depth) || ray_depth <= 0.05f) continue;

        const int local_x = x - left;
        int source_x = std::clamp(
            static_cast<int>(static_cast<float>(local_x) * asset->Width() /
                             static_cast<float>(dst_w)),
            0, asset->Width() - 1);
        if (fallback_facing && (facing == CharacterFacing::Back || mirror_side)) {
            source_x = asset->Width() - 1 - source_x;
        }
        for (int y = top; y <= bottom; ++y) {
            const int local_y = std::clamp(
                static_cast<int>((static_cast<float>(y) - top_row) *
                                 asset->Height() /
                                 std::max(1.0f, bottom_row - top_row)),
                0, asset->Height() - 1);
            char32_t glyph = U' ';
            CharacterCellOpacity opacity = CharacterCellOpacity::Transparent;
            ResolveFacingCell(*asset, source_x, local_y,
                              fallback_facing ? facing : CharacterFacing::Front,
                              glyph, opacity);
            if (opacity == CharacterCellOpacity::Transparent) continue;
            const size_t index = static_cast<size_t>(y) * cell_w + x;
            if (index >= wall_depths.size() || index >= sprite_depths.size()) {
                continue;
            }
            if (wall_depths[index] < ray_depth - 0.05f ||
                sprite_depths[index] <= ray_depth + 0.05f) {
                continue;
            }
            const Color bg{5, 9, 14};
            const Color fg = opacity == CharacterCellOpacity::OpaqueEmpty
                ? SpriteForeground(*asset, U'_', distance, 210, options)
                : SpriteForeground(*asset, glyph, distance, 210, options);
            out_cells[index] = MakeCell(
                opacity == CharacterCellOpacity::OpaqueEmpty ? U' ' : glyph,
                fg, bg,
                opacity == CharacterCellOpacity::Glyph && IsEyeGlyph(glyph)
                    ? 0x01 : 0);
            sprite_depths[index] = ray_depth;
        }
    }
}

} // namespace

int CharacterArtAsset::Width() const {
    size_t width = 0;
    for (const auto& row : rows) width = std::max(width, row.size());
    return static_cast<int>(width);
}

CharacterCellOpacity CharacterArtAsset::OpacityAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= Height()) {
        return CharacterCellOpacity::Transparent;
    }
    const auto& row = rows[static_cast<size_t>(y)];
    if (x >= static_cast<int>(row.size())) {
        return CharacterCellOpacity::Transparent;
    }
    if (y < static_cast<int>(opacity.size()) &&
        x < static_cast<int>(opacity[static_cast<size_t>(y)].size())) {
        return opacity[static_cast<size_t>(y)][static_cast<size_t>(x)];
    }
    return row[static_cast<size_t>(x)] == U' '
               ? CharacterCellOpacity::Transparent
               : CharacterCellOpacity::Glyph;
}

CharacterArtBank::CharacterArtBank() {
    BuildFallbackBank(assets_);
}

bool CharacterArtBank::Load(const std::string& path) {
    std::ifstream input(path);
    if (!input) return false;

    std::vector<CharacterArtAsset> parsed;
    CharacterArtAsset current;
    bool in_asset = false;
    bool is_pistol = false;
    std::string line;
    while (std::getline(input, line)) {
        line = TrimRight(std::move(line));
        if (!in_asset) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream header(line);
            std::string record;
            std::string name;
            std::string detail;
            std::string ink_name;
            std::string facing_name;
            CharacterFacing parsed_facing = CharacterFacing::Front;
            if (!(header >> record >> name >> detail >> ink_name) ||
                (record != "sprite" && record != "weapon") ||
                parsed.size() >= kMaxArtAssets || !ParseInk(ink_name, current.ink)) {
                return false;
            }
            if (header >> facing_name && !ParseFacing(facing_name, parsed_facing)) {
                return false;
            }
            std::string unexpected;
            if (header >> unexpected) return false;
            current = CharacterArtAsset{};
            current.facing = parsed_facing;
            if (!ParseInk(ink_name, current.ink)) return false;
            is_pistol = record == "weapon";
            current.is_pistol = is_pistol;
            if (is_pistol) {
                if (!ParseWeaponSlot(name, current.weapon_slot) ||
                    !ParsePistolFrame(detail, current.pistol_frame)) {
                    return false;
                }
                current.ink = CharacterInk::Weapon;
            } else if (!ParseKind(name, current.sprite_kind) ||
                       !ParseLod(detail, current.lod)) {
                return false;
            }
            in_asset = true;
            continue;
        }
        if (line == "end") {
            if (current.rows.empty() || current.Width() <= 0) return false;
            parsed.push_back(std::move(current));
            current = CharacterArtAsset{};
            in_asset = false;
            is_pistol = false;
            continue;
        }
        if (current.rows.size() >= kMaxArtRows || line.size() > kMaxArtColumns * 4) {
            return false;
        }
        std::u32string glyphs;
        std::vector<CharacterCellOpacity> row_opacity;
        DecodeArtRow(line, glyphs, row_opacity);
        current.rows.push_back(std::move(glyphs));
        current.opacity.push_back(std::move(row_opacity));
        if (current.Width() > static_cast<int>(kMaxArtColumns)) return false;
    }
    if (in_asset || parsed.empty()) return false;

    assets_ = std::move(parsed);
    loaded_from_file_ = true;
    return true;
}

const CharacterArtAsset* CharacterArtBank::Find(CharacterSpriteKind kind,
                                                CharacterLod lod) const {
    return Find(kind, lod, CharacterFacing::Front);
}

const CharacterArtAsset* CharacterArtBank::Find(CharacterSpriteKind kind,
                                                CharacterLod lod,
                                                CharacterFacing facing) const {
    for (const auto& asset : assets_) {
        if (!asset.is_pistol && asset.sprite_kind == kind && asset.lod == lod &&
            asset.facing == facing) {
            return &asset;
        }
    }
    // A single authored side profile is allowed to serve both side views;
    // DrawOneSprite mirrors it only when the requested side is absent.
    if (facing == CharacterFacing::SideLeft) {
        for (const auto& asset : assets_) {
            if (!asset.is_pistol && asset.sprite_kind == kind &&
                asset.lod == lod && asset.facing == CharacterFacing::SideRight) {
                return &asset;
            }
        }
    }
    return nullptr;
}

const CharacterArtAsset* CharacterArtBank::FindWeapon(WeaponSlot slot,
                                                       PistolFrame frame) const {
    for (const auto& asset : assets_) {
        if (asset.is_pistol && asset.weapon_slot == slot &&
            asset.pistol_frame == frame) return &asset;
    }
    return nullptr;
}

const CharacterArtAsset* CharacterArtBank::FindPistol(PistolFrame frame) const {
    return FindWeapon(WeaponSlot::Pistol, frame);
}

CharacterLod SelectCharacterLod(float distance_meters) {
    if (distance_meters < 4.0f) return CharacterLod::Near;
    if (distance_meters < 12.0f) return CharacterLod::Mid;
    return CharacterLod::Far;
}

CharacterLod SelectCharacterLodHysteretic(float distance_meters,
                                          CharacterLod previous) {
    if (!std::isfinite(distance_meters)) return previous;
    switch (previous) {
    case CharacterLod::Near:
        return distance_meters >= 4.5f ? CharacterLod::Mid : CharacterLod::Near;
    case CharacterLod::Mid:
        if (distance_meters < 3.5f) return CharacterLod::Near;
        if (distance_meters >= 12.5f) return CharacterLod::Far;
        return CharacterLod::Mid;
    case CharacterLod::Far:
    default:
        return distance_meters < 11.5f ? CharacterLod::Mid : CharacterLod::Far;
    }
}

CharacterFacing SelectCharacterFacing(float actor_yaw,
                                      const Vec3& actor_position,
                                      const Vec3& camera_position) {
    const float to_camera = std::atan2(camera_position.y - actor_position.y,
                                       camera_position.x - actor_position.x);
    const float relative = NormalizeAngle(actor_yaw - to_camera);
    const float absolute = std::fabs(relative);
    if (absolute <= kCharacterPi * 0.25f) return CharacterFacing::Front;
    if (absolute >= kCharacterPi * 0.75f) return CharacterFacing::Back;
    return relative > 0.0f ? CharacterFacing::SideRight
                           : CharacterFacing::SideLeft;
}

void RenderCharacterFrame(const GridCell* cells, int grid_w, int grid_h,
                          const CharacterView& view,
                          CharCell* out_cells, int cell_w, int cell_h,
                          float focal_cells_per_unit,
                          const CharacterRenderOptions& options) {
    if (out_cells == nullptr || cell_w <= 0 || cell_h <= 0 ||
        grid_w <= 0 || grid_h <= 0 || focal_cells_per_unit <= 0.01f) {
        return;
    }
    const CharCell clear = MakeCell(U' ', {138, 148, 156}, {5, 9, 14});
    std::fill(out_cells, out_cells + static_cast<size_t>(cell_w) * cell_h, clear);

    const CameraProjection camera(view.origin, view.yaw, view.pitch,
                                  cell_w, cell_h, focal_cells_per_unit,
                                  kCharacterCellAspect);
    for (int x = 0; x < cell_w; ++x) {
        RayConfig ray_config;
        ray_config.origin_xy = Vec2{view.origin.x, view.origin.y};
        ray_config.yaw = camera.ColumnYaw(x);
        const RayResult ray = CastColumnRay(ray_config, cells, grid_w, grid_h);
        for (int y = 0; y < cell_h; ++y) {
            const bool ceiling = static_cast<float>(y) + 0.5f < camera.CenterY();
            const float plane_z = ceiling ? ray.final_ceiling_z : ray.final_floor_z;
            const Vec3 ray_direction = camera.RayDirectionAt(
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f);
            float ray_distance = std::numeric_limits<float>::quiet_NaN();
            if (std::fabs(ray_direction.z) > 1e-5f) {
                ray_distance = (plane_z - view.origin.z) / ray_direction.z;
            }
            Vec3 world_point;
            if (std::isfinite(ray_distance) && ray_distance > 0.0f) {
                ray_distance = std::min(kMaxSpriteDistance, ray_distance);
                world_point = view.origin + ray_direction * ray_distance;
            } else {
                const Vec3 horizontal = camera.HorizontalColumnDirection(x);
                ray_distance = kMaxSpriteDistance;
                world_point = view.origin + horizontal * ray_distance;
            }
            const float distance = std::min(
                kMaxSpriteDistance,
                std::max(0.25f, std::sqrt(
                    (world_point.x - view.origin.x) *
                        (world_point.x - view.origin.x) +
                    (world_point.y - view.origin.y) *
                        (world_point.y - view.origin.y))));
            const float world_x = world_point.x;
            const float world_y = world_point.y;
            const GridCell sample = SampleCell(cells, grid_w, grid_h,
                                               world_x, world_y);
            const char32_t glyph = PlaneGlyph(ceiling, world_x, world_y,
                                              distance, x, y);
            const Color fg = ceiling
                ? ScaleColor({77, 92, 101}, 0.52f + 0.48f *
                                  Saturate(1.0f - distance / 40.0f),
                              options.high_contrast ? 115 : 90)
                : ScaleColor({92, 108, 111}, 0.55f + 0.45f *
                                  Saturate(1.0f - distance / 40.0f),
                              options.high_contrast ? 135 : 120);
            const Color bg = SurfaceColor(sample.material, sample.light, distance,
                                          ceiling, options.high_contrast);
            out_cells[static_cast<size_t>(y) * cell_w + x] =
                MakeCell(glyph, fg, bg, 0);
        }
        for (uint32_t i = ray.segment_count; i > 0; --i) {
            const OccludingSegment& segment = ray.segments[i - 1];
            const WallProjection wall_projection = ProjectWall(
                segment, view.origin.z, view.pitch, focal_cells_per_unit, cell_h);
            if (!wall_projection.visible) continue;
            const float surface_u = WallSurfaceCoordinate(
                view, ray_config, segment.distance);
            const float screen_span = std::max(
                wall_projection.screen_bottom_y - wall_projection.screen_top_y,
                0.001f);
            const int top = std::max(0, static_cast<int>(std::ceil(
                wall_projection.screen_top_y)));
            const int bottom = std::min(cell_h - 1, static_cast<int>(std::floor(
                wall_projection.screen_bottom_y)));
            for (int y = top; y <= bottom; ++y) {
                const float vertical_t = std::clamp(
                    (static_cast<float>(y) + 0.5f - wall_projection.screen_top_y) /
                        screen_span,
                    0.0f, 1.0f);
                const float surface_z = segment.top_z +
                    (segment.bottom_z - segment.top_z) * vertical_t;
                out_cells[static_cast<size_t>(y) * cell_w + x] =
                    WallCell(segment, surface_u, surface_z, options);
            }
        }
    }
}

void DrawCharacterSprites(const CharacterView& view,
                          const std::vector<CharacterSpriteInstance>& sprites,
                          const CharacterArtBank& art,
                          const GridCell* cells, int grid_w, int grid_h,
                          CharCell* out_cells, int cell_w, int cell_h,
                          float focal_cells_per_unit,
                          const CharacterRenderOptions& options) {
    if (out_cells == nullptr || cell_w <= 0 || cell_h <= 0) return;
    const CameraProjection projection(
        view.origin, view.yaw, view.pitch, cell_w, cell_h,
        focal_cells_per_unit, kCharacterCellAspect);
    CharacterDepthBuffer wall_depths;
    BuildWallDepthBuffer(projection, cells, grid_w, grid_h, wall_depths);
    CharacterDepthBuffer sprite_depths(
        static_cast<size_t>(cell_w) * cell_h,
        std::numeric_limits<float>::infinity());
    std::vector<CharacterSpriteInstance> ordered = sprites;
    std::stable_sort(ordered.begin(), ordered.end(), [&](const auto& left,
                                                         const auto& right) {
        return projection.HorizontalDepth(left.position) >
               projection.HorizontalDepth(right.position);
    });
    for (const auto& sprite : ordered) {
        DrawOneSprite(view, sprite, art, cells, grid_w, grid_h, out_cells,
                      cell_w, cell_h, focal_cells_per_unit, options,
                      wall_depths, sprite_depths);
    }
}

void DrawCharacterPortrait(CharCell* cells, int cell_w, int cell_h,
                           const CharacterArtBank& art,
                           CharacterSpriteKind kind, int left, int top,
                           int max_width, int max_height,
                           const CharacterRenderOptions& options) {
    if (cells == nullptr || cell_w <= 0 || cell_h <= 0 || max_width < 4 ||
        max_height < 4 || left >= cell_w || top >= cell_h ||
        left + max_width <= 0 || top + max_height <= 0) {
        return;
    }
    const CharacterArtAsset* asset = art.Find(kind, CharacterLod::Near);
    if (asset == nullptr || asset->Width() <= 0 || asset->Height() <= 0) {
        return;
    }
    const int source_width = asset->Width();
    const int source_height = asset->Height();

    const int panel_left = std::max(0, left);
    const int panel_top = std::max(0, top);
    const int panel_right = std::min(cell_w - 1, left + max_width - 1);
    const int panel_bottom = std::min(cell_h - 1, top + max_height - 1);
    if (panel_left > panel_right || panel_top > panel_bottom) return;

    const InkPalette palette = Palette(asset->ink);
    const Color panel_bg{8, 12, 18};
    const Color panel_fg = ScaleColor(palette.accent, 0.95f, 235);
    for (int y = panel_top; y <= panel_bottom; ++y) {
        for (int x = panel_left; x <= panel_right; ++x) {
            const bool top_edge = y == panel_top;
            const bool bottom_edge = y == panel_bottom;
            const bool left_edge = x == panel_left;
            const bool right_edge = x == panel_right;
            char32_t glyph = U' ';
            if (top_edge && left_edge) glyph = U'╔';
            else if (top_edge && right_edge) glyph = U'╗';
            else if (bottom_edge && left_edge) glyph = U'╚';
            else if (bottom_edge && right_edge) glyph = U'╝';
            else if (top_edge || bottom_edge) glyph = U'═';
            else if (left_edge || right_edge) glyph = U'║';
            cells[static_cast<size_t>(y) * cell_w + x] =
                MakeCell(glyph, panel_fg, panel_bg, glyph == U' ' ? 0 : 0x01);
        }
    }

    const int inner_left = panel_left + 1;
    const int inner_top = panel_top + 1;
    const int inner_width = std::max(1, panel_right - panel_left - 1);
    const int inner_height = std::max(1, panel_bottom - panel_top - 1);
    const float scale = std::min(
        static_cast<float>(inner_width) / static_cast<float>(source_width),
        static_cast<float>(inner_height) / static_cast<float>(source_height));
    const int dst_width = std::max(1, static_cast<int>(std::floor(
        source_width * scale)));
    const int dst_height = std::max(1, static_cast<int>(std::floor(
        source_height * scale)));
    const int art_left = inner_left + std::max(0, (inner_width - dst_width) / 2);
    const int art_top = inner_top + std::max(0, (inner_height - dst_height) / 2);
    for (int y = 0; y < dst_height; ++y) {
        const int target_y = art_top + y;
        if (target_y < inner_top || target_y >= panel_bottom) continue;
        const int source_y = std::clamp(static_cast<int>(
            y * source_height / static_cast<float>(dst_height)),
            0, source_height - 1);
        const std::u32string& row = asset->rows[static_cast<size_t>(source_y)];
        for (int x = 0; x < dst_width; ++x) {
            const int target_x = art_left + x;
            if (target_x < inner_left || target_x >= panel_right) continue;
            const int source_x = std::clamp(static_cast<int>(
                x * source_width / static_cast<float>(dst_width)),
                0, source_width - 1);
            if (source_x >= static_cast<int>(row.size())) continue;
            const char32_t glyph = row[static_cast<size_t>(source_x)];
            if (glyph == U' ' || !IsSingleWidthGlyph(glyph)) continue;
            cells[static_cast<size_t>(target_y) * cell_w + target_x] =
                MakeCell(glyph, SpriteForeground(*asset, glyph, 0.0f, 255,
                                                  options), panel_bg,
                         glyph == U'o' || glyph == U'O' ? 0x01 : 0);
        }
    }
}

void DrawPistolViewmodel(CharCell* cells, int cell_w, int cell_h,
                         const CharacterArtBank& art, PistolFrame frame,
                         float recoil_amount,
                         const CharacterRenderOptions& options) {
    DrawWeaponViewmodel(cells, cell_w, cell_h, art, WeaponSlot::Pistol, frame,
                        recoil_amount, options);
}

void DrawWeaponViewmodel(CharCell* cells, int cell_w, int cell_h,
                         const CharacterArtBank& art, WeaponSlot slot,
                         PistolFrame frame, float recoil_amount,
                         const CharacterRenderOptions& options) {
    if (cells == nullptr || cell_w <= 0 || cell_h <= 0) return;
    const CharacterArtAsset* asset = art.FindWeapon(slot, frame);
    if (asset == nullptr || asset->Width() <= 0 || asset->Height() <= 0) return;
    const int source_width = asset->Width();
    const int source_height = asset->Height();
    const int max_width = std::max(8, static_cast<int>(cell_w * 0.32f));
    const int max_height = std::max(5, static_cast<int>(cell_h * 0.43f));
    const float scale = std::min(static_cast<float>(max_width) / source_width,
                                 static_cast<float>(max_height) / source_height);
    const int dst_w = std::max(1, static_cast<int>(std::floor(source_width * scale)));
    const int dst_h = std::max(1, static_cast<int>(std::floor(source_height * scale)));
    const float grip_screen_x = static_cast<float>(cell_w) * kPistolGripScreenU;
    const float grip_screen_y = static_cast<float>(cell_h) * kPistolGripScreenV;
    const int base_x = static_cast<int>(std::lround(
        grip_screen_x - kPistolGripAnchorU * static_cast<float>(dst_w)));
    const int base_y = static_cast<int>(std::lround(
        grip_screen_y - kPistolGripAnchorV * static_cast<float>(dst_h) -
        Saturate(recoil_amount) * 2.0f));
    const InkPalette palette = Palette(CharacterInk::Weapon);
    for (int dy = 0; dy < dst_h; ++dy) {
        const int source_y = std::clamp(static_cast<int>(dy * source_height /
                                                         static_cast<float>(dst_h)),
                                        0, source_height - 1);
        const std::u32string& row = asset->rows[static_cast<size_t>(source_y)];
        for (int dx = 0; dx < dst_w; ++dx) {
            const int source_x = std::clamp(static_cast<int>(dx * source_width /
                                                         static_cast<float>(dst_w)),
                                            0, source_width - 1);
            if (source_x >= static_cast<int>(row.size())) continue;
            const char32_t glyph = row[static_cast<size_t>(source_x)];
            if (glyph == U' ' || !IsSingleWidthGlyph(glyph)) continue;
            const Color fg = (glyph == U'=' || glyph == U'>' || glyph == U'*')
                ? ScaleColor(palette.accent, options.high_contrast ? 1.08f : 1.0f, 245)
                : SpriteForeground(*asset, glyph, 2.0f, 255, options);
            const int x = base_x + dx;
            const int y = base_y + dy;
            if (x < 0 || y < 0 || x >= cell_w || y >= cell_h) continue;
            cells[static_cast<size_t>(y) * cell_w + x] =
                MakeCell(glyph, fg, {5, 9, 14}, 0x01);
        }
    }
}

void DrawCharacterEffects(CharCell* cells, int cell_w, int cell_h,
                          uint64_t frame_index, bool shot_flash,
                          bool hit_flash, bool explosion,
                          bool reduce_flicker, bool reduce_shake) {
    if (cells == nullptr || cell_w <= 0 || cell_h <= 0) return;
    const int cx = cell_w / 2;
    const int cy = cell_h / 2;
    if (shot_flash && (!reduce_flicker || frame_index % 2 == 0)) {
        SetEffectCell(cells, cell_w, cell_h, cx, cy, U'✦', {255, 220, 104},
                      {84, 42, 20});
        SetEffectCell(cells, cell_w, cell_h, cx - 2, cy, U'*', {245, 168, 66},
                      {31, 24, 18});
        SetEffectCell(cells, cell_w, cell_h, cx + 2, cy, U'*', {245, 168, 66},
                      {31, 24, 18});
    }
    if (hit_flash) {
        const Color red{226, 70, 74};
        for (int x = 0; x < cell_w; x += std::max(1, cell_w / 12)) {
            SetEffectCell(cells, cell_w, cell_h, x, 0, U'!', red, {34, 10, 18});
            SetEffectCell(cells, cell_w, cell_h, x, cell_h - 1, U'!', red, {34, 10, 18});
        }
    }
    if (explosion && (!reduce_flicker || frame_index % 2 == 0)) {
        const char32_t glyphs[] = {U'╳', U'✦', U'*', U'╱', U'╲'};
        const int radius = reduce_flicker ? 4 : 7;
        for (int i = 0; i < 8; ++i) {
            const int dx = ((static_cast<int>(frame_index) * 5 + i * 7) %
                            (radius * 2 + 1)) - radius;
            const int dy = ((static_cast<int>(frame_index) * 3 + i * 11) %
                            (radius * 2 + 1)) - radius;
            SetEffectCell(cells, cell_w, cell_h, cx + dx, cy + dy,
                          glyphs[i % 5], {244, 148, 62}, {45, 24, 18});
        }
    }
    if (shot_flash || explosion) {
        const int max_shift = reduce_shake ? 1 : 2;
        const int dx = static_cast<int>((frame_index * 13) %
                                        static_cast<uint64_t>(max_shift * 2 + 1)) -
                       max_shift;
        const int dy = static_cast<int>((frame_index * 7) %
                                        static_cast<uint64_t>(max_shift * 2 + 1)) -
                       max_shift;
        if (dx != 0 || dy != 0) {
            const std::vector<CharCell> before(
                cells, cells + static_cast<size_t>(cell_w) * cell_h);
            for (int y = 0; y < cell_h; ++y) {
                for (int x = 0; x < cell_w; ++x) {
                    const int sx = std::clamp(x - dx, 0, cell_w - 1);
                    const int sy = std::clamp(y - dy, 0, cell_h - 1);
                    cells[static_cast<size_t>(y) * cell_w + x] =
                        before[static_cast<size_t>(sy) * cell_w + sx];
                }
            }
        }
    }
}

bool WriteCharacterFrameSvg(const CharCell* cells, int cell_w, int cell_h,
                            const std::string& path) {
    if (cells == nullptr || cell_w <= 0 || cell_h <= 0 || path.empty()) return false;
    const std::filesystem::path output(path);
    std::error_code ec;
    if (output.has_parent_path()) std::filesystem::create_directories(
        output.parent_path(), ec);
    if (ec) return false;
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    constexpr int kCellWidthPx = 10;
    constexpr int kCellHeightPx = 20;
    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""
         << cell_w * kCellWidthPx << "\" height=\"" << cell_h * kCellHeightPx
         << "\" viewBox=\"0 0 " << cell_w * kCellWidthPx << ' '
         << cell_h * kCellHeightPx << "\">\n";
    file << "<rect width=\"100%\" height=\"100%\" fill=\"#05090e\"/>\n";
    file << "<g font-family=\"Cascadia Mono,Consolas,DejaVu Sans Mono,monospace\""
         << " font-size=\"17\" dominant-baseline=\"alphabetic\""
         << " textLength=\"9\" lengthAdjust=\"spacingAndGlyphs\">\n";
    for (int y = 0; y < cell_h; ++y) {
        for (int x = 0; x < cell_w; ++x) {
            const CharCell& cell = cells[static_cast<size_t>(y) * cell_w + x];
            file << "<rect x=\"" << x * kCellWidthPx << "\" y=\""
                 << y * kCellHeightPx << "\" width=\"" << kCellWidthPx
                 << "\" height=\"" << kCellHeightPx << "\" fill=\"#"
                 << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(cell.bg_r) << std::setw(2)
                 << static_cast<int>(cell.bg_g) << std::setw(2)
                 << static_cast<int>(cell.bg_b) << std::dec << "\"/>";
            if (cell.code_point != U' ' && IsXmlSafe(cell.code_point)) {
                const std::string utf8 = XmlEscape(CharCellToUtf8(cell.code_point));
                file << "<text x=\"" << x * kCellWidthPx << "\" y=\""
                     << (y + 1) * kCellHeightPx - 2 << "\" fill=\"#"
                     << std::hex << std::setw(2) << std::setfill('0')
                     << static_cast<int>(cell.fg_r) << std::setw(2)
                     << static_cast<int>(cell.fg_g) << std::setw(2)
                     << static_cast<int>(cell.fg_b) << std::dec << "\">"
                     << utf8 << "</text>";
            }
        }
        file << '\n';
    }
    file << "</g>\n</svg>\n";
    return file.good();
}

} // namespace writeover
