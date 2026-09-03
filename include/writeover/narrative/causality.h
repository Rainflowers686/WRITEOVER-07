#pragma once
// Causality ledger: bounded ring of dispatched events (parent links kept).
// Consumed by the F3 panel and judge mode; read-only from other modules.

#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"
#include "writeover/common/world_event.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace writeover {

inline constexpr size_t kCausalityLedgerCapacity = 500;

struct CausalityEntry {
    EventId event_id;
    EventId parent_event_id;
    uint64_t sim_frame = 0;
    EventKind kind = EventKind::Notification;
};

class CausalityLedger {
public:
    void Push(const CausalityEntry& entry);
    // Newest entry, or nullptr when empty.
    const CausalityEntry* Latest() const;
    // Newest-first view, at most n entries (read-only; never consumes).
    std::vector<CausalityEntry> Recent(size_t n) const;

    size_t Count() const { return count_; }

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    std::array<CausalityEntry, kCausalityLedgerCapacity> ring_{};
    size_t head_ = 0;
    size_t count_ = 0;
};

} // namespace writeover