#pragma once
// Combat: camera-view hitscan + deterministic spread/recoil via sim RNG.
// All identities are strong IDs (M 17.2 closure).

#include "writeover/common/ids.h"
#include "writeover/common/rng.h"
#include "writeover/common/serialize.h"
#include "writeover/common/weapon_types.h"
#include "writeover/player/weapon.h"
#include "writeover/world/grid.h"  // IWorldQuery (player -> world is allowed)

#include <array>
#include <cstdint>

namespace writeover {

struct CombatState {
    WeaponSlot slot = WeaponSlot::Pistol;
    std::array<uint16_t, kWeaponSlotCount> ammo_in_mag = {12, 30, 10};
    std::array<uint16_t, kWeaponSlotCount> reserve = {48, 120, 30};
    uint32_t reload_frames_left = 0;
    uint32_t next_fire_frame = 0;
    uint32_t last_shot_frame = 0;
    bool aiming = false;
    float spread_factor = 0.0f;  // grows while firing, decays with time
};

void ResetCombatState(CombatState& state);

bool CanFire(const CombatState& state, const WeaponDef& weapon, uint32_t frame);
// Returns true and decrements ammo when a shot is allowed.
bool ConsumeShot(CombatState& state, const WeaponDef& weapon, uint32_t frame);
void StartReload(CombatState& state, WeaponSlot slot, const WeaponDef& weapon);
void AdvanceReload(CombatState& state, uint32_t ticks);

struct HitscanResult {
    bool hit = false;
    EntityId target_id;       // invalid = static world geometry
    Vec3 hit_point;
    float distance = 0.0f;
    bool headshot = false;
    uint16_t damage = 0;
    // The exact deterministic ray after spread/recoil. Runtime target
    // adapters use this same direction so NPC hits and static occlusion do
    // not consume a second random sample.
    float resolved_yaw = 0.0f;
    float resolved_pitch = 0.0f;
};

struct FireRequest {
    Vec3 origin;              // camera/eye position
    float yaw = 0.0f;
    float pitch = 0.0f;
    WeaponSlot slot = WeaponSlot::Pistol;
    float spread_factor = 0.0f;
};

// March RayResult-style through the grid; first full-occlusion boundary at
// eye height is a static hit. Deterministic spread is applied via sim RNG.
HitscanResult ResolveHitscan(const FireRequest& request,
                             const WeaponDef& weapon,
                             const IWorldQuery& world,
                             DeterministicRNG& sim_rng);

// Applies spread/recoil offsets to yaw/pitch (deterministic).
void ApplyShotJitter(const WeaponDef& weapon, float spread_factor,
                     float& yaw, float& pitch, DeterministicRNG& sim_rng);

void SerializeCombatState(Serializer& s, const CombatState& c);
void DeserializeCombatState(Deserializer& d, CombatState& c);

} // namespace writeover
