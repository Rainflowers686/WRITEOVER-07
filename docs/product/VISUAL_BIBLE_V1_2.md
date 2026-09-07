# VISUAL BIBLE v1.2 — WRITEOVER-07 Character-Art Constitution

Status: active production contract for the Character Reboot-01 M2 work. The
former PVS-01 half-block presentation is a historical implementation
milestone, not the normal-world visual direction. This file contains no
unresolved TBD or maybe decisions.

## 1. Product identity and non-negotiable law

WRITEOVER-07 is a real-time first-person game whose world is rendered as
ANSI/Unicode character art. The player reads space from glyph silhouette,
negative space, line rhythm and authored character strokes. ANSI TrueColor
adds material, light and faction accents; it does not substitute for the
glyph layer.

The normal world path produces `CharCell[width * height]` directly:

`ray geometry -> character surface resolver -> material shading -> authored
sprite projection -> character weapon/effects -> HUD/subtitle -> CharCell grid
-> ANSI terminal backend`.

The normal world must not use a logical RGB framebuffer, a `Color[width *
height * 2]` intermediate, or `▀` half-block packing. The legacy
`production_renderer.*` implementation remains compiled only for historical
regression tests and is not wired from `composition_root.cpp`.

## 2. Terminal canvas and projection

- Target presentation: 240 columns x 67 rows at the ULTRA terminal size.
- Every world cell has a single-width glyph, foreground colour, background
  colour and style flags. Spaces are intentional negative space, not missing
  textures.
- World coordinates use x right, y forward in the map plane, and z up.
  Screen x increases rightward and screen y increases downward.
- Terminal cell aspect is frozen at `0.5` horizontal cell width per vertical
  cell height for horizontal projection. Unicode wide or ambiguous glyphs are
  excluded from the 3D layer; the HUD follows the existing UTF-8 compositor
  rules.
- Vertical FOV is 60 degrees. Horizontal FOV is derived from the cell aspect
  and focal length; at 240x67 it is approximately 92 degrees. It is not a
  second hard-coded 90-degree path.
- The horizon is the projected camera pitch. The floor occupies rows below
  the horizon and the ceiling rows above it. Height-Span ray geometry remains
  the occlusion and projection authority.
- Native ANSI terminals receive SGR TrueColor (`38;2;r;g;b` and
  `48;2;r;g;b`) where capability probing permits it. The backend has a
  deterministic fallback for terminals without VT support.

## 3. Character materials

Materials are authored glyph vocabularies with a restrained shade palette.
The current B1 vocabulary is intentionally sparse and curated:

| Material | Character vocabulary and use |
|---|---|
| Metal panel | `║`, `═`, `╫`, `·`; vertical/horizontal seams and occasional rivets |
| Glass | `│`, `─`, `┼`, `╱`; reflective panes and sparse diagonal glints |
| Grate | `╬`, `╫`, `┼`, `+`; repeated structural intersections |
| Hazard | `╱`, `╲`; warning stripe direction, not a filled colour block |
| Concrete / wood | `┼`, `│`, `─`, `·`; low-frequency seams and restrained surface marks |

The resolver chooses glyph density from distance, surface orientation, light,
material state and panel rhythm. Large quiet areas are deliberate negative
space. It must not emit random high-frequency ASCII snow, repeated texture
noise, or a material that is visually only a coloured rectangle.

Foreground colours are material accents, not the geometry. Wall backgrounds
stay near the dark facility base; bright colour is reserved for localized
emissive marks, authored strokes and interaction feedback.

## 4. Floor and ceiling

The floor and ceiling are semantic planes, not flat coloured fills:

- Floor seams follow the projected world axes and repeat on a two-metre major
  rhythm. Major intersections use `┼`, with `│` / `─` only on aligned seams.
- Ceiling beams use the same world-aligned rhythm at lower contrast. The
  ceiling is quieter than the floor so the playable route remains legible.
- Near distance may add a few `·` anchor marks; mid and far distance remove
  them. No random checkerboard or per-cell noise is allowed.
- Normal floor background luminance is dark, with RGB channel maxima normally
  no higher than 36. Ceiling maxima are normally no higher than 22. Wall
  stroke maxima are normally no higher than 170, and large-area world
  brightness is capped below the emissive range.
- No large-area light-grey or white floor is permitted. Emissive/highlight
  colour may exceed 200 only on small glyph groups such as a terminal cursor,
  muzzle flash or warning mark.
- Perspective is carried by seam convergence, the horizon boundary, surface
  height spans and distance-dependent glyph density together. Colour alone
  must never be the only floor/wall cue.

## 5. Depth policy

Distance changes both detail and intensity:

- Near: less than 4m; detailed authored strokes, explicit equipment marks and
  full material seam vocabulary.
- Mid: 4m inclusive to less than 12m; independently authored body/equipment
  sprite and reduced surface detail.
- Far: 12m and beyond; independently authored silhouette/occupation cue and
  sparse surface rhythm.

World sprites are capped at 96, 48 and 24 terminal cells tall for near, mid
and far respectively. The resolver uses projected depth for scale and the
same ray geometry for wall occlusion. It does not scale a single noisy sprite
for every distance.

## 6. Authored character and prop art

The lightweight B1 authoring file is
`data/characters/b1_character_art.txt`. It is plain UTF-8 text compiled into
bounded runtime data by the CharacterArtBank; it is not a new general asset
engine. The runtime bounds are 64 assets, 32 rows and 64 single-width columns
per asset.

- Each important actor/prop has separate far, mid and near entries where a
  visual cue changes with distance.
- Spaces are transparent in world sprites. An empty sprite cell never writes
  a black billboard over the room.
- Security cues are helmet/visor/mask, shoulder protection, radio, badge and
  weapon silhouette. Clothing and posture carry role identity.
- Full-Human face priority is eyes, eye direction, brow shape, hair/head
  silhouette, jaw and shadow, followed by a restrained mouth. Eyes may be
  visibly larger for expression. Noses are reduced or omitted and nostrils
  are never drawn. The face must not become an emoji or comedy/anime mask.
- Dialogue/inspect presentation has a bounded Full-Human portrait candidate:
  a framed near-detail CharCell overlay using the same authored asset bank.
  It is available to a future dialogue trigger without adding a dialogue
  engine or changing module ownership.

## 7. B1 Wake golden scene

The first production art target is the small B1 Wake / Revival bay. Its
composition must contain a readable wall, floor, ceiling, marked service door,
terminal, at least one security or semi-human cue, a Full-Human cue when the
scene population places one in view, pistol viewmodel, lighting, subtitle and
the normal player HUD. The scene is a bounded evidence target; it does not
expand the 41-floor content scope.

The player should understand “a first-person character-art room” before
reading any documentation. Props are line/silhouette compositions, not
procedural coloured rectangles. Door panels, cameras, terminals and crates
use the same material and transparency rules as actors.

## 8. Pistol character viewmodel

The first weapon is an authored CharCell pistol with four bounded frames:

- `IdleA` and `IdleB`: two stable silhouettes with a clear barrel, slide,
  grip and hand/anchor negative space.
- `Fire`: one key frame with a `====>` / `═` barrel silhouette and a short
  `*` / `✦` muzzle glyph effect.
- `Reload`: one key frame with the weapon lowered and a changed magazine/grip
  silhouette.

The weapon is anchored to the bottom-right safe region: 12 cells from the
right edge and 6 rows from the bottom at the 240x67 canvas. It occupies no
more than 30% of the width or 34% of the height. Recoil is a two-cell
maximum vertical presentation offset in the character path; gameplay recoil
remains owned by the player/combat foundation.

## 9. Character-first effects and game feel

Effects communicate through glyphs first and colour second:

- Gunfire: recoil, a localized muzzle flash, `*`/`✦`/`╳` sparks and hit
  feedback. The gun remains identifiable during the effect.
- Explosion: a bounded flash, visible `✦`/`╳`/`*` burst, a short debris/smoke
  glyph phase and audio event. Normal shake is a maximum two-cell equivalent
  displacement for 120ms; reduced-shake is one-cell equivalent for 60ms.
- Muzzle flash lasts at most 4 simulation frames at the 120Hz presentation
  cadence. Hit feedback lasts at most 6 frames; the explosion glyph phase at
  most 10 frames.
- Sprint adds at most one-cell vertical head motion at 8Hz. It must not
  destroy the horizon or subtitle readability.
- Flash, shake and glitch are localized and bounded. Reduced Flicker removes
  rapid alternation, and Reduced Camera Shake removes the displacement, but
  neither setting removes the event glyph or its narrative information.

The foundation benchmark must treat a 240x67 CharCell frame with walls,
sprites, viewmodel, effects and ANSI encoding as the render proxy. The target
budgets are simulation p99 <= 2ms, render p99 <= 3ms, terminal encode p99 <=
1ms and total measured CPU p99 <= 6ms. These are performance gates, not a
claim that a future full 41-floor scene has already been profiled.

## 10. HUD, subtitle and developer information

Normal player presentation contains only:

- health;
- ammunition and weapon name;
- a small interaction/crosshair cue;
- objective or alert information when active;
- subtitle in the bottom safe zone.

At 240x67, the normal HUD safe margin is 2 cells from the edges and the
subtitle safe zone is the bottom 6 rows, centered within 80% of the width.
`PRESET`, grid dimensions, positions, yaw and other engineering diagnostics
are available only behind the explicit F3 developer overlay. They must never
appear in the normal player frame.

## 11. Narrator typography

Ordinary narrator delivery uses ear-channel state, a bottom/center subtitle
and deterministic typewriter progression. Strong Narrator Authority may use
full-screen typography because it is typography, not normal world rendering:

- a bounded Block Font Atlas composes large block glyphs;
- the text travels diagonally from lower-left toward upper-right and may
  occupy up to 70% of the terminal canvas;
- reveal is typed by glyph block, with a bounded shake/glitch accent;
- the world dims but remains a world layer behind the typography;
- effect duration is at most 1800ms and audio ducking target is -12dB where
  the platform audio backend supports it;
- Reduced Flicker uses stable colour/inversion rather than rapid flashes;
  Reduced Camera Shake removes displacement. The full text remains readable
  in both modes.

The Save-interruption line is a narrative input/state decision. The renderer
only owns the visual typography contract and does not hard-code a new gameplay
rule around it.

## 12. Accessibility and terminal safety

- All important information is present in glyph/text form; colour is never
  the sole state signal.
- Reduced Flicker and Reduced Camera Shake are honoured by normal effects and
  narrator typography.
- World glyphs are single-width and UTF-8 encoded deterministically. Wide
  Unicode is reserved for the HUD/text compositor.
- ANSI TrueColor is progressive enhancement. A terminal without TrueColor
  still receives the authored glyph/silhouette language through its fallback
  colour path.

## 13. Evidence and acceptance boundary

C01 through C06 must be generated by the Windows Release production
executable and the final CharCell buffer. An SVG/PNG conversion is only a
reviewable visualization of that real runtime frame; it is not a mockup or a
dedicated screenshot renderer. The evidence set is:

`docs/production/evidence/character_reboot/`

with `before_current_release.png`, `C01_B1_WAKE.png`, `C02_NPC_NEAR.png`,
`C03_NPC_MID.png`, `C04_NPC_FAR.png`, `C05_PISTOL_IDLE.png` and
`C06_PISTOL_FIRE.png`, plus source frame dumps when useful.

Automated tests and performance gates cannot self-certify product Gold.
Visual acceptance remains a Rain decision after inspecting the real Release
evidence. No new public release is authorized by this visual contract.
