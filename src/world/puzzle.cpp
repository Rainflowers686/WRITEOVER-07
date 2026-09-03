#include "writeover/world/puzzle.h"

namespace writeover {

void PuzzleSystem::Register(const PuzzleInstance& puzzle) {
    for (auto& p : puzzles_) {
        if (p.content_id == puzzle.content_id) {
            p = puzzle;
            return;
        }
    }
    puzzles_.push_back(puzzle);
}

PuzzleState PuzzleSystem::GetState(const StringId& content_id) const {
    for (const auto& p : puzzles_) {
        if (p.content_id == content_id) {
            return p.state;
        }
    }
    return PuzzleState::Locked;
}

void PuzzleSystem::SetState(const StringId& content_id, PuzzleState state) {
    for (auto& p : puzzles_) {
        if (p.content_id == content_id) {
            p.state = state;
            return;
        }
    }
    Register(PuzzleInstance{content_id, state});
}

void PuzzleSystem::Save(Serializer& s) const {
    s.WriteU32(static_cast<uint32_t>(puzzles_.size()));
    for (const auto& p : puzzles_) {
        WriteId(s, p.content_id);
        s.WriteU8(static_cast<uint8_t>(p.state));
    }
}

void PuzzleSystem::Load(Deserializer& d) {
    puzzles_.clear();
    const uint32_t count = d.ReadU32();
    for (uint32_t i = 0; i < count; ++i) {
        PuzzleInstance p;
        p.content_id = ReadId<StringId>(d);
        p.state = static_cast<PuzzleState>(d.ReadU8());
        puzzles_.push_back(p);
    }
}

} // namespace writeover