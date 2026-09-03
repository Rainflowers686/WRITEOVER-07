#include "writeover/narrative/storylet.h"

#include "writeover/common/command.h"
#include "writeover/common/io.h"

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace writeover {

void StoryletEngine::Register(const Storylet& story) {
    for (auto& s : storylets_) {
        if (s.id == story.id) {
            s = story;
            return;
        }
    }
    storylets_.push_back(story);
    std::sort(storylets_.begin(), storylets_.end(),
              [](const Storylet& a, const Storylet& b) {
                  if (a.priority != b.priority) {
                      return a.priority > b.priority;  // higher first
                  }
                  return a.id < b.id;  // stable tie-breaker
              });
}

bool StoryletEngine::HasFired(StoryletId id) const {
    return fired_.count(id) > 0;
}

void StoryletEngine::MarkFired(StoryletId id) { fired_.insert(id); }

namespace {
bool EvalCondition(const StoryletCondition& condition,
                   const FactStore& facts,
                   const std::vector<RoomId>& visited_rooms,
                   const std::vector<NPCInstance>& npcs,
                   const std::set<std::string>& flags,
                   uint8_t difficulty,
                   uint64_t frame) {
    return std::visit(
        [&](const auto& c) -> bool {
            using T = std::decay_t<decltype(c)>;
            if constexpr (std::is_same_v<T, FactEqualsCondition>) {
                WorldFact fact;
                if (!facts.Get(c.fact, fact)) {
                    return false;
                }
                if (std::holds_alternative<bool>(fact.value)) {
                    return std::get<bool>(fact.value) == c.equals;
                }
                return false;
            } else if constexpr (std::is_same_v<T, RoomVisitedCondition>) {
                return std::find(visited_rooms.begin(), visited_rooms.end(), c.room) !=
                       visited_rooms.end();
            } else if constexpr (std::is_same_v<T, NpcStateCondition>) {
                for (const auto& npc : npcs) {
                    if (npc.id == c.npc) {
                        return npc.state == c.state;
                    }
                }
                return false;
            } else if constexpr (std::is_same_v<T, FrameRangeCondition>) {
                return frame >= c.min_frame && frame <= c.max_frame;
            } else if constexpr (std::is_same_v<T, DifficultyCondition>) {
                return difficulty >= c.min_level;
            } else if constexpr (std::is_same_v<T, FlagCondition>) {
                return flags.count(c.flag) > 0;
            } else {
                return false;  // unreachable in practice; keeps MSVC exhaustive
            }
        },
        condition);
}
} // namespace

const Storylet* StoryletEngine::SelectEligible(
    const FactStore& facts,
    const std::vector<RoomId>& visited_rooms,
    const std::vector<NPCInstance>& npcs,
    const std::set<std::string>& flags,
    uint8_t difficulty,
    uint64_t frame) const {
    for (const auto& storylet : storylets_) {
        if (storylet.once && HasFired(storylet.id)) {
            continue;
        }
        bool all_pass = true;
        for (const auto& condition : storylet.conditions) {
            if (!EvalCondition(condition, facts, visited_rooms, npcs, flags,
                               difficulty, frame)) {
                all_pass = false;
                break;
            }
        }
        if (all_pass) {
            return &storylet;
        }
    }
    return nullptr;
}

void StoryletEngine::Save(Serializer& s) const {
    s.WriteU32(static_cast<uint32_t>(storylets_.size()));
    for (const auto& storylet : storylets_) {
        WriteId(s, storylet.id);
        s.WriteString(storylet.text_id);
        s.WriteU16(storylet.priority);
        s.WriteU8(storylet.once ? 1 : 0);
        s.WriteU32(static_cast<uint32_t>(storylet.conditions.size()));
        for (const auto& c : storylet.conditions) {
            s.WriteU8(static_cast<uint8_t>(c.index()));
            std::visit([&s](const auto& cond) {
                using T = std::decay_t<decltype(cond)>;
                if constexpr (std::is_same_v<T, FactEqualsCondition>) {
                    WriteId(s, cond.fact);
                    s.WriteU8(cond.equals ? 1 : 0);
                } else if constexpr (std::is_same_v<T, RoomVisitedCondition>) {
                    WriteId(s, cond.room);
                } else if constexpr (std::is_same_v<T, NpcStateCondition>) {
                    WriteId(s, cond.npc);
                    s.WriteU8(static_cast<uint8_t>(cond.state));
                } else if constexpr (std::is_same_v<T, FrameRangeCondition>) {
                    s.WriteU64(cond.min_frame);
                    s.WriteU64(cond.max_frame);
                } else if constexpr (std::is_same_v<T, DifficultyCondition>) {
                    s.WriteU8(cond.min_level);
                } else if constexpr (std::is_same_v<T, FlagCondition>) {
                    s.WriteString(cond.flag);
                }
            }, c);
        }
        s.WriteU32(static_cast<uint32_t>(storylet.actions.size()));
        for (const auto& a : storylet.actions) {
            s.WriteU8(static_cast<uint8_t>(a.index()));
            std::visit([&s](const auto& act) {
                using T = std::decay_t<decltype(act)>;
                if constexpr (std::is_same_v<T, NarratorLineAction>) {
                    s.WriteString(act.text_id);
                    s.WriteU8(act.persona);
                } else if constexpr (std::is_same_v<T, DialogAction>) {
                    s.WriteString(act.text_id);
                } else if constexpr (std::is_same_v<T, WorldCommandAction>) {
                    SerializeWorldCommand(s, act.command);
                } else if constexpr (std::is_same_v<T, EndGameCommand>) {
                    s.WriteU8(act.ending_index);
                }
            }, a);
        }
    }
    s.WriteU32(static_cast<uint32_t>(fired_.size()));
    for (const auto id : fired_) {
        WriteId(s, id);
    }
}

void StoryletEngine::Load(Deserializer& d) {
    storylets_.clear();
    fired_.clear();
    const uint32_t storylet_count = d.ReadU32();
    for (uint32_t i = 0; i < storylet_count; ++i) {
        Storylet storylet;
        storylet.id = ReadId<StoryletId>(d);
        storylet.text_id = d.ReadString();
        storylet.priority = d.ReadU16();
        storylet.once = d.ReadU8() != 0;
        const uint32_t condition_count = d.ReadU32();
        if (condition_count > 64) {
            d.ReadU64();
            continue;
        }
        for (uint32_t j = 0; j < condition_count; ++j) {
            const uint8_t index = d.ReadU8();
            switch (index) {
            case 0: {
                FactEqualsCondition c;
                c.fact = ReadId<FactId>(d);
                c.equals = d.ReadU8() != 0;
                storylet.conditions.push_back(c);
                break;
            }
            case 1: {
                RoomVisitedCondition c;
                c.room = ReadId<RoomId>(d);
                storylet.conditions.push_back(c);
                break;
            }
            case 2: {
                NpcStateCondition c;
                c.npc = ReadId<NpcId>(d);
                c.state = static_cast<NPCState>(d.ReadU8());
                storylet.conditions.push_back(c);
                break;
            }
            case 3: {
                FrameRangeCondition c;
                c.min_frame = d.ReadU64();
                c.max_frame = d.ReadU64();
                storylet.conditions.push_back(c);
                break;
            }
            case 4: {
                DifficultyCondition c;
                c.min_level = d.ReadU8();
                storylet.conditions.push_back(c);
                break;
            }
            case 5: {
                FlagCondition c;
                c.flag = d.ReadString();
                storylet.conditions.push_back(c);
                break;
            }
            default:
                d.ReadU64();
                break;
            }
        }
        const uint32_t action_count = d.ReadU32();
        if (action_count > 32) {
            d.ReadU64();
            continue;
        }
        for (uint32_t j = 0; j < action_count; ++j) {
            const uint8_t index = d.ReadU8();
            switch (index) {
            case 0: {
                NarratorLineAction a;
                a.text_id = d.ReadString();
                a.persona = d.ReadU8();
                storylet.actions.push_back(a);
                break;
            }
            case 1: {
                DialogAction a;
                a.text_id = d.ReadString();
                storylet.actions.push_back(a);
                break;
            }
            case 2: {
                WorldCommandAction a;
                a.command = DeserializeWorldCommand(d);
                storylet.actions.push_back(a);
                break;
            }
            case 3: {
                EndGameCommand a;
                a.ending_index = d.ReadU8();
                storylet.actions.push_back(a);
                break;
            }
            default:
                d.ReadU64();
                break;
            }
        }
        storylets_.push_back(std::move(storylet));
    }
    std::sort(storylets_.begin(), storylets_.end(),
              [](const Storylet& a, const Storylet& b) {
                  if (a.priority != b.priority) {
                      return a.priority > b.priority;
                  }
                  return a.id < b.id;
              });
    // Only read fired state if it exists in the stream (content files
    // may only contain definitions). This fixes F-08.
    if (d.Remaining() >= 4) {
        const uint32_t fired_count = d.ReadU32();
        if (d.Remaining() >= fired_count * 8) {  // StoryletId is 8 bytes
            for (uint32_t i = 0; i < fired_count; ++i) {
                fired_.insert(ReadId<StoryletId>(d));
            }
        }
    }
}

Result<void> StoryletEngine::LoadBinary(const std::string& path) {
    auto data = ReadFileBinary(path);
    if (data.IsError()) {
        return Result<void>::Err(300, "cannot read storylet content: " + path);
    }
    const std::vector<uint8_t>& bytes = data.Value();
    if (bytes.size() < 8) {
        return Result<void>::Err(302, "storylet content truncated: " + path);
    }
    // Skip the 8-byte compiled-content header (magic + version), then parse
    // the payload with the shared Load format.
    Deserializer d(bytes.data() + 8, bytes.size() - 8);
    Load(d);
    if (d.HasError()) {
        return Result<void>::Err(301, "storylet content corrupted: " + path);
    }
    return Result<void>::Ok();
}

} // namespace writeover