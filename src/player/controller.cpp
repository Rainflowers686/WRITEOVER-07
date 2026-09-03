#include "writeover/player/controller.h"

#include "writeover/common/math.h"

#include <cmath>

namespace writeover {

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

    // Vertical: gravity when airborne, or while falling off a ledge.
    if (!ls.contact.grounded || ls.velocity.z < 0.0f) {
        ls.velocity.z -= kGravity * dt;
        ls.velocity.z = std::max(ls.velocity.z, -kMaxFallVelocity);
    }

    // Integrate position axis-by-axis and collide.
    Vec3 next = ls.position + ls.velocity * dt;

    const AABB box_x = GetPostureBox(ls.posture, Vec3{next.x, ls.position.y, ls.position.z});
    if (world.AabbBlocked(box_x)) {
        next.x = ls.position.x;
        ls.velocity.x = 0.0f;
    }

    const AABB box_y = GetPostureBox(ls.posture, Vec3{next.x, next.y, ls.position.z});
    if (world.AabbBlocked(box_y)) {
        next.y = ls.position.y;
        ls.velocity.y = 0.0f;
    }

    const AABB box_z = GetPostureBox(ls.posture, Vec3{next.x, next.y, next.z});
    if (world.AabbBlocked(box_z)) {
        // Vertical collision: snap to ground (feet at floor height).
        next.z = world.FloorHeightAt(next.x, next.y);
        ls.velocity.z = 0.0f;
        ls.contact.grounded = true;
        ls.traversal = Traversal::Grounded;
    } else {
        ls.contact.grounded = false;
        if (ls.traversal == Traversal::Grounded && ls.velocity.z != 0.0f) {
            ls.traversal = Traversal::Fall;
        }
    }

    ls.position = next;

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

} // namespace writeover