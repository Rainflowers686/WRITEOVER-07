#include "writeover/narrative/judge.h"

namespace writeover {

void JudgeController::Enable(uint64_t seed) {
    enabled_ = true;
    seed_ = seed;
    has_checkpoint_ = false;
}

void JudgeController::Disable() {
    enabled_ = false;
    has_checkpoint_ = false;
}

void JudgeController::SaveCheckpoint(const JudgeCheckpoint& cp) {
    checkpoint_ = cp;
    has_checkpoint_ = true;
}

const JudgeCheckpoint* JudgeController::GetCheckpoint(uint32_t index) const {
    if (has_checkpoint_ && checkpoint_.index == index) {
        return &checkpoint_;
    }
    return nullptr;
}

bool JudgeController::HasCheckpoint(uint32_t index) const {
    return has_checkpoint_ && checkpoint_.index == index;
}

void JudgeController::Save(Serializer& s) const {
    s.WriteU8(enabled_ ? 1 : 0);
    s.WriteU64(seed_);
    s.WriteU8(has_checkpoint_ ? 1 : 0);
    if (has_checkpoint_) {
        s.WriteU32(checkpoint_.index);
        WriteId(s, checkpoint_.room);
        s.WriteU64(checkpoint_.frame);
        s.WriteU64(checkpoint_.save_slot_seed);
        WriteId(s, checkpoint_.checkpoint_id);
    }
}

void JudgeController::Load(Deserializer& d) {
    enabled_ = d.ReadU8() != 0;
    seed_ = d.ReadU64();
    has_checkpoint_ = d.ReadU8() != 0;
    if (has_checkpoint_) {
        checkpoint_.index = d.ReadU32();
        checkpoint_.room = ReadId<RoomId>(d);
        checkpoint_.frame = d.ReadU64();
        checkpoint_.save_slot_seed = d.ReadU64();
        checkpoint_.checkpoint_id = ReadId<CheckpointId>(d);
    }
}

} // namespace writeover