#include "writeover/core/profile.h"

#include "writeover/common/io.h"
#include "writeover/common/serialize.h"

#include <string>
#include <utility>
#include <vector>

namespace writeover {

void ProfileMeta::Save(Serializer& s) const {
    s.WriteU32(schema_version);
    s.WriteU32(death_count);
    s.WriteU32(load_count);
    s.WriteU32(endings_seen);
}

void ProfileMeta::Load(Deserializer& d) {
    schema_version = d.ReadU32();
    death_count = d.ReadU32();
    load_count = d.ReadU32();
    endings_seen = d.ReadU32();
}

Result<ProfileMeta> ProfileStore::Load(const std::string& path) {
    auto data = ReadFileBinary(path);
    if (data.IsError()) {
        return Result<ProfileMeta>::Err(200, "cannot read profile: " + path);
    }
    Deserializer d(data.Value().data(), data.Value().size());
    ProfileMeta meta;
    meta.Load(d);
    if (d.HasError() || meta.schema_version != 1) {
        return Result<ProfileMeta>::Err(201, "profile corrupted or unsupported");
    }
    return Result<ProfileMeta>::Ok(meta);
}

Result<void> ProfileStore::Save(const std::string& path, const ProfileMeta& meta) {
    std::vector<uint8_t> buffer;
    {
        Serializer s(buffer);
        meta.Save(s);
    }
    const std::string tmp = path + ".tmp";
    auto written = WriteFileBinary(tmp, buffer);
    if (written.IsError()) {
        return written;
    }
    return ReplaceFileAtomic(tmp, path);
}

} // namespace writeover