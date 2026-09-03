#pragma once
// Orthogonal player state axes (frozen). Posture/traversal/lean are separate,
// so "crouch while jumping over a low wall leaning right" stays expressible
// without a state explosion. Shared vocabulary used by world collision,
// player controller and renderer eye-height projection.

#include "writeover/common/types.h"

#include <cstdint>

namespace writeover {

// Body height (mutually exclusive).
enum class Posture : uint8_t {
    Stand = 0,   // collider 1.80m, eye 1.60m
    Crouch = 1,  // collider 1.20m, eye 1.00m
    Prone = 2,   // collider 0.55m, eye 0.42m
    Count = 3,
};

// Locomotion (transient movement state).
enum class Traversal : uint8_t {
    Grounded = 0,
    Jump = 1,
    Fall = 2,
    Vault = 3,   // gated by WO_FEATURE_VAULT
    Climb = 4,
    Mantle = 5,  // gated by WO_FEATURE_MANTLE
    Count = 6,
};

// Lateral offset axis (independent).
enum class Lean : uint8_t {
    Center = 0,
    Left = 1,
    Right = 2,
    Count = 3,
};

// Per-posture dimensions. The ordering of these intervals is a hard contract:
// prone < crouch < stand; clearance windows must not conflict.
struct PostureParams {
    float collider_height = kColliderStand;
    float eye_height = kEyeStand;
    float radius = kPlayerRadius;
};

inline PostureParams GetPostureParams(Posture p) {
    switch (p) {
    case Posture::Prone:
        return PostureParams{kColliderProne, kEyeProne, kPlayerRadius};
    case Posture::Crouch:
        return PostureParams{kColliderCrouch, kEyeCrouch, kPlayerRadius};
    case Posture::Stand:
    default:
        return PostureParams{kColliderStand, kEyeStand, kPlayerRadius};
    }
}

} // namespace writeover