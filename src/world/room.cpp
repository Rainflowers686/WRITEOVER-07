#include "writeover/world/room.h"

#include "writeover/common/io.h"

namespace writeover {

void SerializeRoom(Serializer& s, const Room& room) {
    s.WriteU32(kWocMagic);
    s.WriteU32(kWocVersion);
    WriteId(s, room.id);
    s.WriteString(room.display_name);  // UTF-8 text, matches contentc.py
    s.WriteI32(room.grid.Width());
    s.WriteI32(room.grid.Height());
    s.WriteF32(room.spawn_point.x);
    s.WriteF32(room.spawn_point.y);
    s.WriteF32(room.spawn_point.z);
    s.WriteF32(room.spawn_yaw);
    const auto& cells = room.grid.Data();
    s.WriteU32(static_cast<uint32_t>(cells.size()));
    for (const auto& cell : cells) {
        s.WriteF32(cell.floor_height);
        s.WriteF32(cell.ceiling_height);
        s.WriteU8(cell.material);
        s.WriteU8(cell.light);
        s.WriteU8(cell.flags);
    }
}

Result<Room> DeserializeRoom(Deserializer& d) {
    const uint32_t magic = d.ReadU32();
    const uint32_t version = d.ReadU32();
    if (magic != kWocMagic) {
        return Result<Room>::Err(kWocMagic, "not a compiled room file (bad magic)");
    }
    if (version != kWocVersion) {
        return Result<Room>::Err(kWocMagic + 1, "unsupported room format version");
    }

    Room room;
    room.id = ReadId<RoomId>(d);
    room.display_name = d.ReadString();
    const int32_t w = d.ReadI32();
    const int32_t h = d.ReadI32();
    if (w <= 0 || h <= 0 || w > 256 || h > 256) {
        return Result<Room>::Err(kWocMagic + 2, "invalid room dimensions");
    }
    room.spawn_point.x = d.ReadF32();
    room.spawn_point.y = d.ReadF32();
    room.spawn_point.z = d.ReadF32();
    room.spawn_yaw = d.ReadF32();

    room.grid = Grid(w, h);
    const uint32_t cell_count = d.ReadU32();
    if (cell_count != static_cast<uint32_t>(w * h)) {
        return Result<Room>::Err(kWocMagic + 3, "room cell count mismatch");
    }
    auto& cells = room.grid.Data();
    cells.resize(cell_count);
    for (auto& cell : cells) {
        cell.floor_height = d.ReadF32();
        cell.ceiling_height = d.ReadF32();
        cell.material = d.ReadU8();
        cell.light = d.ReadU8();
        cell.flags = d.ReadU8();
    }

    if (d.HasError()) {
        return Result<Room>::Err(kWocMagic + 4, "room file truncated or corrupted");
    }
    return Result<Room>::Ok(std::move(room));
}

Result<Room> LoadRoomFile(const std::string& path) {
    auto data = ReadFileBinary(path);
    if (data.IsError()) {
        return Result<Room>::Err(kWocMagic + 5, "cannot read room file: " + path);
    }
    // Skip the 8-byte WOC header handled in DeserializeRoom; parser reads all.
    Deserializer d(data.Value().data(), data.Value().size());
    return DeserializeRoom(d);
}

} // namespace writeover