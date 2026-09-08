# ADR-0011: Spatial truth hard gate

- 状态: 已实现，待真实前台空间证据与 Rain 验收
- 日期: 2026-09-08
- 决策者: Rainflowers686
- 范围: M2 Character-Art world presentation and bounded B1 interaction

## 决策

- Height-Span walls, floor/ceiling sampling, character sprites, and B1
  interaction use one `CameraProjection` pinhole contract.  The terminal cell
  aspect is part of that contract; it is not an independent sprite scale.
- Runtime actor yaw is carried into `CharacterSpriteInstance` and resolves
  front, back, and side authored presentation from the camera's world
  position.  Incapacitated bodies use separate floor-presented body poses.
- Character composition uses a bounded per-terminal-cell depth buffer.  An
  authored cell is `Transparent`, `Glyph`, or `OpaqueEmpty`; only the latter
  two participate in character occlusion, so negative space remains real.
- B1 `F` interaction casts the center camera ray with yaw and pitch, selects
  the nearest intersected target, and applies the existing world line-of-sight
  check.  The former DoorReader no-sight exception is removed.

## Consequences

- Close-range caps remain the existing data contract; anchoring and depth are
  derived from the same effective projection scale.
- Dynamic player/NPC collision and systemic body authority remain owned by
  their existing modules.  This ADR adds no ECS, generic interaction system,
  new renderer, map, weapon, or gameplay scope.
- The spatial tests are geometry and character-cell assertions, not a claim
  that automated tests replace real Windows Terminal evidence.  The required
  yaw/orbit, partial-occlusion, look-up/down, and body-presentation evidence
  remains a separate release-runtime gate.

## Reversion boundary

Revert this ADR together with the focused projection, character-cell, body
presentation, B1 interaction, and spatial-test changes.  Do not use
destructive Git operations, alter the historical release tag, or restore the
legacy half-block world path.
