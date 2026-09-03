#pragma once
// Weapon slot vocabulary (shared: player combat produces commands and events
// that world systems consume; the enum lives in common to avoid a
// player->world cycle).

#include <cstdint>
#include <cstddef>

namespace writeover {

enum class WeaponSlot : uint8_t {
    Pistol = 0,
    Smg = 1,
    Stunner = 2,
    Count = 3,
};

inline constexpr size_t kWeaponSlotCount = static_cast<size_t>(WeaponSlot::Count);

} // namespace writeover