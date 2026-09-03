#pragma once
// WorldCommand: a typed REQUEST to change world state.
// Commands are point-to-point; they are NOT WorldEvents. World authoritative
// systems validate and apply commands during the mutation phase, then emit
// WorldEvent facts. The narrator may emit a subset of these commands through
// its capability set; it can never mutate the FactStore directly.

#include "writeover/common/ids.h"
#include "writeover/common/player_types.h"
#include "writeover/common/types.h"
#include "writeover/common/weapon_types.h"

#include <variant>

namespace writeover {

struct CommandMove {
    Vec2 move;
    bool sprint = false;
};
struct CommandLook {
    float yaw_delta = 0.0f;    // radians
    float pitch_delta = 0.0f;  // radians
};
struct CommandJump {};
struct CommandSetPosture {
    Posture posture = Posture::Stand;
};
struct CommandLean {
    int8_t lean = 0;  // -1 left, 0 center, +1 right
};
struct CommandFire {
    WeaponSlot slot = WeaponSlot::Pistol;
};
struct CommandReload {
    WeaponSlot slot = WeaponSlot::Pistol;
};
struct CommandInteract {};
struct CommandSetPower {
    SystemId system;
    bool powered = false;
};
struct CommandSetDoor {
    DoorId door;
    bool open = false;
};
struct CommandUseCheckpoint {};

using WorldCommand = std::variant<
    CommandMove, CommandLook, CommandJump, CommandSetPosture, CommandLean,
    CommandFire, CommandReload, CommandInteract,
    CommandSetPower, CommandSetDoor, CommandUseCheckpoint>;

} // namespace writeover