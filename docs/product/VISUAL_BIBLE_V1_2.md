# VISUAL BIBLE v1.2 — WRITEOVER-07

Production contract for M2. No TBD / maybe / defined later in this file.

## 1. Framebuffer

- Half-block cells: `▀`
- ULTRA: 240x67 cells = 240x134 logical pixels
- Normal world is pixel FPS.
- Text/corruption is for Narrator/HUD/terminal only.

## 2. Coordinate Convention

- Logical pixel coordinates: x right, y down.
- World x/y maps to screen horizontal/vertical with fixed aspect.
- World up is +Z; projection convention is vertical screen y from top.

## 3. Aspect Correction

- Terminal cells are not square.
- Logical pixel aspect ratio is fixed at 1:1 after half-block split.
- Render must scale world-projected x by `aspect_correction = 0.5` relative to y when composing half-block cells.
- Horizontal FOV: 60 degrees vertical / 90 degrees horizontal at ULTRA default.
- Vertical projection: standard centered perspective, y=0 top, y=logical_height-1 bottom.

## 4. Textures / Atlases

- Material atlas logical size: 64x64 half-block pixels per material tile.
- Wall U/V: u across wall face, v down the wall.
- Floor/ceiling sampling: nearest-neighbor, v aligned to world axes.
- Orientation shading: +x, +y, +z faces use precomputed face shade multipliers.
- Distance shading/fog: exponential fog with near 0m, far 40m.
- Lighting is multiplicative on texture RGB.

## 5. Sprites

- Sprite projection: billboard, centered at logical pixel position.
- Sprite occlusion: same depth buffer as walls; if a wall sample is closer, sprite is hidden.
- Sprite LOD: 3 tiers (near/medium/far).
- Sprite screen-size rule: max 96 logical px tall near, 48 medium, 24 far.

## 6. Weapon Viewmodel

- Anchor: bottom-right, 24 logical pixels from right edge, 12 from bottom.
- Max screen occupation: 22% width, 30% height.
- Animation frame guidance: 3-frame recoil, 2-frame idle, 4-frame reload.

## 7. HUD / Subtitle Safe Zones

- HUD safe zone: 2 logical pixels from each edge at ULTRA.
- Subtitle safe zone: bottom 6 logical pixels, centered, max 80% width.

## 8. Effects Budget

- Particle budget: 512 logical particles max.
- Screen effect budget: no more than 2 full-screen effects simultaneously.
- Muzzle flash: 1-3 frames.
- Spark: max 8 particles per impact, 12 frames.
- Explosion: 1 flash frame + smoke plume of max 40 particles.
- Smoke: max 60 logical particles per hazard zone.

## 9. Camera

- Normal camera shake: max 2 logical pixels displacement, max 120ms.
- Reduced shake: max 1 logical pixel, max 60ms.
- Flash duration: max 2 frames.
- Recoil range: max 6 logical pixels up + 2 pixels random horizontal.
- Damage kick: max 4 logical pixels, max 150ms.
- Sprint bob: max 1 logical pixel vertical, 8Hz.

## 10. Luminance Bands

- Ceiling: very dark, RGB max <= 90.
- Floor: dark, RGB max <= 120.
- Normal wall: mid-dark/mid, RGB max <= 170.
- Emissive: high but localized, RGB max can exceed 200 only in small areas.
- Large-area world luminance: strictly bounded by the above.
- Pure white (255,255,255): only transient/small-area effects.

Current ReferenceRenderer bright floor: REJECTED.

## 11. Narrator Intrusion Typography

### Normal Narrator

- Earpiece
- Subtitle
- Typewriter progression

### Major Authority Intervention

- Full-screen typography using BlockFontAtlas.
- Large glyph composition from atlas.
- Diagonal layout.
- Screen occupation: up to 70%.
- Typing/block progression: staggered block reveal.
- Shake: bounded 3 logical pixels.
- Glitch: 2-frame inversion/color shift.
- Color: black background + white/grey block glyphs or red warning accent.
- World freeze/visibility: world dims to 20% luminance.
- Duration: max 1800ms.
- Audio ducking: -12dB.

Reference interaction: player selects Save -> brief impulse -> world/audio response -> giant diagonal text. This is a product reference, not hard-coded renderer text.

### Accessibility

- ReduceShake: lower camera displacement, giant typography is not cancelled.
- ReduceFlicker: no high-speed flash; use stable inversion/block/color shift.
- Narrative information must remain readable.