#pragma once
// GOAP-lite for Full NPCs (frozen constraints: <=3 candidate goals, <=10
// actions, plan depth <=5, replan <=5Hz / on event). Best-first search over
// fact predicates; deterministic iteration order (actions sorted by cost).

#include "writeover/ai/npc.h"
#include "writeover/world/fact_belief.h"

#include <array>
#include <cstdint>
#include <vector>

namespace writeover {

enum class ActionType : uint8_t {
    MoveTo = 0,
    Investigate = 1,
    Report = 2,
    Attack = 3,
    Reload = 4,
    TakeCover = 5,
    OpenDoor = 6,
    UseTerminal = 7,
    GuardPoint = 8,
    Communicate = 9,
    Count = 10,
};

inline constexpr int kGoapMaxActions = 10;
inline constexpr int kGoapMaxDepth = 5;
inline constexpr size_t kGoapActionArraySize = static_cast<size_t>(ActionType::Count);

struct FactPredicate {
    FactId fact_id;
    bool desired_value = true;
};

struct GOAPAction {
    ActionType type = ActionType::MoveTo;
    const char* name = "";
    std::vector<FactPredicate> preconditions;
    std::vector<FactPredicate> effects;
    float cost = 1.0f;
    float duration_sec = 0.0f;
};

struct PlanStep {
    GOAPAction action;
    Vec3 target_pos;
};

// Returns an empty plan when no plan with depth <= max_depth exists.
std::vector<PlanStep> PlanGoap(const std::vector<FactPredicate>& goals,
                               const FactStore& facts,
                               const std::array<GOAPAction, kGoapActionArraySize>& actions,
                               int max_depth = kGoapMaxDepth);

} // namespace writeover