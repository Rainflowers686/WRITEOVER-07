#include "writeover/world/infrastructure.h"

namespace writeover {

void InfrastructureSystem::AddDoor(DoorState door) {
    for (auto& d : doors_) {
        if (d.id == door.id) {
            d = door;
            return;
        }
    }
    doors_.push_back(door);
}

void InfrastructureSystem::AddSystem(SystemState system) {
    for (auto& s : systems_) {
        if (s.id == system.id) {
            s = system;
            return;
        }
    }
    systems_.push_back(system);
}

const DoorState* InfrastructureSystem::GetDoor(DoorId id) const {
    for (const auto& d : doors_) {
        if (d.id == id) {
            return &d;
        }
    }
    return nullptr;
}

const SystemState* InfrastructureSystem::GetSystem(SystemId id) const {
    for (const auto& s : systems_) {
        if (s.id == id) {
            return &s;
        }
    }
    return nullptr;
}

bool InfrastructureSystem::SetDoorOpen(DoorId id, bool open) {
    for (auto& d : doors_) {
        if (d.id == id) {
            if (d.locked && open) {
                return false;  // locked doors cannot be opened by commands
            }
            if (d.open == open) {
                return false;  // no change -> no event
            }
            d.open = open;
            return true;
        }
    }
    return false;
}

bool InfrastructureSystem::SetDoorLocked(DoorId id, bool locked) {
    for (auto& d : doors_) {
        if (d.id == id) {
            d.locked = locked;
            if (locked) {
                d.open = false;
            }
            return true;
        }
    }
    return false;
}

bool InfrastructureSystem::SetPowered(SystemId id, bool powered) {
    for (auto& s : systems_) {
        if (s.id == id) {
            if (s.powered == powered) {
                return false;  // no change -> no event
            }
            s.powered = powered;
            return true;
        }
    }
    return false;
}

void InfrastructureSystem::Save(Serializer& s) const {
    s.WriteU32(static_cast<uint32_t>(doors_.size()));
    for (const auto& d : doors_) {
        WriteId(s, d.id);
        s.WriteU8(d.open ? 1 : 0);
        s.WriteU8(d.locked ? 1 : 0);
    }
    s.WriteU32(static_cast<uint32_t>(systems_.size()));
    for (const auto& sys : systems_) {
        WriteId(s, sys.id);
        s.WriteU8(sys.system_type);
        s.WriteU8(sys.powered ? 1 : 0);
    }
}

void InfrastructureSystem::Load(Deserializer& d) {
    doors_.clear();
    systems_.clear();
    const uint32_t door_count = d.ReadU32();
    for (uint32_t i = 0; i < door_count; ++i) {
        DoorState door;
        door.id = ReadId<DoorId>(d);
        door.open = d.ReadU8() != 0;
        door.locked = d.ReadU8() != 0;
        doors_.push_back(door);
    }
    const uint32_t system_count = d.ReadU32();
    for (uint32_t i = 0; i < system_count; ++i) {
        SystemState sys;
        sys.id = ReadId<SystemId>(d);
        sys.system_type = d.ReadU8();
        sys.powered = d.ReadU8() != 0;
        systems_.push_back(sys);
    }
}

} // namespace writeover