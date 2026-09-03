#pragma once
// Judge mode: fixed lawful seed + true checkpoints + hotkey jump (judge only).

#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"

#include <cstdint>

namespace writeover {

struct JudgeCheckpoint {
    uint32_t index = 0;
    RoomId room;
    uint64_t frame = 0;
    uint64_t save_slot_seed = 0;
    CheckpointId checkpoint_id;
};

class JudgeController {
public:
    void Enable(uint64_t seed);
    void Disable();
    bool IsEnabled() const { return enabled_; }
    uint64_t Seed() const { return seed_; }

    void SaveCheckpoint(const JudgeCheckpoint& cp);
    const JudgeCheckpoint* GetCheckpoint(uint32_t index) const;
    bool HasCheckpoint(uint32_t index) const;

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    bool enabled_ = false;
    uint64_t seed_ = 0xDEADBEEF;
    JudgeCheckpoint checkpoint_;
    bool has_checkpoint_ = false;
};

} // namespace writeover