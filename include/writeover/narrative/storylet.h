#pragma once
// Storylet contract (M-012 closure): typed conditions and actions. There are
// NO magic a/b/f fields. Text lives in StringIds (content layer), conditions
// reference facts/states/frames/difficulty/flags. Selection is deterministic:
// priority desc, then storylet id asc.

#include "writeover/ai/npc.h"
#include "writeover/common/command.h"
#include "writeover/common/ids.h"
#include "writeover/common/result.h"
#include "writeover/common/serialize.h"
#include "writeover/world/fact_belief.h"

#include <cstdint>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace writeover {

// --- Conditions (typed; no magic fields) ---
struct FactEqualsCondition {
    FactId fact;
    bool equals = true;
};
struct RoomVisitedCondition {
    RoomId room;
};
struct NpcStateCondition {
    NpcId npc;
    NPCState state = NPCState::Idle;
};
struct FrameRangeCondition {
    uint64_t min_frame = 0;
    uint64_t max_frame = 0;
    bool active = true;  // false = no frame constraint
};
struct DifficultyCondition {
    uint8_t min_level = 0;
};
struct FlagCondition {
    std::string flag;
};

using StoryletCondition = std::variant<
    FactEqualsCondition, RoomVisitedCondition, NpcStateCondition,
    FrameRangeCondition, DifficultyCondition, FlagCondition>;

// --- Actions (typed) ---
struct NarratorLineAction {
    std::string text_id;  // StringId at load time; content uses text ids
    uint8_t persona = 0;  // NarratorPersona value
};
struct DialogAction {
    std::string text_id;
};
struct WorldCommandAction {
    WorldCommand command;
};
struct EndGameCommand {
    uint8_t ending_index = 0;
};

using StoryletAction = std::variant<
    NarratorLineAction, DialogAction, WorldCommandAction, EndGameCommand>;

struct Storylet {
    StoryletId id;
    std::string text_id;
    uint16_t priority = 0;
    bool once = true;
    std::vector<StoryletCondition> conditions;
    std::vector<StoryletAction> actions;
};

class StoryletEngine {
public:
    // Register upserts by id and keeps the deterministic sort.
    void Register(const Storylet& story);

    bool HasFired(StoryletId id) const;
    void MarkFired(StoryletId id);

    // First eligible storylet in deterministic order, or nullptr.
    const Storylet* SelectEligible(const FactStore& facts,
                                   const std::vector<RoomId>& visited_rooms,
                                   const std::vector<NPCInstance>& npcs,
                                   const std::set<std::string>& flags,
                                   uint8_t difficulty,
                                   uint64_t frame) const;

    void Save(Serializer& s) const;
    void Load(Deserializer& d);
    // Load compiled binary content (produced by tools/contentc).
    Result<void> LoadBinary(const std::string& path);

    size_t Count() const { return storylets_.size(); }

private:
    std::vector<Storylet> storylets_;   // sorted by priority/id
    std::set<StoryletId> fired_;        // deterministic (ordered set)
};

} // namespace writeover