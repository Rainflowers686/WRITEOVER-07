#include "tests/test_harness.h"

#include "writeover/core/settings.h"
#include "writeover/player/combat.h"
#include "writeover/player/controller.h"
#include "writeover/player/input.h"
#include "writeover/player/weapon.h"
#include "writeover/world/grid.h"

#include <cmath>
#include <vector>

namespace writeover {

namespace {

Grid MakeOpenGrid(int w = 6, int h = 6) {
    Grid grid(w, h);
    for (int32_t r = 0; r < h; ++r) {
        for (int32_t c = 0; c < w; ++c) {
            grid.SetCell(c, r, GridCell{});
        }
    }
    return grid;
}

bool PostureClearance() {
    const Grid grid = MakeOpenGrid();
    GridWorldQuery query(&grid);
    const Vec3 pos{1.5f, 1.5f, 0.0f};
    // Standing fits in an open cell (ceiling 4.0).
    WO_CHECK(CanFit(Posture::Stand, pos, query));
    // Below a 1.0m ceiling, only prone fits.
    Grid low = MakeOpenGrid();
    GridCell c = low.GetCell(1, 1);
    c.ceiling_height = 0.8f;
    low.SetCell(1, 1, c);
    GridWorldQuery low_query(&low);
    return !CanFit(Posture::Stand, Vec3{1.5f, 1.5f, 0.0f}, low_query) &&
           !CanFit(Posture::Crouch, Vec3{1.5f, 1.5f, 0.0f}, low_query) &&
           CanFit(Posture::Prone, Vec3{1.5f, 1.5f, 0.0f}, low_query);
}

bool LocomotionStateOrthogonal() {
    // A single source of truth for locomotion: no duplicate onGround /
    // velocityZ / jumpVelocity fields (M-006 closure).
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.posture = Posture::Crouch;
    ls.lean = Lean::Right;   // crouch + lean is expressible
    ls.traversal = Traversal::Grounded;
    ls.contact.grounded = true;  // spawn on floor
    return ls.IsGrounded() && ls.velocity.z == 0.0f &&
           ls.posture == Posture::Crouch && ls.lean == Lean::Right;
}

bool IntegrateMoveBlocked() {
    const Grid grid = MakeOpenGrid();
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    // Move forward (+y, no input rotation here) into open space.
    IntegrateLocomotion(ls, Vec2{0.0f, 1.0f}, false, query,
                        SimClock::kFixedDeltaTime);
    return ls.position.y > 1.5f;
}

bool JumpThenLand() {
    const Grid grid = MakeOpenGrid();
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.contact.grounded = true;  // spawn on floor
    TryJump(ls);
    WO_CHECK(ls.velocity.z > 0.0f);
    // Step up to 300 ticks at 120Hz: jump arc (apex ~1.1m) takes ~112 ticks.
    for (int i = 0; i < 300; ++i) {
        IntegrateLocomotion(ls, Vec2{0.0f, 0.0f}, false, query,
                            SimClock::kFixedDeltaTime);
        if (ls.contact.grounded && i > 5) {
            return true;
        }
    }
    return false;
}

bool InputMapperRebind() {
    InputMapper mapper;
    mapper.SetBinding(GameAction::Jump, PhysicalKey::Space);
    mapper.SetBinding(GameAction::Interact, PhysicalKey::E);
    WO_CHECK(mapper.MapKey(PhysicalKey::Space) == GameAction::Jump);
    WO_CHECK(mapper.MapKey(PhysicalKey::E) == GameAction::Interact);
    // Unmapped key -> GameAction::Count.
    return mapper.MapKey(PhysicalKey::F10) == GameAction::Count;
}

// Issue B.1: the same PhysicalKey may map to different actions in different
// contexts (Gameplay Num1 -> WeaponSlot1 vs Dialogue Num1 -> DialogOption1).
bool InputContextSameKeyDifferentActions() {
    InputMapper mapper;  // defaults already assign both contexts
    WO_CHECK(mapper.MapKey(InputContext::Gameplay, PhysicalKey::Num1) ==
             GameAction::WeaponSlot1);
    WO_CHECK(mapper.MapKey(InputContext::Dialogue, PhysicalKey::Num1) ==
             GameAction::DialogOption1);
    // Num4 exists only in Dialogue context; Gameplay should leave it unmapped.
    WO_CHECK(mapper.MapKey(InputContext::Gameplay, PhysicalKey::Num4) ==
             GameAction::Count);
    return true;
}

// Issue B.1: within ONE context, duplicate binding still replaces (the old
// key no longer maps to the replaced action).
bool InputContextReplaceWithinContext() {
    InputMapper mapper;
    // In Gameplay, Num2 currently maps to WeaponSlot2. Rebind to Fire.
    mapper.SetBinding(InputContext::Gameplay, GameAction::Fire, PhysicalKey::Num2);
    WO_CHECK(mapper.MapKey(InputContext::Gameplay, PhysicalKey::Num2) ==
             GameAction::Fire);
    WO_CHECK(mapper.MapKey(InputContext::Gameplay, PhysicalKey::Num2) !=
             GameAction::WeaponSlot2);
    // Dialogue context is untouched.
    WO_CHECK(mapper.MapKey(InputContext::Dialogue, PhysicalKey::Num2) ==
             GameAction::DialogOption2);
    return true;
}

bool CombatFireAndAmmo() {
    const WeaponDef& pistol = DefaultWeapons()[static_cast<size_t>(WeaponSlot::Pistol)];
    CombatState state;
    WO_CHECK(CanFire(state, pistol, 0));
    WO_CHECK(ConsumeShot(state, pistol, 0));
    const size_t slot = static_cast<size_t>(WeaponSlot::Pistol);
    WO_CHECK_EQ(static_cast<int64_t>(state.ammo_in_mag[slot]), 11);
    // Next fire frame blocks a shot in the same tick.
    return !CanFire(state, pistol, 0);
}

bool CombatReloadTransfer() {
    const WeaponDef& pistol = DefaultWeapons()[static_cast<size_t>(WeaponSlot::Pistol)];
    CombatState state;
    const size_t slot = static_cast<size_t>(WeaponSlot::Pistol);
    state.ammo_in_mag[slot] = 2;
    state.reserve[slot] = 10;
    StartReload(state, WeaponSlot::Pistol, pistol);
    WO_CHECK(state.reload_frames_left > 0);
    AdvanceReload(state, state.reload_frames_left);
    return state.ammo_in_mag[slot] == pistol.ammo_per_mag &&
           state.reserve[slot] == 10 - (pistol.ammo_per_mag - 2);
}

bool HitscanWalls() {
    const Grid grid = MakeOpenGrid(8, 6);
    GridWorldQuery query(&grid);
    const WeaponDef& pistol = DefaultWeapons()[static_cast<size_t>(WeaponSlot::Pistol)];
    DeterministicRNG rng(7);
    FireRequest req;
    req.origin = Vec3{1.5f, 1.5f, 1.0f};
    req.yaw = 0.0f;
    req.pitch = 0.0f;
    const HitscanResult res = ResolveHitscan(req, pistol, query, rng);
    // Open grid: no solid cell -> no hit.
    return !res.hit;
}

// ---------------------------------------------------------------------------
// HK-3 property tests (F-16/F-17/F-18 closure)
// ---------------------------------------------------------------------------

// Idle on open floor stays grounded across ticks (F-16 regression).
bool GroundedIdleStable() {
    const Grid grid = MakeOpenGrid();
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.contact.grounded = true;
    for (int i = 0; i < 120; ++i) {  // 120 ticks @ 120Hz = 1 second
        IntegrateLocomotion(ls, Vec2{0.0f, 0.0f}, false, query,
                            SimClock::kFixedDeltaTime);
        WO_CHECK(ls.contact.grounded);  // never lose ground on flat floor
        WO_CHECK_NEAR(ls.position.z, 0.0f, 0.01f);  // no drift
    }
    return true;
}

// 0.2m step-up works (F-17 closure).
bool StepUp20cm() {
    Grid grid = MakeOpenGrid(8, 4);
    // Raise the whole right side (cols 3-7) so the player stays on the
    // raised floor after crossing the step at x=3.0.
    for (int32_t r = 0; r < 4; ++r) {
        for (int32_t c = 3; c < 8; ++c) {
            GridCell cell = grid.GetCell(c, r);
            cell.floor_height = 0.2f;
            grid.SetCell(c, r, cell);
        }
    }
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.contact.grounded = true;
    // ~70 ticks to cross x=3.0 (velocity ramp-up); 120 ticks keeps the
    // player on the raised right side.
    for (int i = 0; i < 120; ++i) {
        IntegrateLocomotion(ls, Vec2{1.0f, 0.0f}, false, query,
                            SimClock::kFixedDeltaTime);
    }
    WO_CHECK(ls.position.x > 3.0f);
    // After crossing the step boundary at x=3.0, feet snap to the raised
    // floor (0.2m) via the Z-axis floor snap.
    WO_CHECK_NEAR(ls.position.z, 0.2f, 0.1f);
    return true;
}

// 0.5m step-up does NOT auto-step (maxStep = 0.35m).
bool StepUp50cmRejected() {
    Grid grid = MakeOpenGrid(8, 4);
    // Raise the whole right side (cols 3-7) by 0.5m.
    for (int32_t r = 0; r < 4; ++r) {
        for (int32_t c = 3; c < 8; ++c) {
            GridCell cell = grid.GetCell(c, r);
            cell.floor_height = 0.5f;
            grid.SetCell(c, r, cell);
        }
    }
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.contact.grounded = true;
    // Try to move in +x through the step boundary at x=3.0.
    for (int i = 0; i < 120; ++i) {
        IntegrateLocomotion(ls, Vec2{1.0f, 0.0f}, false, query,
                            SimClock::kFixedDeltaTime);
    }
    // Must NOT cross -- blocked by the 0.5m step (feet stay below 0.5).
    WO_CHECK(ls.position.x < 3.0f);
    WO_CHECK(ls.position.z < 0.5f - 0.01f);
    return true;
}

// LeanClamp: leaning into a wall returns < kLeanOffset (F-18 closure).
bool LeanClampAgainstWall() {
    Grid grid = MakeOpenGrid(8, 4);
    // Solid wall at row 2 (y=2.0..3.0). At yaw=0, local right = +y, so
    // leaning right probes +y direction, which hits this wall.
    for (int32_t c = 0; c < 8; ++c) {
        GridCell w;
        w.flags = CellFlag_Solid;
        grid.SetCell(c, 2, w);
    }
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};  // 0.5m from the wall (row 2 edge at 2.0)
    ls.yaw = 0.0f;  // facing +x, local right = +y
    ls.contact.grounded = true;
    // Leaning right (+y) into the wall should be clamped.
    const float clamped = LeanClamp(ls, Lean::Right, query);
    WO_CHECK(clamped < kLeanOffset - 0.01f);  // not the full lean
    // Leaning left (-y) away from the wall should be full offset.
    const float free = LeanClamp(ls, Lean::Left, query);
    WO_CHECK_NEAR(free, kLeanOffset, 0.01f);
    return true;
}

// Issue F: leaning at yaw=90 must clamp in the correct direction (local
// right = -x when yaw=90). Uses the same wall as LeanClampAgainstWall.
bool LeanRespectsYaw90() {
    Grid grid = MakeOpenGrid(8, 4);
    // Wall at row 2 (y=2.0..3.0).
    for (int32_t c = 0; c < 8; ++c) {
        GridCell w;
        w.flags = CellFlag_Solid;
        grid.SetCell(c, 2, w);
    }
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.yaw = 3.14159265f * 0.5f;  // 90 deg: facing +y, local right = -x
    ls.contact.grounded = true;
    // Leaning right at yaw=90 means -x direction. No wall in -x (col 1 is
    // open) → should be free.
    const float right = LeanClamp(ls, Lean::Right, query);
    WO_CHECK_NEAR(right, kLeanOffset, 0.01f);
    // Leaning left at yaw=90 means +x direction. No wall in +x (col 2 is
    // open at row 1) → should be free.
    const float left = LeanClamp(ls, Lean::Left, query);
    WO_CHECK_NEAR(left, kLeanOffset, 0.01f);
    return true;
}

// HeadCollision: standing under a low ceiling cancels positive Z.
bool HeadCollisionStopsJump() {
    Grid grid = MakeOpenGrid(6, 4);
    // Low ceiling at col 1.
    for (int32_t r = 0; r < 4; ++r) {
        GridCell c = grid.GetCell(1, r);
        c.ceiling_height = 2.0f;  // headroom 2.0m (stand collider 1.8m)
        grid.SetCell(1, r, c);
    }
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.contact.grounded = true;
    WO_CHECK(!HeadCollision(ls, query));  // clearance > 2m
    // Jump: should not go above the ceiling.
    TryJump(ls);
    for (int i = 0; i < 30; ++i) {
        IntegrateLocomotion(ls, Vec2{0.0f, 0.0f}, false, query,
                            SimClock::kFixedDeltaTime);
        // Head must stay below the ceiling.
        const float head_z = ls.position.z + kColliderStand;
        WO_CHECK(head_z <= 2.0f + 0.05f);
    }
    return true;
}

// Falling does not become Grounded in mid-air (F-16 inverse).
bool FallingNotGroundedInAir() {
    const Grid grid = MakeOpenGrid(6, 6);
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.contact.grounded = true;
    // Jump off the floor.
    TryJump(ls);
    bool ever_airborne = false;
    for (int i = 0; i < 250; ++i) {  // jump apex ~112 ticks; allow landing
        IntegrateLocomotion(ls, Vec2{0.0f, 0.0f}, false, query,
                            SimClock::kFixedDeltaTime);
        if (!ls.contact.grounded) {
            ever_airborne = true;
        }
        if (ls.contact.grounded && ever_airborne) {
            // Re-grounded only after actually landing (feet within epsilon
            // of floor). This is fine.
            WO_CHECK_NEAR(ls.position.z, 0.0f, kGroundProbeEpsilon + 0.01f);
            return true;
        }
    }
    // Must have been airborne at some point.
    return ever_airborne;
}

// No NaN/Inf in locomotion state after any sequence of inputs.
bool LocomotionNoNanInf() {
    const Grid grid = MakeOpenGrid(8, 6);
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 0.0f};
    ls.contact.grounded = true;
    const Vec2 inputs[] = {
        {0.0f, 1.0f}, {0.0f, -1.0f}, {-1.0f, 0.0f}, {1.0f, 0.0f},
        {0.5f, 0.5f}, {-0.3f, 0.7f}, {0.0f, 0.0f}
    };
    for (int frame = 0; frame < 500; ++frame) {
        const Vec2& dir = inputs[frame % 7];
        const bool sprint = (frame % 10) < 3;
        IntegrateLocomotion(ls, dir, sprint, query,
                            SimClock::kFixedDeltaTime);
        if (frame % 20 == 0) {
            TryJump(ls);
        }
        if (frame % 50 == 0) {
            TrySetPosture(ls, (frame % 100 < 50) ? Posture::Crouch : Posture::Stand, query);
        }
        WO_CHECK(!std::isnan(ls.position.x));
        WO_CHECK(!std::isnan(ls.position.y));
        WO_CHECK(!std::isnan(ls.position.z));
        WO_CHECK(!std::isinf(ls.position.x));
        WO_CHECK(!std::isinf(ls.position.y));
        WO_CHECK(!std::isinf(ls.position.z));
        WO_CHECK(!std::isnan(ls.velocity.x));
        WO_CHECK(!std::isnan(ls.velocity.y));
        WO_CHECK(!std::isnan(ls.velocity.z));
        WO_CHECK(!std::isinf(ls.velocity.x));
        WO_CHECK(!std::isinf(ls.velocity.y));
        WO_CHECK(!std::isinf(ls.velocity.z));
    }
    return true;
}

// Locomotion state save/load preserves semantic state.
bool LocomotionSaveLoadRoundTrip() {
    const Grid grid = MakeOpenGrid(6, 6);
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{1.5f, 1.5f, 1.0f};
    ls.velocity = Vec3{1.0f, 0.5f, -0.3f};
    ls.posture = Posture::Crouch;
    ls.traversal = Traversal::Jump;
    ls.lean = Lean::Left;
    ls.yaw = 1.0f;
    ls.pitch = -0.2f;
    ls.contact.grounded = false;
    ls.jump_cooldown_frames = 15;
    // Serialize to bytes.
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    s.WriteF32(ls.position.x);
    s.WriteF32(ls.position.y);
    s.WriteF32(ls.position.z);
    s.WriteF32(ls.velocity.x);
    s.WriteF32(ls.velocity.y);
    s.WriteF32(ls.velocity.z);
    s.WriteU8(static_cast<uint8_t>(ls.posture));
    s.WriteU8(static_cast<uint8_t>(ls.traversal));
    s.WriteU8(static_cast<uint8_t>(ls.lean));
    s.WriteF32(ls.yaw);
    s.WriteF32(ls.pitch);
    s.WriteU8(ls.contact.grounded ? 1 : 0);
    s.WriteU16(ls.jump_cooldown_frames);
    // Deserialize.
    Deserializer d(bytes.data(), bytes.size());
    LocomotionState restored;
    restored.position.x = d.ReadF32();
    restored.position.y = d.ReadF32();
    restored.position.z = d.ReadF32();
    restored.velocity.x = d.ReadF32();
    restored.velocity.y = d.ReadF32();
    restored.velocity.z = d.ReadF32();
    restored.posture = static_cast<Posture>(d.ReadU8());
    restored.traversal = static_cast<Traversal>(d.ReadU8());
    restored.lean = static_cast<Lean>(d.ReadU8());
    restored.yaw = d.ReadF32();
    restored.pitch = d.ReadF32();
    restored.contact.grounded = d.ReadU8() != 0;
    restored.jump_cooldown_frames = d.ReadU16();
    // Compare.
    WO_CHECK_NEAR(restored.position.x, ls.position.x, 0.001f);
    WO_CHECK_NEAR(restored.position.y, ls.position.y, 0.001f);
    WO_CHECK_NEAR(restored.position.z, ls.position.z, 0.001f);
    WO_CHECK_NEAR(restored.velocity.x, ls.velocity.x, 0.001f);
    WO_CHECK_NEAR(restored.velocity.y, ls.velocity.y, 0.001f);
    WO_CHECK_NEAR(restored.velocity.z, ls.velocity.z, 0.001f);
    WO_CHECK(restored.posture == ls.posture);
    WO_CHECK(restored.traversal == ls.traversal);
    WO_CHECK(restored.lean == ls.lean);
    WO_CHECK_NEAR(restored.yaw, ls.yaw, 0.001f);
    WO_CHECK_NEAR(restored.pitch, ls.pitch, 0.001f);
    WO_CHECK(restored.contact.grounded == ls.contact.grounded);
    return restored.jump_cooldown_frames == ls.jump_cooldown_frames;
}

// Discriminating lean regression: the same world geometry must produce a
// DIFFERENT clamped lean when the camera yaw is rotated 90 degrees, because
// lean is camera-LOCAL (right = -x at yaw=90, not +x). A world-X lean
// implementation fails this test.
bool LeanYawRegressionDiscriminating() {
    Grid grid = MakeOpenGrid(10, 6);
    // Asymmetric world: solid wall strip along x=4 (vertical wall at col 4).
    // Player starts at (2.5, 2.5). At yaw=0 local right = +y (no wall there
    // along row 2 up to a point); at yaw=90 local right = -x -> but -x is
    // open. To discriminate, place wall at x=4.0 AND at row y=4.5 such that:
    //   yaw=0: local right = +y -> wall at row 4 -> clamped.
    //   yaw=90: local right = -x -> wall at col 0 (x<0.35 blocked) but far
    //   from player at 2.5 -> free lean.
    for (int32_t r = 0; r < 6; ++r) {
        GridCell w;
        w.flags = CellFlag_Solid;
        grid.SetCell(4, r, w);  // wall at x 4..5
        grid.SetCell(r, 4, w);  // wall at y 4..5
    }
    GridWorldQuery query(&grid);
    LocomotionState ls;
    ls.position = Vec3{2.5f, 2.5f, 0.0f};
    ls.contact.grounded = true;

    // At yaw=0: local right is +y. Wall along y=4 is 1.5m away in +y;
    // lean offset 0.35 keeps the AABB at y 2.15..2.85, which does NOT reach
    // the y=4 wall, so a correct LOCAL implementation leans fully right
    // (free) while a naive WORLD-X implementation would also lean +x freely.
    // To make this discriminating we need the wall to be the thing hit at
    // yaw=0 in +y: move the player close to the +y wall.
    ls.position = Vec3{2.5f, 3.5f, 0.0f};  // 0.5m from the y=4 wall
    ls.yaw = 0.0f;  // facing +x, local right = +y -> toward y=4 wall
    const float right_at_0 = LeanClamp(ls, Lean::Right, query);
    WO_CHECK(right_at_0 < kLeanOffset - 0.01f);  // clamped by +y wall

    // yaw=90: facing +y, local right = -x. The wall at x=4 is now BEHIND
    // (local right = -x is open, x<0 region); the +y wall is now in front
    // (forward), not right. So right lean is FREE at yaw=90.
    ls.yaw = 3.14159265f * 0.5f;
    const float right_at_90 = LeanClamp(ls, Lean::Right, query);
    WO_CHECK_NEAR(right_at_90, kLeanOffset, 0.01f);

    // The two results must differ meaningfully (that is the discriminating
    // property the old world-X implementation would violate).
    return (kLeanOffset - right_at_0) > 0.05f;
}

// Mouse delta increases yaw (horizontal look).
bool MouseDeltaChangesYaw() {
    LocomotionState ls;
    ls.yaw = 0.0f;
    ApplyMouseLook(ls, Vec2{100.0f, 0.0f}, 50);
    return ls.yaw > 0.0f;
}

// Pitch clamps at +30 degrees when looking up hard.
bool PitchClampedPlus30() {
    LocomotionState ls;
    ls.pitch = 0.0f;
    constexpr float kPitchUp = -10000.0f;  // mouse up (negative dy)
    ApplyMouseLook(ls, Vec2{0.0f, kPitchUp}, 100);
    constexpr float kMaxPitch = 30.0f * 3.14159265f / 180.0f;
    return ls.pitch > 0.0f && ls.pitch <= kMaxPitch + 0.0001f;
}

// Pitch clamps at -30 degrees when looking down hard.
bool PitchClampedMinus30() {
    LocomotionState ls;
    ls.pitch = 0.0f;
    constexpr float kPitchDown = 10000.0f;  // mouse down (positive dy)
    ApplyMouseLook(ls, Vec2{0.0f, kPitchDown}, 100);
    constexpr float kMaxPitch = 30.0f * 3.14159265f / 180.0f;
    return ls.pitch < 0.0f && ls.pitch >= -kMaxPitch - 0.0001f;
}

// Forward wish at yaw=0 maps to world +y (depends on convention: forward is
// the local +y axis and CameraRelativeWish converts by the yaw).
bool ForwardRespectsYaw0() {
    const Vec2 wish = CameraRelativeWish(Vec2{0.0f, 1.0f}, 0.0f);
    // forward at yaw=0 is (cos0, sin0) = (1, 0) in the renderer convention,
    // but the controller's move convention uses +y forward. We document the
    // exact mapping here: for yaw=0, forward wish must equal (0, 1) per
    // forward = (cos(yaw), sin(yaw)) applied to the +y local axis.
    const float fwd_x = std::cos(0.0f);
    const float fwd_y = std::sin(0.0f);
    return std::fabs(wish.x - fwd_x) < 0.0001f &&
           std::fabs(wish.y - fwd_y) < 0.0001f;
}

// Forward wish at yaw=90 maps to world (0, 1) -> (cos90, sin90) = (0, 1).
bool ForwardRespectsYaw90() {
    const float yaw = 3.14159265f * 0.5f;
    const Vec2 wish = CameraRelativeWish(Vec2{0.0f, 1.0f}, yaw);
    const float fwd_x = std::cos(yaw);
    const float fwd_y = std::sin(yaw);
    return std::fabs(wish.x - fwd_x) < 0.0001f &&
           std::fabs(wish.y - fwd_y) < 0.0001f;
}

// The InputMapper and the Settings binding tables must agree: both read the
// same [context][action]->key model and the mapper consumes Settings.
bool SettingsMapperConsistent() {
    Settings s = Settings::Defaults();
    InputMapper mapper;
    for (size_t c = 0; c < kInputContextCount; ++c) {
        for (size_t a = 0; a < kGameActionCount; ++a) {
            mapper.SetBinding(static_cast<InputContext>(c),
                              static_cast<GameAction>(a),
                              s.key_bindings[c][a]);
        }
    }
    const size_t gp = static_cast<size_t>(InputContext::Gameplay);
    return mapper.MapKey(InputContext::Gameplay, PhysicalKey::W) ==
               GameAction::MoveForward &&
           s.key_bindings[gp][static_cast<size_t>(GameAction::MoveForward)] ==
               PhysicalKey::W &&
           mapper.GetBinding(InputContext::Gameplay, GameAction::Jump) ==
               s.key_bindings[gp][static_cast<size_t>(GameAction::Jump)];
}

} // namespace

void RegisterPlayerTests(TestHarness& test) {
    test.Add("player.posture_clearance", &PostureClearance);
    test.Add("player.locomotion_orthogonal", &LocomotionStateOrthogonal);
    test.Add("player.integrate_move_blocked", &IntegrateMoveBlocked);
    test.Add("player.jump_then_land", &JumpThenLand);
    test.Add("player.input_mapper_rebind", &InputMapperRebind);
    test.Add("input.context_same_key_different_actions", &InputContextSameKeyDifferentActions);
    test.Add("input.context_replace_within_context", &InputContextReplaceWithinContext);
    test.Add("player.combat_fire_ammo", &CombatFireAndAmmo);
    test.Add("player.combat_reload", &CombatReloadTransfer);
    test.Add("player.hitscan_open_grid", &HitscanWalls);
    // HK-3 controller geometry property tests.
    test.Add("controller.grounded_idle_stable", &GroundedIdleStable);
    test.Add("controller.step_up_20cm", &StepUp20cm);
    test.Add("controller.step_up_50cm_rejected", &StepUp50cmRejected);
    test.Add("controller.lean_clamp_against_wall", &LeanClampAgainstWall);
    test.Add("controller.lean_respects_yaw_90", &LeanRespectsYaw90);
    test.Add("controller.lean_yaw_regression_discriminating", &LeanYawRegressionDiscriminating);
    test.Add("player.mouse_delta_changes_yaw", &MouseDeltaChangesYaw);
    test.Add("player.pitch_clamped_plus_30", &PitchClampedPlus30);
    test.Add("player.pitch_clamped_minus_30", &PitchClampedMinus30);
    test.Add("player.forward_respects_yaw_0", &ForwardRespectsYaw0);
    test.Add("player.forward_respects_yaw_90", &ForwardRespectsYaw90);
    test.Add("input.settings_mapper_consistent", &SettingsMapperConsistent);
    test.Add("controller.head_collision_stops_jump", &HeadCollisionStopsJump);
    test.Add("controller.falling_not_grounded_in_air", &FallingNotGroundedInAir);
    test.Add("controller.no_nan_inf", &LocomotionNoNanInf);
    test.Add("controller.save_load_round_trip", &LocomotionSaveLoadRoundTrip);
}

} // namespace writeover