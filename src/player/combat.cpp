#include "writeover/player/combat.h"

#include "writeover/common/math.h"

#include <cmath>

namespace writeover {

namespace {
constexpr float kFramesPerSecond = 120.0f;
} // namespace

void ResetCombatState(CombatState& state) {
    state.slot = WeaponSlot::Pistol;
    state.ammo_in_mag = {12, 30, 10};
    state.reserve = {48, 120, 30};
    state.reload_frames_left = 0;
    state.next_fire_frame = 0;
    state.last_shot_frame = 0;
    state.aiming = false;
    state.spread_factor = 0.0f;
}

bool CanFire(const CombatState& state, const WeaponDef& weapon, uint32_t frame) {
    if (state.reload_frames_left > 0) {
        return false;
    }
    if (frame < state.next_fire_frame) {
        return false;
    }
    const size_t slot = static_cast<size_t>(state.slot);
    if (state.ammo_in_mag[slot] == 0) {
        return false;
    }
    return weapon.is_hitscan;  // all P0 weapons are hitscan
}

bool ConsumeShot(CombatState& state, const WeaponDef& weapon, uint32_t frame) {
    if (!CanFire(state, weapon, frame)) {
        return false;
    }
    const size_t slot = static_cast<size_t>(state.slot);
    --state.ammo_in_mag[slot];
    state.next_fire_frame = frame + static_cast<uint32_t>(
        (1.0f / weapon.fire_rate_hz) * kFramesPerSecond + 0.5f);
    state.last_shot_frame = frame;
    state.spread_factor = std::min(1.0f, state.spread_factor + 0.08f);
    return true;
}

void StartReload(CombatState& state, WeaponSlot slot, const WeaponDef& weapon) {
    const size_t i = static_cast<size_t>(slot);
    if (state.ammo_in_mag[i] >= weapon.ammo_per_mag || state.reserve[i] == 0) {
        return;
    }
    state.reload_frames_left = static_cast<uint32_t>(
        weapon.reload_seconds * kFramesPerSecond + 0.5f);
}

void AdvanceReload(CombatState& state, uint32_t ticks) {
    if (state.reload_frames_left == 0) {
        return;
    }
    if (ticks >= state.reload_frames_left) {
        state.reload_frames_left = 0;
    } else {
        state.reload_frames_left -= ticks;
        return;
    }
    const size_t i = static_cast<size_t>(state.slot);
    const WeaponDef& weapon = DefaultWeapons()[i];
    const uint16_t needed = weapon.ammo_per_mag - state.ammo_in_mag[i];
    const uint16_t taken = static_cast<uint16_t>(std::min<uint32_t>(needed, state.reserve[i]));
    state.ammo_in_mag[i] += taken;
    state.reserve[i] -= taken;
}

void ApplyShotJitter(const WeaponDef& weapon, float spread_factor,
                     float& yaw, float& pitch, DeterministicRNG& sim_rng) {
    const float total_spread = weapon.spread_degrees * (1.0f + spread_factor * 1.5f);
    yaw += DegToRad(total_spread) * (sim_rng.NextFloat() * 2.0f - 1.0f);
    pitch += DegToRad(total_spread) * (sim_rng.NextFloat() * 2.0f - 1.0f);
    yaw += DegToRad(weapon.recoil_side_deg) * (sim_rng.NextFloat() - 0.5f);
    pitch -= DegToRad(weapon.recoil_up_deg) * 0.5f;
}

HitscanResult ResolveHitscan(const FireRequest& request,
                             const WeaponDef& weapon,
                             const IWorldQuery& world,
                             DeterministicRNG& sim_rng) {
    HitscanResult result;
    float yaw = request.yaw;
    float pitch = request.pitch;
    ApplyShotJitter(weapon, 0.0f, yaw, pitch, sim_rng);  // spread_factor=0 for stub

    const float dx = std::cos(yaw);
    const float dy = std::sin(yaw);
    const float dz = std::tan(pitch);

    constexpr float kRayStep = 0.25f;
    const float max_steps = weapon.range_meters / kRayStep;
    for (int i = 0; i <= static_cast<int>(max_steps); ++i) {
        const float t = static_cast<float>(i) * kRayStep;
        if (t > weapon.range_meters) {
            break;
        }
        const float x = request.origin.x + dx * t;
        const float y = request.origin.y + dy * t;
        const float z = request.origin.z + dz * t;
        const int32_t col = static_cast<int32_t>(std::floor(x));
        const int32_t row = static_cast<int32_t>(std::floor(y));
        if (!(col >= 0 && col < world.Width() && row >= 0 && row < world.Height())) {
            break;
        }
        const GridCell cell = world.GetCell(col, row);
        const bool full_block = cell.IsSolid() ||
                                cell.floor_height > z || cell.ceiling_height < z;
        if (full_block) {
            result.hit = true;
            result.hit_point = Vec3{x, y, z};
            result.distance = t;
            result.damage = weapon.non_lethal ? 0 : weapon.damage;
            return result;
        }
    }
    return result;
}

void SerializeCombatState(Serializer& s, const CombatState& c) {
    s.WriteU8(static_cast<uint8_t>(c.slot));
    for (const auto ammo : c.ammo_in_mag) {
        s.WriteU16(ammo);
    }
    for (const auto res : c.reserve) {
        s.WriteU16(res);
    }
    s.WriteU32(c.reload_frames_left);
    s.WriteU32(c.next_fire_frame);
    s.WriteU32(c.last_shot_frame);
    s.WriteU8(c.aiming ? 1 : 0);
    s.WriteF32(c.spread_factor);
}

void DeserializeCombatState(Deserializer& d, CombatState& c) {
    c.slot = static_cast<WeaponSlot>(d.ReadU8());
    for (auto& ammo : c.ammo_in_mag) {
        ammo = d.ReadU16();
    }
    for (auto& res : c.reserve) {
        res = d.ReadU16();
    }
    c.reload_frames_left = d.ReadU32();
    c.next_fire_frame = d.ReadU32();
    c.last_shot_frame = d.ReadU32();
    c.aiming = d.ReadU8() != 0;
    c.spread_factor = d.ReadF32();
}

} // namespace writeover