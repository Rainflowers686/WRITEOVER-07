#pragma once
// Weapon definitions (data-driven). Display names are StringIds, never raw
// const char* (M 17.3 closure).

#include "writeover/common/ids.h"
#include "writeover/common/weapon_types.h"

#include <array>
#include <cstdint>

namespace writeover {

struct WeaponDef {
    WeaponId id;
    StringId name;            // localization key
    uint16_t damage = 10;
    uint16_t ammo_per_mag = 12;
    uint16_t reserve_capacity = 48;
    float fire_rate_hz = 8.0f;
    float reload_seconds = 1.5f;
    bool is_hitscan = true;
    uint8_t shot_count = 1;
    float spread_degrees = 1.0f;
    float recoil_up_deg = 2.0f;
    float recoil_side_deg = 0.5f;
    float range_meters = 25.0f;
    bool non_lethal = false;
    uint8_t damage_type = 0;   // 0=kinetic, 1=electrical
    float loudness = 0.8f;     // AI hearing radius factor
};

// Frozen P0 defaults: pistol / SMG / stunner.
const std::array<WeaponDef, kWeaponSlotCount>& DefaultWeapons();

} // namespace writeover