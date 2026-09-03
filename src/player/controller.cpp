#include "writeover/player/controller.h"

#include "writeover/common/math.h"

#include <cmath>

namespace writeover {

namespace {

// A blocked horizontal move may still be a CLIMBABLE STEP (floor rise up to
// kMaxStepHeight above the player's feet) rather than a wall. Returns true
// when the only blocking conditions at `pos` are step-rise floors.
bool IsStepRiseOnly(const Vec3& pos, Posture posture, float feet_z,
                    const IWorldQuery& world) {
    const AABB box = GetPostureBox(posture, pos);
    const int32_t min_col = static_cast<int32_t>(std::floor(box.min.x));
    const int32_t max_col = static_cast<int32_t>(std::floor(box.max.x));
    const int32_t min_row = static_cast<int32_t>(std::floor(box.min.y));
    const int32_t max_row = static_cast<int32_t>(std::floor(box.max.y));
    bool has_step = false;
    for (int32_t row = min_row; row <= max_row; ++row) {
        for (int32_t col = min_col; col <= max_col; ++col) {
            if (col < 0 || row < 0 || col >= world.Width() ||
                row >= world.Height()) {
                return false;  // map edge = real wall
            }
            if (world.IsSolidAt(col, row)) {
                return false;  // solid = real wall
            }
            const GridCell cell = world.GetCell(col, row);
            if (cell.ceiling_height < box.max.z - kEpsPosition) {
                return false;  // low ceiling = real block
            }
            const float floor_rise = cell.floor_height - feet_z;
            if (floor_rise > kMaxStepHeight + kEpsPosition) {
                return false;  // step too high = real wall
            }
            if (floor_rise > kEpsPosition) {
                has_step = true;
            }
        }
    }
    return has_step;
}

} // namespace

Vec3 LocomotionState::EyePosition() const {
    return Vec3{position.x, position.y,
                position.z + GetPostureParams(posture).eye_height};
}

AABB GetPostureBox(Posture posture, const Vec3& pos) {
    const PostureParams p = GetPostureParams(posture);
    AABB box;
    box.min = Vec3{pos.x - p.radius, pos.y - p.radius, pos.z};
    box.max = Vec3{pos.x + p.radius, pos.y + p.radius, pos.z + p.collider_height};
    return box;
}

bool CanFit(Posture posture, const Vec3& pos, const IWorldQuery& world) {
    return !world.AabbBlocked(GetPostureBox(posture, pos));
}

bool CanMove(const Vec3& from, const Vec3& to, Posture posture,
             const IWorldQuery& world) {
    // Axis-separated sweep: resolve X, then Y, then Z. This is what the
    // controller does each tick; the helper exposes the same policy to tests.
    Vec3 candidate = from;

    candidate.x = to.x;
    if (world.AabbBlocked(GetPostureBox(posture, candidate))) {
        return false;
    }

    candidate.y = to.y;
    if (world.AabbBlocked(GetPostureBox(posture, candidate))) {
        return false;
    }

    candidate.z = to.z;
    if (world.AabbBlocked(GetPostureBox(posture, candidate))) {
        return false;
    }
    return true;
}

float GetMoveSpeed(Posture posture, bool sprint) {
    if (posture == Posture::Prone) {
        return kProneSpeed;
    }
    if (posture == Posture::Crouch) {
        return kCrouchSpeed;
    }
    return sprint ? kSprintSpeed : kWalkSpeed;
}

void IntegrateLocomotion(LocomotionState& ls,
                         const Vec2& input_dir,
                         bool sprint,
                         const IWorldQuery& world,
                         float dt) {
    // Horizontal wish direction, normalized.
    const float speed = GetMoveSpeed(ls.posture, sprint);
    Vec2 wish = input_dir;
    const float wish_len = Length(wish);
    if (wish_len > 1.0f) {
        wish = wish * (1.0f / wish_len);
    }
    const Vec2 target_h = wish * speed;

    // Light deterministic smoothing toward the target speed.
    constexpr float kHorizSnap = 12.0f;
    const float k = Clamp(kHorizSnap * dt, 0.0f, 1.0f);
    ls.velocity.x = Lerp(ls.velocity.x, target_h.x, k);
    ls.velocity.y = Lerp(ls.velocity.y, target_h.y, k);

    // Vertical: gravity when airborne.
    if (!ls.contact.grounded) {
        ls.velocity.z -= kGravity * dt;
        ls.velocity.z = std::max(ls.velocity.z, -kMaxFallVelocity);
    } else {
        ls.velocity.z = 0.0f;  // snap to ground when grounded
    }

    // Head collision: cancel upward velocity if ceiling hit.
    if (ls.velocity.z > 0.0f && HeadCollision(ls, world)) {
        ls.velocity.z = 0.0f;
    }

    // Integrate position axis-by-axis and collide.
    Vec3 next = ls.position + ls.velocity * dt;

    // X-axis move: a block that is only a climbable step does not cancel
    // the move; the Z pass snaps the feet up onto the step.
    Vec3 candidate = Vec3{next.x, ls.position.y, ls.position.z};
    if (world.AabbBlocked(GetPostureBox(ls.posture, candidate))) {
        if (!IsStepRiseOnly(candidate, ls.posture, ls.position.z, world)) {
            next.x = ls.position.x;
            ls.velocity.x = 0.0f;
        }
    }

    // Y-axis move.
    candidate = Vec3{next.x, next.y, ls.position.z};
    if (world.AabbBlocked(GetPostureBox(ls.posture, candidate))) {
        if (!IsStepRiseOnly(candidate, ls.posture, ls.position.z, world)) {
            next.y = ls.position.y;
            ls.velocity.y = 0.0f;
        }
    }

    // Z-axis move with floor snap and step-up onto higher floors.
    candidate = Vec3{next.x, next.y, next.z};
    if (world.AabbBlocked(GetPostureBox(ls.posture, candidate))) {
        // Vertical collision: snap to ground.
        next.z = world.FloorHeightAt(next.x, next.y);
        ls.velocity.z = 0.0f;
    } else {
        // Floor snap: if feet are below the floor (e.g., stepping onto a
        // raised floor), snap up. Only snap when not jumping upward.
        const float floor_z = world.FloorHeightAt(next.x, next.y);
        if (next.z < floor_z - kEpsPosition && !(ls.velocity.z > 0.0f)) {
            const float rise = floor_z - next.z;
            if (rise <= kMaxStepHeight + kGroundProbeEpsilon) {
                next.z = floor_z;
                if (ls.velocity.z < 0.0f) {
                    ls.velocity.z = 0.0f;
                }
            }
        }
    }

    ls.position = next;

    // Ground probe: determine grounded state from floor contact.
    ls.contact.grounded = GroundProbe(ls, world);
    if (ls.contact.grounded) {
        ls.traversal = Traversal::Grounded;
    } else if (ls.velocity.z != 0.0f) {
        ls.traversal = Traversal::Fall;
    }

    if (ls.jump_cooldown_frames > 0) {
        --ls.jump_cooldown_frames;
    }
}

void TryJump(LocomotionState& ls) {
    if (ls.contact.grounded && ls.jump_cooldown_frames == 0) {
        ls.velocity.z = kJumpVelocity;
        ls.traversal = Traversal::Jump;
        ls.contact.grounded = false;
        ls.jump_cooldown_frames = kJumpCooldownFrames;
    }
}

void TrySetPosture(LocomotionState& ls, Posture target, const IWorldQuery& world) {
    if (target == ls.posture) {
        return;
    }
    if (!CanFit(target, ls.position, world)) {
        return;  // clearance check fails (checks every covered cell)
    }
    // Feet stay planted; only the collider height and eye height change.
    ls.posture = target;
}

void SetLean(LocomotionState& ls, int8_t lean) {
    if (lean < 0) {
        ls.lean = Lean::Left;
    } else if (lean > 0) {
        ls.lean = Lean::Right;
    } else {
        ls.lean = Lean::Center;
    }
}

// ---------------------------------------------------------------------------
// HK-3 Character Controller Geometry (F-16, F-17, F-18 closure)
// ---------------------------------------------------------------------------

bool GroundProbe(const LocomotionState& ls, const IWorldQuery& world) {
    const float floor_z = world.FloorHeightAt(ls.position.x, ls.position.y);
    const float feet_z = ls.position.z;
    // Grounded if feet are within epsilon of the floor below AND not
    // ascending (a jump start must not re-ground on the first tick).
    return (feet_z - floor_z) <= kGroundProbeEpsilon &&
           !(ls.velocity.z > kEpsVelocity);
}

bool TryStepUp(LocomotionState& ls, const Vec2& move_dir, float speed,
               float dt, const IWorldQuery& world) {
    const Vec2 wish = Normalize(move_dir) * speed * dt;
    const Vec3 target = Vec3{ls.position.x + wish.x, ls.position.y + wish.y,
                             ls.position.z};
    const AABB box = GetPostureBox(ls.posture, target);
    if (!world.AabbBlocked(box)) {
        return false;  // move succeeds without step-up
    }
    // Real wall (solid / low ceiling / too-high step): not a climbable step.
    if (!IsStepRiseOnly(target, ls.posture, ls.position.z, world)) {
        return false;
    }
    // Climbable step: raise the feet to the target floor if clearance fits.
    const float target_floor = world.FloorHeightAt(target.x, target.y);
    const Vec3 raised = Vec3{target.x, target.y, target_floor};
    if (world.AabbBlocked(GetPostureBox(ls.posture, raised))) {
        return false;
    }
    // Apply the step-up.
    ls.position = raised;
    ls.velocity.z = 0.0f;
    return true;
}

bool StepDownAvailable(const LocomotionState& ls, const IWorldQuery& world) {
    const float floor_z = world.FloorHeightAt(ls.position.x, ls.position.y);
    const float feet_z = ls.position.z;
    // Step-down available if there's a floor within step height below.
    return (feet_z - floor_z) <= kMaxStepHeight + kGroundProbeEpsilon;
}

bool HeadCollision(const LocomotionState& ls, const IWorldQuery& world) {
    const float ceil_z = world.CeilingHeightAt(ls.position.x, ls.position.y);
    const float head_z = ls.position.z + GetPostureParams(ls.posture).collider_height;
    return (head_z - ceil_z) >= -kGroundProbeEpsilon;
}

float LeanClamp(const LocomotionState& ls, Lean direction,
                const IWorldQuery& world) {
    const float offset = (direction == Lean::Left) ? -kLeanOffset : kLeanOffset;
    const Vec3 lean_pos = Vec3{ls.position.x + offset, ls.position.y, ls.position.z};
    const AABB lean_box = GetPostureBox(ls.posture, lean_pos);
    if (world.AabbBlocked(lean_box)) {
        // Binary search for the maximum lean that fits.
        float lo = 0.0f, hi = kLeanOffset;
        for (int iter = 0; iter < 6; ++iter) {
            const float mid = (lo + hi) * 0.5f;
            const float test_offset = (direction == Lean::Left) ? -mid : mid;
            const Vec3 test_pos = Vec3{ls.position.x + test_offset,
                                        ls.position.y, ls.position.z};
            const AABB test_box = GetPostureBox(ls.posture, test_pos);
            if (!world.AabbBlocked(test_box)) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        return lo;
    }
    return kLeanOffset;
}

float NearWallDistance(const LocomotionState& ls, const IWorldQuery& world) {
    const float yaw = ls.yaw;
    const float dx = std::cos(yaw);
    const float dy = std::sin(yaw);
    // Raycast in the facing direction for a short distance.
    const float kProbeDistance = 1.0f;
    for (float t = 0.1f; t <= kProbeDistance; t += 0.05f) {
        const float px = ls.position.x + dx * t;
        const float py = ls.position.y + dy * t;
        const AABB probe_box = GetPostureBox(ls.posture,
                                            Vec3{px, py, ls.position.z});
        if (world.AabbBlocked(probe_box)) {
            return t;
        }
    }
    return kProbeDistance;
}

bool VaultEligibility(float obstacle_top, float obstacle_height,
                      float available_clearance) {
    (void)obstacle_height;
    // Vault: player jumps onto the obstacle and vaults over.
    return obstacle_top <= kColliderCrouch + kMaxStepHeight &&
           available_clearance >= kClearanceCrouchMin;
}

bool MantleEligibility(float obstacle_top, float obstacle_height,
                       float available_clearance) {
    (void)obstacle_height;
    // Mantle: player pulls up onto the obstacle.
    return obstacle_top <= kColliderStand &&
           available_clearance >= kClearanceStandMin;
}

bool LadderEligibility(bool face_ladder, float ladder_height,
                       float headroom) {
    return face_ladder && ladder_height > 0.5f && headroom >= kClearanceStandMin;
}

} // namespace writeover