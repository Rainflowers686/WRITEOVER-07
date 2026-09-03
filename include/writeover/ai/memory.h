#pragma once
// NPC memory: bounded, deterministic (ascending frame order).

#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"

#include <cstdint>
#include <vector>

namespace writeover {

inline constexpr size_t kMaxActiveMemories = 64;

struct MemoryEntry {
    FactId fact_id;
    uint32_t frame = 0;
    float salience = 0.0f;  // 0..1
};

class MemoryStore {
public:
    void Add(const MemoryEntry& entry);
    std::vector<MemoryEntry> Recall(FactId fact_id) const;  // ascending frame
    void Forget(FactId fact_id);
    size_t Count() const { return entries_.size(); }

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    std::vector<MemoryEntry> entries_;
};

} // namespace writeover