#include "tests/test_harness.h"

#include "writeover/ai/goap.h"
#include "writeover/ai/memory.h"
#include "writeover/ai/perception.h"
#include "writeover/world/grid.h"

#include <array>

namespace writeover {

namespace {

Grid MakeViewGrid() {
    Grid grid(8, 6);
    for (int32_t r = 0; r < 6; ++r) {
        for (int32_t c = 0; c < 8; ++c) {
            grid.SetCell(c, r, GridCell{});
        }
    }
    return grid;
}

bool PerceptionSeesInOpen() {
    const Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    NPCInstance npc;
    npc.id = NpcId::New(1);
    npc.position = Vec3{1.5f, 1.5f, 0.0f};
    npc.yaw = 1.57079632679f;  // facing +y
    PerceptionSystem system;
    const auto result = system.Update(npc, query, Vec3{1.5f, 4.0f, 0.0f},
                                      1.0f, {}, 10);
    return result.sees_player;
}

bool PerceptionBlockedByWall() {
    Grid grid = MakeViewGrid();
    for (int32_t r = 0; r < 6; ++r) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(4, r, wall);  // vertical wall col 4
    }
    GridWorldQuery query(&grid);
    NPCInstance npc;
    npc.id = NpcId::New(1);
    npc.position = Vec3{1.5f, 3.5f, 0.0f};
    npc.yaw = 0.0f;  // facing +x into the wall
    PerceptionSystem system;
    const auto result = system.Update(npc, query, Vec3{6.5f, 3.5f, 0.0f},
                                      1.0f, {}, 10);
    return !result.sees_player;
}

bool PerceptionHearsNoise() {
    const Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    NPCInstance npc;
    npc.id = NpcId::New(1);
    npc.position = Vec3{1.5f, 1.5f, 0.0f};
    npc.yaw = 0.0f;
    PerceptionSystem system;
    std::vector<NoiseSource> noises;
    noises.push_back(NoiseSource{Vec3{2.5f, 1.5f, 0.0f}, 0.9f, 9});
    const auto result = system.Update(npc, query, Vec3{10.0f, 10.0f, 0.0f},
                                      1.0f, noises, 10);
    return result.hears_noise;
}

bool GoapPlansSimpleChain() {
    FactStore facts;
    facts.Set(WorldFact{FactId::New(1), EntityId::New(0), PredicateType::State, false});
    facts.Set(WorldFact{FactId::New(2), EntityId::New(0), PredicateType::State, false});

    std::array<GOAPAction, kGoapActionArraySize> actions{};
    for (auto& a : actions) {
        a = GOAPAction{};
    }
    actions[0] = GOAPAction{ActionType::MoveTo, "move", {}, {}, 1.0f, 1.0f};
    actions[1] = GOAPAction{ActionType::Report, "report",
                            {}, {{FactId::New(1), true}}, 1.0f, 0.5f};
    actions[2] = GOAPAction{ActionType::UseTerminal, "use_terminal",
                            {{FactId::New(1), true}}, {{FactId::New(2), true}}, 2.0f, 1.0f};

    const std::vector<PlanStep> plan =
        PlanGoap({{FactId::New(2), true}}, facts, actions, 4);
    WO_CHECK(!plan.empty());
    if (plan.empty()) {
        return false;
    }
    // The final step must be the action achieving the goal.
    return plan.back().action.type == ActionType::UseTerminal;
}

bool GoapNoPlanWhenImpossible() {
    FactStore facts;
    facts.Set(WorldFact{FactId::New(1), EntityId::New(0), PredicateType::State, false});
    std::array<GOAPAction, kGoapActionArraySize> actions{};
    for (auto& a : actions) {
        a = GOAPAction{};
    }
    // Gap: no action sets fact 1, goal demands it.
    const std::vector<PlanStep> plan =
        PlanGoap({{FactId::New(1), true}}, facts, actions, 3);
    return plan.empty();
}

bool MemoryRecallOrdered() {
    MemoryStore memory;
    memory.Add(MemoryEntry{FactId::New(1), 10, 0.5f});
    memory.Add(MemoryEntry{FactId::New(2), 20, 0.8f});
    memory.Add(MemoryEntry{FactId::New(1), 30, 0.9f});
    const auto recalled = memory.Recall(FactId::New(1));
    WO_CHECK_EQ(static_cast<int64_t>(recalled.size()), 2);
    // Oldest first (ascending frame).
    return !recalled.empty() && recalled[0].frame == 10;
}

} // namespace

void RegisterAiTests(TestHarness& test) {
    test.Add("ai.perception_sees_open", &PerceptionSeesInOpen);
    test.Add("ai.perception_blocked_wall", &PerceptionBlockedByWall);
    test.Add("ai.perception_hears_noise", &PerceptionHearsNoise);
    test.Add("ai.goap_plans_chain", &GoapPlansSimpleChain);
    test.Add("ai.goap_no_plan_impossible", &GoapNoPlanWhenImpossible);
    test.Add("ai.memory_recall_order", &MemoryRecallOrdered);
}

} // namespace writeover