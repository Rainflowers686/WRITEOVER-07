#include "writeover/player/weapon.h"

namespace writeover {

const std::array<WeaponDef, kWeaponSlotCount>& DefaultWeapons() {
    static const std::array<WeaponDef, kWeaponSlotCount> kDefaults = {{
        // Pistol
        WeaponDef{WeaponId::New(1), StringId::New(101), 12, 12, 48, 8.0f, 1.2f,
                  true, 1, 1.2f, 2.0f, 0.5f, 25.0f, false, 0, 0.6f},
        // SMG
        WeaponDef{WeaponId::New(2), StringId::New(102), 6, 30, 120, 14.0f, 1.8f,
                  true, 1, 2.5f, 2.8f, 0.9f, 20.0f, false, 0, 0.85f},
        // Electro-stunner (non-lethal)
        WeaponDef{WeaponId::New(3), StringId::New(103), 0, 10, 30, 4.0f, 2.0f,
                  true, 1, 0.4f, 0.0f, 0.0f, 12.0f, true, 1, 0.45f},
    }};
    return kDefaults;
}

} // namespace writeover