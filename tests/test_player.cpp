#include "tests/test_harness.h"

#include "writeover/player/combat.h"
#include "writeover/player/controller.h"
#include "writeover/player/input.h"
#include "writeover/player/weapon.h"
#include "writeover/world/grid.h"

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

} // namespace

void RegisterPlayerTests(TestHarness& test) {
    test.Add("player.posture_clearance", &PostureClearance);
    test.Add("player.locomotion_orthogonal", &LocomotionStateOrthogonal);
    test.Add("player.integrate_move_blocked", &IntegrateMoveBlocked);
    test.Add("player.jump_then_land", &JumpThenLand);
    test.Add("player.input_mapper_rebind", &InputMapperRebind);
    test.Add("player.combat_fire_ammo", &CombatFireAndAmmo);
    test.Add("player.combat_reload", &CombatReloadTransfer);
    test.Add("player.hitscan_open_grid", &HitscanWalls);
}

} // namespace writeover