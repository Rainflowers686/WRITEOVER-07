#pragma once
// Profile metadata: cross-save meta state (deaths, loads, endings seen).
// Persisted independently (profile.wo07p) from the world save (M-009 closure).
// Profile never gates deterministic in-level plot.

#include "writeover/common/result.h"
#include "writeover/common/serialize.h"

#include <cstdint>
#include <string>

namespace writeover {

struct ProfileMeta {
    uint32_t schema_version = 1;
    uint32_t death_count = 0;
    uint32_t load_count = 0;
    uint32_t endings_seen = 0;  // bitmask of seen ending indices

    void Save(Serializer& s) const;
    void Load(Deserializer& d);
};

class ProfileStore {
public:
    Result<ProfileMeta> Load(const std::string& path);
    Result<void> Save(const std::string& path, const ProfileMeta& meta);
};

// Shortcut file name (also used by docs/schemas).
inline const char* kProfileFileName = "profile.wo07p";

} // namespace writeover