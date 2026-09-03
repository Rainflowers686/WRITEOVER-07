# VISUAL BIBLE v1.2 — WRITEOVER-07

> Freeze for the Half-block TrueColor Pixel Framebuffer direction.

## 1. Core Framebuffer

- Terminal cell glyph: `▀` (U+2580)
- Foreground RGB = upper logical pixel
- Background RGB = lower logical pixel
- ULTRA target: **240×67 terminal cells = 240×134 logical pixels**
- Normal world is a pixel FPS
- Character/text corruption is reserved for Narrator intrusion, glitch, HUD,
  and terminals

## 2. Rejected Reference Visual

The current bright/light-gray `ReferenceRenderer` floor is explicitly
**rejected** as final art direction.

It remains only as:

- technical reference
- test raster
- calibration for rays/depth

## 3. Color Policy

Large-area world colors must not use pure `255,255,255` white.

White is allowed only for:

- flash
- glitch
- highlight
- very short duration
- very small area

## 4. Material Atlas

Materials are authored in a small atlas with distinct half-block patterns and
palettes:

| Material | Language |
|----------|----------|
| Metal | cold desaturated blue/gray, subtle vertical striations |
| Concrete | warm dark gray, noise speckle |
| Glass | lower alpha-like cyan, edge highlight |
| Dirt/grime | brown-black mottling |
| Wood | warm brown, grain line pairs |
| Grate | black gaps on dark steel |
| Hazard | yellow/black diagonal, low-frequency |

## 5. Wall / Floor / Ceiling Language

- Walls: upper/lower pair chosen to imply vertical extent; not flat white
- Floors: darker, warmer, low contrast speckle
- Ceilings: near-black with occasional panel seams
- Doors: readable color-coded frame; no text dump unless terminal context

## 6. Sprite Atlas & NPC Sprites

- NPC sprites are small half-block pixel sprites, not bitmap photographs
- Each identity-bearing NPC has a silhouette palette tag:
  - Security: blue-gray with reflective accent
  - Medical: red/cross accent
  - Research: teal/white lab accent
  - Maintenance: orange/dark accent
  - Staff/civilian: muted neutral
- Sprite animation is 2–4 frame arcade-style; no full skeletal animation

## 7. Weapon Viewmodels

- 2D half-block viewmodel in lower-right
- Muzzle flash is a short 1–3 frame additive bright burst
- Recoil is a few logical-pixel displacement, not full-screen shake
- Weapon hand-feel is more important than photoreal detail

## 8. Lighting

- Cell light is a luminance multiplier
- No real-time global illumination
- Local lights use small radius falloff
- Disabling power reduces lights to emergency/backup palette
- Smoke/short-circuit can locally reduce visibility

## 9. Effects

- Muzzle flash: white/yellow, 1–3 frames
- Spark: orange/white small particles, short lifespan
- Explosion: limited flash + smoke zone, not giant fire sim
- Smoke: dark gray translucent-looking half-block plume; temporary visibility
  reduction
- Camera shake: small, localized, bounded
- Head bob: subtle, suppressed when aiming/sprinting?; defined by M3
- Recoil: weapon viewmodel + camera pitch impulse

## 10. Narrator Intrusion Typography

- Normal text is terminal monospace
- Narrator intrusion uses corrupted glyphs, inverted cells, scanline flicker
- Reserved for glitch/HUD/terminal/narrative moments
- Not applied to normal world rendering

## 11. HUD

- Minimal, diegetic where possible
- Ammo/health/status, objective only when relevant
- F3 may show systemic raw values (developer/debug)
- No "STEALTH +10" feedback; consequences are shown through world state

## 12. Subtitle

- Bottom band, high contrast, max 2 lines
- No pure white full-width backgrounds; dark translucent backing preferred
- Accessibility: subtitle toggle and size scaling are not blocked by Narrator
  presentation authority

## 13. Accessibility

- High-contrast mode
- Colorblind-friendly critical markers (shape + color, not color alone)
- Subtitle on/off and size
- Reduced camera shake option
- Terminal text size option

These settings are protected from permanent destructive changes.
