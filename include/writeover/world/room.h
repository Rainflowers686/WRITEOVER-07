#pragma once
// Room: content-defined world data loaded from the compiled (.woc) binary
// produced by tools/contentc/contentc.py. Runtime never parses JSON
// (M-015 closure: authoring JSON -> Python stdlib compiler -> deterministic
// binary; release C++ reads binary only).

#include "writeover/common/ids.h"
#include "writeover/common/result.h"
#include "writeover/common/serialize.h"
#include "writeover/world/grid.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

inline constexpr uint32_t kWocMagic = 0x574F4331;  // "WOC1"
inline constexpr uint32_t kWocVersion = 1;

struct Room {
    RoomId id;
    std::string display_name;      // UTF-8, content-facing display text
    Grid grid;
    Vec3 spawn_point;
    float spawn_yaw = 0.0f;
    std::vector<StringId> npc_refs;
    std::vector<StringId> storylet_refs;
};

// Codec for the compiled room format (also used by mapc + tests).
void SerializeRoom(Serializer& s, const Room& room);
Result<Room> DeserializeRoom(Deserializer& d);

// Loads a .woc room file from disk.
Result<Room> LoadRoomFile(const std::string& path);

} // namespace writeover