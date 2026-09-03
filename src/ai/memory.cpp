#include "writeover/ai/memory.h"

#include <algorithm>

namespace writeover {

void MemoryStore::Add(const MemoryEntry& entry) {
    entries_.push_back(entry);
    std::sort(entries_.begin(), entries_.end(),
              [](const MemoryEntry& a, const MemoryEntry& b) {
                  return a.frame < b.frame;
              });
    if (entries_.size() > kMaxActiveMemories) {
        entries_.erase(entries_.begin(),
                       entries_.end() - static_cast<std::ptrdiff_t>(kMaxActiveMemories));
    }
}

std::vector<MemoryEntry> MemoryStore::Recall(FactId fact_id) const {
    std::vector<MemoryEntry> matches;
    for (const auto& e : entries_) {
        if (e.fact_id == fact_id) {
            matches.push_back(e);
        }
    }
    return matches;
}

void MemoryStore::Forget(FactId fact_id) {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                  [fact_id](const MemoryEntry& e) {
                                      return e.fact_id == fact_id;
                                  }),
                   entries_.end());
}

void MemoryStore::Save(Serializer& s) const {
    s.WriteU32(static_cast<uint32_t>(entries_.size()));
    for (const auto& e : entries_) {
        WriteId(s, e.fact_id);
        s.WriteU32(e.frame);
        s.WriteF32(e.salience);
    }
}

void MemoryStore::Load(Deserializer& d) {
    entries_.clear();
    const uint32_t count = d.ReadU32();
    entries_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        MemoryEntry e;
        e.fact_id = ReadId<FactId>(d);
        e.frame = d.ReadU32();
        e.salience = d.ReadF32();
        entries_.push_back(e);
    }
}

} // namespace writeover