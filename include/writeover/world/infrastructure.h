#pragma once
// Infrastructure systems: doors, power, cameras. World authoritative systems
// apply WorldCommands and emit WorldEvents. The narrator reaches these only
// through typed commands (capability-gated).

#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"

#include <cstdint>
#include <vector>

namespace writeover {

struct DoorState {
    DoorId id;
    bool open = false;
    bool locked = false;
};

struct SystemState {
    SystemId id;
    uint8_t system_type = 0;  // 0=power, 1=camera, 2=comms, 3=door-group
    bool powered = false;
};

class InfrastructureSystem {
public:
    void AddDoor(DoorState door);
    void AddSystem(SystemState system);

    const DoorState* GetDoor(DoorId id) const;
    const SystemState* GetSystem(SystemId id) const;

    // Returns true when a change was applied (false = unknown id / no-op).
    bool SetDoorOpen(DoorId id, bool open);
    bool SetDoorLocked(DoorId id, bool locked);
    bool SetPowered(SystemId id, bool powered);

    size_t DoorCount() const { return doors_.size(); }
    size_t SystemCount() const { return systems_.size(); }

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    std::vector<DoorState> doors_;
    std::vector<SystemState> systems_;
};

} // namespace writeover