#include "writeover/narrative/causality.h"

#include <algorithm>

namespace writeover {

void CausalityLedger::Push(const CausalityEntry& entry) {
    if (count_ < kCausalityLedgerCapacity) {
        const size_t write_index = (head_ + count_) % kCausalityLedgerCapacity;
        ring_[write_index] = entry;
        ++count_;
        return;
    }
    ring_[head_] = entry;
    head_ = (head_ + 1) % kCausalityLedgerCapacity;
}

const CausalityEntry* CausalityLedger::Latest() const {
    if (count_ == 0) {
        return nullptr;
    }
    const size_t tail = (head_ + count_ - 1) % kCausalityLedgerCapacity;
    return &ring_[tail];
}

std::vector<CausalityEntry> CausalityLedger::Recent(size_t n) const {
    std::vector<CausalityEntry> out;
    const size_t take = std::min(n, count_);
    out.reserve(take);
    for (size_t i = 0; i < take; ++i) {
        const size_t index = (head_ + count_ - take + i) % kCausalityLedgerCapacity;
        out.push_back(ring_[index]);
    }
    return out;
}

void CausalityLedger::Save(Serializer& s) const {
    s.WriteU32(static_cast<uint32_t>(count_));
    for (size_t i = 0; i < count_; ++i) {
        const size_t index = (head_ + i) % kCausalityLedgerCapacity;
        const CausalityEntry& e = ring_[index];
        WriteId(s, e.event_id);
        WriteId(s, e.parent_event_id);
        s.WriteU64(e.sim_frame);
        s.WriteU8(static_cast<uint8_t>(e.kind));
    }
}

void CausalityLedger::Load(Deserializer& d) {
    head_ = 0;
    count_ = 0;
    const uint32_t count = d.ReadU32();
    if (count > kCausalityLedgerCapacity) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        CausalityEntry e;
        e.event_id = ReadId<EventId>(d);
        e.parent_event_id = ReadId<EventId>(d);
        e.sim_frame = d.ReadU64();
        e.kind = static_cast<EventKind>(d.ReadU8());
        ring_[i] = e;
        ++count_;
    }
}

} // namespace writeover