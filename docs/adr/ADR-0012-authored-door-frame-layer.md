# ADR-0012: Authored door-frame layer

- 状态: 已实现，待课程交付 owner 在审计时确认
- 日期: 2026-09-14
- 决策者: Rainflowers686
- 受影响模块: M2 Character-Art / M3 scene content / production renderer

## 背景

The visual acceptance pass showed that a door panel alone could read as a
floating prop.  The revised contract requires a wall-supported opening with a
frame, panel, inset or observation detail, and a stable depth relationship.
The existing public character-kind enum had no authored frame layer, so the
scene could not express that relationship without overloading a gameplay kind.

## 决策

Add the single public enum value `CharacterSpriteKind::DoorFrame`.  A scene
may place a `door_frame` entity immediately before its matching `door` panel.
Both use the existing Character-Art bank and `DrawDoorPlane`; the frame is
allowed to share the panel's depth, but a nearer wall, prop, or actor still
occludes it.  The frame has no interaction, systemic id, or gameplay state.

This is a bounded authored presentation layer.  It does not add a new
renderer, projection mode, framebuffer, map schema, save field, or animation
system.

## 备选方案

- Reuse `Door` for both layers and infer frame status from scene ordering.  Rejected:
  it couples visual layering to incidental array order and makes the asset
  contract ambiguous.
- Draw a camera-facing decorative rectangle in the HUD.  Rejected:
  it breaks wall anchoring, depth, and the Character-Art visual contract.
- Rebuild the room renderer.  Rejected:
  the problem is an authored asset/layer distinction, not a structural
  renderer limitation.

## 影响

The public header hash changes and is recorded in the contract snapshot.  The
existing art review and public-header tests continue to cover the enum and
asset path.  Door frames are visual-only, so save/load, determinism, AI,
interaction authorization, and package schemas are unchanged.  The extra
layer adds a bounded number of authored cells per door and remains inside the
existing production render budget.

## 回滚/回退

Remove the `DoorFrame` enum, its parser/fallback entry, frame entities and
frame asset, then restore the previous header snapshot.  Do not remove the
wall-supported panel or alter interaction geometry as part of that rollback.
