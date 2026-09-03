#pragma once
// Character controller contract. SINGLE source of truth for locomotion state
// (M-006 closure): one position, one velocity, orthogonal posture/traversal/
// lean axes, and a contact descriptor. No duplicate onGround/velocityZ/
// gravityAccumulator fields. Collision checks ALL cells covered by the AABB
// (not just the center cell).

#include "writeover/common/clock.h"
#include "writeover/common/player_types.h"
#include "writeover/common/types.h"
#include "writeover/world/grid.h"  // IWorldQuery (player -> world is allowed)

#include <cstdint>

namespace writeover {

struct ContactState {
    bool grounded = false;
    bool on_ladder = false;
    bool on_climbable = false;
};

struct LocomotionState {
    Vec3 position;                // feet position
    Vec3 velocity;                // single velocity vector
    Posture posture = Posture::Stand;
    Traversal traversal = Traversal::Grounded;
    Lean lean = Lean::Center;
    float yaw = 0.0f;             // camera heading (radians)
    float pitch = 0.0f;           // camera pitch (radians, clamped +-30 deg)
    uint16_t jump_cooldown_frames = 0;
    ContactState contact;

    bool IsGrounded() const { return contact.grounded; }
    Vec3 EyePosition() const;  // feet + eye height for current posture
};

// Frozen constants.
inline constexpr float kWalkSpeed = 3.2f;
inline constexpr float kSprintSpeed = 5.2f;
inline constexpr float kCrouchSpeed = 1.6f;
inline constexpr float kProneSpeed = 0.8f;
inline constexpr float kGravity = 9.8f;
inline constexpr float kMaxFallVelocity = 18.0f;
inline constexpr float kJumpVelocity = 4.6f;
inline constexpr uint16_t kJumpCooldownFrames = 30;  // 0.25s at 120Hz
// Auto step-up height (P0 ~0.35m). Steps above this are NOT auto-climbed.
inline constexpr float kMaxStepHeight = 0.35f;
// Vertical epsilon for ground contact probes.
inline constexpr float kGroundProbeEpsilon = 0.05f;

AABB GetPostureBox(Posture posture, const Vec3& pos);

// True iff the given posture's AABB fits at pos (checks every covered cell).
bool CanFit(Posture posture, const Vec3& pos, const IWorldQuery& world);

// Axis-separated sweep test (same policy as IntegrateLocomotion).
bool CanMove(const Vec3& from, const Vec3& to, Posture posture,
             const IWorldQuery& world);

// HK-3 GroundProbe: true iff the feet are supported by a floor within
// kGroundProbeEpsilon below (standing on the floor stays grounded; F-16).
bool GroundProbe(const LocomotionState& ls, const IWorldQuery& world);

// HK-3 StepUp: tries to auto-step up to kMaxStepHeight when horizontal
// movement is blocked by a floor rise. Returns true if the step was taken
// (position advanced). Deterministic; no animation state.
bool TryStepUp(LocomotionState& ls, const Vec2& move_dir, float speed,
               float dt, const IWorldQuery& world);

// HK-3 StepDown: true iff a floor exists within step-down range below
// (falling onto a lower floor continues walking instead of falling).
bool StepDownAvailable(const LocomotionState& ls, const IWorldQuery& world);

// HK-3 HeadCollision: true iff a ceiling hit cancels upward velocity.
bool HeadCollision(const LocomotionState& ls, const IWorldQuery& world);

// HK-3 LeanClamp: returns the max lean offset magnitude (0..kLeanOffset)
// that fits without wall penetration in the given direction. 0 = no lean.
float LeanClamp(const LocomotionState& ls, Lean direction,
                const IWorldQuery& world);

// HK-3 NearWallWeaponQuery: distance to the nearest wall in the facing
// direction (for weapon lower/ADS restriction). Returns kMaxStepHeight*2
// style small distance when very close, else the actual distance.
float NearWallDistance(const LocomotionState& ls, const IWorldQuery& world);

// HK-3 eligibility queries (pure geometry; no animation state).
bool VaultEligibility(float obstacle_top, float obstacle_height,
                      float available_clearance);
bool MantleEligibility(float obstacle_top, float obstacle_height,
                       float available_clearance);
bool LadderEligibility(bool face_ladder, float ladder_height,
                       float headroom);

float GetMoveSpeed(Posture posture, bool sprint);

// Advances locomotion by one fixed tick (dt == SimClock::kFixedDeltaTime).
// input_dir is in world axes (X/Y) with length <= 1.
void IntegrateLocomotion(LocomotionState& ls,
                         const Vec2& input_dir,
                         bool sprint,
                         const IWorldQuery& world,
                         float dt);

void TryJump(LocomotionState& ls);

// Posture transition with clearance check (stand<->crouch<->prone;
// clearance windows enforced by CanFit).
void TrySetPosture(LocomotionState& ls, Posture target, const IWorldQuery& world);

void SetLean(LocomotionState& ls, int8_t lean);  // -1, 0, +1

} // namespace writeover