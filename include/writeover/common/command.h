#pragma once
// WorldCommand: a typed REQUEST to change world state.
// Commands are point-to-point; they are NOT WorldEvents. World authoritative
// systems validate and apply commands during the mutation phase, then emit
// WorldEvent facts. The narrator may emit a subset of these commands through
// its capability set; it can never mutate the FactStore directly.

#include "writeover/common/ids.h"
#include "writeover/common/player_types.h"
#include "writeover/common/serialize.h"
#include "writeover/common/types.h"
#include "writeover/common/weapon_types.h"

#include <cstdint>
#include <type_traits>
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

// Serialize a WorldCommand to a byte stream.
inline void SerializeWorldCommand(Serializer& s, const WorldCommand& cmd) {
    s.WriteU8(static_cast<uint8_t>(cmd.index()));
    std::visit([&s](const auto& c) {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, CommandMove>) {
            s.WriteF32(c.move.x);
            s.WriteF32(c.move.y);
            s.WriteU8(c.sprint ? 1 : 0);
        } else if constexpr (std::is_same_v<T, CommandLook>) {
            s.WriteF32(c.yaw_delta);
            s.WriteF32(c.pitch_delta);
        } else if constexpr (std::is_same_v<T, CommandJump>) {
            // no payload
        } else if constexpr (std::is_same_v<T, CommandSetPosture>) {
            s.WriteU8(static_cast<uint8_t>(c.posture));
        } else if constexpr (std::is_same_v<T, CommandLean>) {
            s.WriteI8(c.lean);
        } else if constexpr (std::is_same_v<T, CommandFire>) {
            s.WriteU8(static_cast<uint8_t>(c.slot));
        } else if constexpr (std::is_same_v<T, CommandReload>) {
            s.WriteU8(static_cast<uint8_t>(c.slot));
        } else if constexpr (std::is_same_v<T, CommandInteract>) {
            // no payload
        } else if constexpr (std::is_same_v<T, CommandSetPower>) {
            WriteId(s, c.system);
            s.WriteU8(c.powered ? 1 : 0);
        } else if constexpr (std::is_same_v<T, CommandSetDoor>) {
            WriteId(s, c.door);
            s.WriteU8(c.open ? 1 : 0);
        } else if constexpr (std::is_same_v<T, CommandUseCheckpoint>) {
            // no payload
        }
    }, cmd);
}

// Deserialize a WorldCommand from a byte stream.
inline WorldCommand DeserializeWorldCommand(Deserializer& d) {
    const uint8_t tag = d.ReadU8();
    switch (tag) {
    case 0: {
        CommandMove c;
        c.move.x = d.ReadF32();
        c.move.y = d.ReadF32();
        c.sprint = d.ReadU8() != 0;
        return c;
    }
    case 1: {
        CommandLook c;
        c.yaw_delta = d.ReadF32();
        c.pitch_delta = d.ReadF32();
        return c;
    }
    case 2: return CommandJump{};
    case 3: return CommandSetPosture{static_cast<Posture>(d.ReadU8())};
    case 4: return CommandLean{d.ReadI8()};
    case 5: return CommandFire{static_cast<WeaponSlot>(d.ReadU8())};
    case 6: return CommandReload{static_cast<WeaponSlot>(d.ReadU8())};
    case 7: return CommandInteract{};
    case 8: {
        CommandSetPower c;
        c.system = ReadId<SystemId>(d);
        c.powered = d.ReadU8() != 0;
        return c;
    }
    case 9: {
        CommandSetDoor c;
        c.door = ReadId<DoorId>(d);
        c.open = d.ReadU8() != 0;
        return c;
    }
    case 10: return CommandUseCheckpoint{};
    default:
        d.Skip(1024);  // mark error; skip enough to avoid infinite loop
        return CommandInteract{};
    }
}

} // namespace writeover