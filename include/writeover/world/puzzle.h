#pragma once
// Puzzle system state (P0: 2 system puzzles + <=4 small puzzles, data-driven).

#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"

#include <cstdint>
#include <vector>

namespace writeover {

enum class PuzzleState : uint8_t {
    Locked = 0,
    InProgress = 1,
    Solved = 2,
};

struct PuzzleInstance {
    StringId content_id;
    PuzzleState state = PuzzleState::Locked;
};

class PuzzleSystem {
public:
    void Register(const PuzzleInstance& puzzle);
    PuzzleState GetState(const StringId& content_id) const;
    void SetState(const StringId& content_id, PuzzleState state);
    size_t Count() const { return puzzles_.size(); }

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    std::vector<PuzzleInstance> puzzles_;
};

} // namespace writeover