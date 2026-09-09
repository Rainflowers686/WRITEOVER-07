#include "tests/test_harness.h"

#include "writeover/ai/goap.h"
#include "writeover/ai/memory.h"
#include "writeover/ai/perception.h"
#include "writeover/ai/runtime.h"
#include "writeover/common/rng.h"
#include "writeover/common/world_event.h"
#include "writeover/systemic/systemic.h"
#include "writeover/world/grid.h"

#include <array>
#include <cmath>
#include <cstring>

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

bool PerceptionWallMufflesNoise() {
    Grid grid = MakeViewGrid();
    for (int32_t row = 0; row < grid.Height(); ++row) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(4, row, wall);
    }
    GridWorldQuery query(&grid);
    NPCInstance npc;
    npc.id = NpcId::New(2);
    npc.position = Vec3{1.5f, 3.5f, 0.0f};
    npc.yaw = 0.0f;
    PerceptionSystem system;
    const std::vector<NoiseSource> noises = {
        NoiseSource{Vec3{6.5f, 3.5f, kEyeStand}, 0.9f, 9}};
    const auto result = system.Update(npc, query, Vec3{7.0f, 5.0f, 0.0f},
                                      kEyeStand, noises, 10);
    // The noise remains potentially audible, but its confidence is reduced
    // by the intervening wall rather than treating hearing as omniscient.
    return result.hears_noise && result.noise_loudness < 0.2f;
}

bool PerceptionIgnoresFutureNoiseTimestamp() {
    const Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    NPCInstance npc;
    npc.id = NpcId::New(3);
    npc.position = Vec3{1.5f, 1.5f, 0.0f};
    npc.yaw = 0.0f;
    const std::vector<NoiseSource> noises = {
        NoiseSource{Vec3{2.5f, 1.5f, 0.0f}, 1.0f, 100}};
    const auto result = PerceptionSystem{}.Update(
        npc, query, Vec3{7.0f, 5.0f, 0.0f}, kEyeStand, noises, 10);
    // A future timestamp must not wrap into a very old but audible sound.
    return !result.hears_noise && result.noise_loudness == 0.0f;
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

bool AutonomousRuntimeRunsFivePhaseLoop() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    ActorRecord actor;
    actor.id = NpcId::New(7);
    actor.data_key = ResourceId::New(7007);
    actor.faction = Faction::Security;
    actor.cognition = CognitionTier::Full;
    actor.role = Role::Guard;
    WO_CHECK(systemic.AddActor(actor));

    EventBus events;
    DeterministicRNG rng(0x1234);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);

    NPCInstance npc;
    npc.id = NpcId::New(7);
    npc.cognition = CognitionTier::Full;
    npc.faction = Faction::Security;
    npc.role = Role::Guard;
    npc.data_key = ResourceId::New(7007);
    npc.position = Vec3{1.5f, 1.5f, 0.0f};
    npc.yaw = 0.0f;  // face the player on +x
    npc.health = 100;
    WO_CHECK(runtime.AddNpc(npc, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{4.5f, 1.5f, 0.0f}, kEyeStand);

    // The fixed cadence must not run early, then must run all five phases
    // against the real perception, systemic memory, and EventBus adapters.
    runtime.Tick(11);
    WO_CHECK_EQ(static_cast<int64_t>(runtime.AutonomousLoopCount()), 0);
    runtime.Tick(12);
    WO_CHECK_EQ(static_cast<int64_t>(runtime.AutonomousLoopCount()), 1);
    WO_CHECK(systemic.MemoryCount() == 1);
    // EventBus defers events posted during this tick into next_pending_; the
    // first dispatch advances them to the next tick by contract.
    WO_CHECK(events.PendingCount() == 0);
    WO_CHECK(runtime.Npcs().size() == 1);
    WO_CHECK(runtime.Npcs().front().instance.state == NPCState::Combat);

    bool saw[5] = {false, false, false, false, false};
    for (const auto& receipt : runtime.Receipts()) {
        if (receipt.npc == NpcId::New(7) && receipt.frame == 12) {
            saw[static_cast<size_t>(receipt.phase)] = true;
        }
    }
    for (bool phase_seen : saw) WO_CHECK(phase_seen);

    events.Dispatch();
    WO_CHECK(events.PendingCount() >= 2);  // state change + Full-human speech
    events.Dispatch();
    WO_CHECK(events.JournalCount() >= 2);
    bool saw_player_damage = false;
    for (const auto& event : events.JournalSnapshot()) {
        const auto* damage = std::get_if<EventPlayerDamage>(&event.payload);
        if (damage != nullptr && event.target_entity == EntityId::New(1) &&
            damage->amount == 8) {
            saw_player_damage = true;
            break;
        }
    }
    // The loop must emit a real NPC -> player consequence, not only an Alert
    // state mutation or a receipt count.
    WO_CHECK(saw_player_damage);
    return true;
}

int CountNpcSpeakEvents(const EventBus& events, NpcId npc) {
    int count = 0;
    for (const auto& event : events.JournalSnapshot()) {
        const auto* speech = std::get_if<EventNpcSpeak>(&event.payload);
        if (speech != nullptr && speech->npc == npc) ++count;
    }
    return count;
}

int CountPlayerDamageEvents(const EventBus& events) {
    int count = 0;
    for (const auto& event : events.JournalSnapshot()) {
        if (std::get_if<EventPlayerDamage>(&event.payload) != nullptr) ++count;
    }
    return count;
}

void DispatchDeferred(EventBus& events) {
    events.Dispatch();
    events.Dispatch();
}

bool AutonomousPatrolUsesGridRouteAndMotor() {
    Grid grid = MakeViewGrid();
    for (int32_t row = 1; row < 5; ++row) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(3, row, wall);
    }
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    EventBus events;
    DeterministicRNG rng(0x401);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance npc;
    npc.id = NpcId::New(401);
    npc.position = Vec3{1.5f, 2.5f, 0.0f};
    npc.yaw = 0.0f;
    npc.state = NPCState::Patrol;
    WO_CHECK(runtime.AddNpc(npc, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{1.5f, 5.5f, 0.0f}, kEyeStand);
    WO_CHECK(runtime.SetPatrolRoute(npc.id,
                                    {Vec3{5.5f, 2.5f, 0.0f},
                                     Vec3{1.5f, 2.5f, 0.0f}}));

    const Vec3 start = runtime.Npcs().front().instance.position;
    for (uint64_t frame = 0; frame <= 720; ++frame) runtime.Tick(frame);
    const Vec3 end = runtime.Npcs().front().instance.position;
    WO_CHECK(std::fabs(end.x - start.x) > 0.25f || std::fabs(end.y - start.y) > 0.25f);
    // The route must use the open row around the wall; no motor step may
    // place the actor in a solid cell or teleport through the barrier.
    WO_CHECK(end.y < 1.5f || end.y > 4.5f || end.x < 3.0f || end.x > 4.0f);
    WO_CHECK(!query.AabbBlocked(AABB{{end.x - 0.42f, end.y - 0.42f, 0.0f},
                                     {end.x + 0.42f, end.y + 0.42f, 1.8f}}));
    return true;
}

bool AutonomousMotorKeepsPersonalSpaceFromPlayer() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    EventBus events;
    DeterministicRNG rng(0x40a);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);

    NPCInstance npc;
    npc.id = NpcId::New(410);
    npc.position = Vec3{1.5f, 3.5f, 0.0f};
    npc.state = NPCState::Patrol;
    WO_CHECK(runtime.AddNpc(npc, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    // Keep perception out of this counterfactual.  The player is still an
    // authoritative dynamic obstacle for the motor, even when the NPC has
    // no reason to react to the player's presence.
    runtime.SetPlayerPose(Vec3{3.5f, 3.5f, 0.0f}, kEyeStand);
    runtime.SetPlayerVisibility(0.0f);
    WO_CHECK(runtime.SetPatrolRoute(npc.id,
                                    {Vec3{6.5f, 3.5f, 0.0f},
                                     Vec3{1.5f, 3.5f, 0.0f}}));

    const Vec3 start = runtime.Npcs().front().instance.position;
    for (uint64_t frame = 0; frame <= 720; ++frame) {
        runtime.Tick(frame);
        const Vec3 position = runtime.Npcs().front().instance.position;
        const float dx = position.x - 3.5f;
        const float dy = position.y - 3.5f;
        // 0.42 m NPC radius + 0.35 m player radius, with a small numerical
        // margin.  The motor may route around the player, but never enter it.
        WO_CHECK(dx * dx + dy * dy >= 0.77f * 0.77f - 0.0001f);
    }
    const Vec3 end = runtime.Npcs().front().instance.position;
    WO_CHECK(std::fabs(end.x - start.x) > 0.25f ||
             std::fabs(end.y - start.y) > 0.25f);
    return true;
}

bool AutonomousGuardCombatRepeatsAndHonoursLineOfSight() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    ActorRecord actor;
    actor.id = NpcId::New(402);
    actor.role = Role::Guard;
    actor.faction = Faction::Security;
    actor.cognition = CognitionTier::SemiHuman;
    WO_CHECK(systemic.AddActor(actor));
    EventBus events;
    DeterministicRNG rng(0x402);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance guard;
    guard.id = NpcId::New(402);
    guard.role = Role::Guard;
    guard.faction = Faction::Security;
    guard.position = Vec3{1.5f, 1.5f, 0.0f};
    guard.yaw = 0.0f;
    guard.health = 100;
    WO_CHECK(runtime.AddNpc(guard, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{4.5f, 1.5f, 0.0f}, kEyeStand);

    runtime.Tick(12);
    DispatchDeferred(events);
    WO_CHECK_EQ(CountPlayerDamageEvents(events), 1);
    runtime.Tick(24);
    DispatchDeferred(events);
    WO_CHECK_EQ(CountPlayerDamageEvents(events), 1);
    runtime.Tick(72);
    DispatchDeferred(events);
    WO_CHECK_EQ(CountPlayerDamageEvents(events), 2);

    GridCell wall;
    wall.flags = CellFlag_Solid;
    grid.SetCell(2, 1, wall);
    runtime.Tick(84);
    DispatchDeferred(events);
    WO_CHECK_EQ(CountPlayerDamageEvents(events), 2);
    grid.SetCell(2, 1, GridCell{});
    runtime.Tick(132);
    DispatchDeferred(events);
    WO_CHECK_EQ(CountPlayerDamageEvents(events), 3);
    WO_CHECK(runtime.GuardAttackCount() == 3);

    // A dead/restarting player is not a valid combat target.  This is a
    // counterfactual against the stale-pose failure mode: the guard may still
    // have a previous Combat state, but it must not emit another attack.
    runtime.SetPlayerTargetActive(false);
    runtime.Tick(144);
    DispatchDeferred(events);
    WO_CHECK_EQ(CountPlayerDamageEvents(events), 3);
    WO_CHECK(runtime.Npcs().front().instance.state == NPCState::Patrol);
    return true;
}

bool AutonomousGuardReplansWhenPlayerMovesOutsideRange() {
    Grid grid(20, 6);
    for (int32_t row = 0; row < grid.Height(); ++row) {
        for (int32_t col = 0; col < grid.Width(); ++col) {
            grid.SetCell(col, row, GridCell{});
        }
    }
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    ActorRecord actor;
    actor.id = NpcId::New(406);
    actor.role = Role::Guard;
    actor.faction = Faction::Security;
    actor.cognition = CognitionTier::SemiHuman;
    WO_CHECK(systemic.AddActor(actor));
    EventBus events;
    DeterministicRNG rng(0x406);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance guard;
    guard.id = NpcId::New(406);
    guard.role = Role::Guard;
    guard.faction = Faction::Security;
    guard.position = Vec3{1.5f, 1.5f, 0.0f};
    guard.yaw = 0.0f;
    guard.sight_range = 64.0f;
    WO_CHECK(runtime.AddNpc(guard, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    // Start outside the guard's 12 m weapon envelope but inside its authored
    // sight range so the combat motor has a real approach to perform.
    runtime.SetPlayerPose(Vec3{15.5f, 1.5f, 0.0f}, kEyeStand);
    runtime.Tick(12);
    WO_CHECK(runtime.Npcs().front().instance.state == NPCState::Combat);
    const Vec3 first_goal_start = runtime.Npcs().front().instance.position;
    for (uint64_t frame = 13; frame <= 180; ++frame) runtime.Tick(frame);
    const Vec3 after_first_approach = runtime.Npcs().front().instance.position;
    WO_CHECK(after_first_approach.x > first_goal_start.x + 0.5f);

    // Move the same target materially.  The guard must continue approaching
    // the new location instead of completing the old route and idling.
    runtime.SetPlayerPose(Vec3{16.5f, 1.5f, 0.0f}, kEyeStand);
    for (uint64_t frame = 181; frame <= 720; ++frame) runtime.Tick(frame);
    const Vec3 final_position = runtime.Npcs().front().instance.position;
    WO_CHECK(final_position.x > after_first_approach.x + 0.5f);
    WO_CHECK(final_position.x < 16.5f);
    return true;
}

bool AutonomousVisibilityChangesCombatCounterfactual() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    ActorRecord actor;
    actor.id = NpcId::New(403);
    actor.role = Role::Guard;
    actor.faction = Faction::Security;
    actor.cognition = CognitionTier::SemiHuman;
    WO_CHECK(systemic.AddActor(actor));
    EventBus events;
    DeterministicRNG rng(0x403);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance guard;
    guard.id = NpcId::New(403);
    guard.role = Role::Guard;
    guard.faction = Faction::Security;
    guard.position = Vec3{1.5f, 1.5f, 0.0f};
    guard.yaw = 0.0f;
    WO_CHECK(runtime.AddNpc(guard, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{4.5f, 1.5f, 0.0f}, kEyeStand);

    runtime.SetPlayerVisibility(0.12f); // crouched/still/dark equivalent
    runtime.Tick(12);
    DispatchDeferred(events);
    WO_CHECK_EQ(CountPlayerDamageEvents(events), 0);
    WO_CHECK(runtime.Npcs().front().instance.state == NPCState::Patrol);

    runtime.SetPlayerVisibility(1.0f); // standing/running/bright equivalent
    runtime.Tick(24);
    DispatchDeferred(events);
    WO_CHECK_EQ(CountPlayerDamageEvents(events), 1);
    WO_CHECK(runtime.Npcs().front().instance.state == NPCState::Combat);
    return true;
}

bool AutonomousInvestigateRoutesNoiseWithoutTeleporting() {
    Grid grid(8, 7);
    for (int32_t row = 0; row < grid.Height(); ++row) {
        for (int32_t col = 0; col < grid.Width(); ++col) {
            grid.SetCell(col, row, GridCell{});
        }
    }
    for (int32_t row = 0; row < 6; ++row) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(4, row, wall);
    }
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    EventBus events;
    DeterministicRNG rng(0x404);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance npc;
    npc.id = NpcId::New(404);
    npc.position = Vec3{2.5f, 2.5f, 0.0f};
    npc.yaw = 0.0f;
    WO_CHECK(runtime.AddNpc(npc, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{2.5f, 5.5f, 0.0f}, kEyeStand);
    events.Post(EventWeaponFire{EntityId::New(1), WeaponSlot::Pistol,
                                Vec3{7.5f, 2.5f, kEyeStand}, 0.0f, 0.0f,
                                0.9f},
                EventKind::Notification, EntityId::New(1), EntityId::Invalid(),
                EventId::Invalid(), 12);
    events.Dispatch();
    events.Dispatch();

    const Vec3 start = runtime.Npcs().front().instance.position;
    runtime.Tick(12);
    WO_CHECK(runtime.Npcs().front().instance.state == NPCState::Investigate);
    const Vec3 first = runtime.Npcs().front().instance.position;
    // The decision creates the route; the motor follows it on the next
    // normal simulation tick. Investigation is not a due-frame teleport.
    WO_CHECK(first.x == start.x && first.y == start.y);
    runtime.Tick(24);
    const Vec3 second = runtime.Npcs().front().instance.position;
    WO_CHECK(second.x < 3.0f && second.x > start.x);
    for (uint64_t frame = 36; frame <= 1200; ++frame) runtime.Tick(frame);
    const Vec3 arrived = runtime.Npcs().front().instance.position;
    WO_CHECK(arrived.x > 4.5f || arrived.y > 5.0f);
    WO_CHECK(!query.AabbBlocked(AABB{{arrived.x - 0.42f, arrived.y - 0.42f, 0.0f},
                                     {arrived.x + 0.42f, arrived.y + 0.42f, 1.8f}}));

    // A fully sealed target has no valid route and therefore cannot cause a
    // remote investigate/discovery jump.
    Grid blocked(8, 7);
    for (int32_t row = 0; row < blocked.Height(); ++row) {
        for (int32_t col = 0; col < blocked.Width(); ++col) {
            blocked.SetCell(col, row, GridCell{});
        }
    }
    for (int32_t row = 0; row < blocked.Height(); ++row) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        blocked.SetCell(4, row, wall);
    }
    GridWorldQuery blocked_query(&blocked);
    EventBus blocked_events;
    AutonomousNpcSystem blocked_runtime;
    blocked_runtime.Attach(&systemic, &blocked_events, &rng);
    NPCInstance blocked_npc = npc;
    blocked_npc.id = NpcId::New(405);
    WO_CHECK(blocked_runtime.AddNpc(blocked_npc, RoomId::New(1)));
    blocked_runtime.SetWorldQuery(&blocked_query);
    blocked_runtime.SetActiveRoom(RoomId::New(1));
    blocked_runtime.SetPlayerPose(Vec3{2.5f, 5.5f, 0.0f}, kEyeStand);
    blocked_events.Post(EventWeaponFire{EntityId::New(1), WeaponSlot::Pistol,
                                        Vec3{7.5f, 2.5f, kEyeStand}, 0.0f, 0.0f,
                                        0.9f},
                        EventKind::Notification, EntityId::New(1),
                        EntityId::Invalid(), EventId::Invalid(), 12);
    blocked_events.Dispatch();
    blocked_events.Dispatch();
    for (uint64_t frame = 12; frame <= 600; frame += 12) {
        blocked_runtime.Tick(frame);
    }
    const Vec3 blocked_end = blocked_runtime.Npcs().front().instance.position;
    WO_CHECK(blocked_end.x < 3.0f);
    return true;
}

bool AutonomousFullNpcSpeaksOnSightTransitionOnly() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    ActorRecord actor;
    actor.id = NpcId::New(18);
    actor.data_key = ResourceId::New(7018);
    actor.faction = Faction::Security;
    actor.cognition = CognitionTier::Full;
    actor.role = Role::Guard;
    WO_CHECK(systemic.AddActor(actor));

    EventBus events;
    DeterministicRNG rng(0x18a5);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance npc;
    npc.id = NpcId::New(18);
    npc.cognition = CognitionTier::Full;
    npc.faction = Faction::Security;
    npc.role = Role::Guard;
    npc.position = Vec3{1.5f, 1.5f, 0.0f};
    npc.yaw = 0.0f;
    npc.health = 100;
    WO_CHECK(runtime.AddNpc(npc, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{4.5f, 1.5f, 0.0f}, kEyeStand);

    const auto dispatch_tick = [&](uint64_t frame) {
        runtime.Tick(frame);
        // Events posted by a tick are deferred by the EventBus contract.
        events.Dispatch();
        events.Dispatch();
    };

    dispatch_tick(12);
    WO_CHECK_EQ(CountNpcSpeakEvents(events, npc.id), 1);
    // Ten-Hz decision cadence over five seconds must not repeat the same
    // speech while the player remains continuously visible.
    for (uint64_t frame = 24; frame <= 600; frame += 12) {
        dispatch_tick(frame);
    }
    WO_CHECK_EQ(CountNpcSpeakEvents(events, npc.id), 1);

    // A real visibility gap arms a new sight-entry transition.
    runtime.SetPlayerPose(Vec3{10.5f, 4.5f, 0.0f}, kEyeStand);
    dispatch_tick(612);
    WO_CHECK_EQ(CountNpcSpeakEvents(events, npc.id), 1);
    runtime.SetPlayerPose(Vec3{4.5f, 1.5f, 0.0f}, kEyeStand);
    dispatch_tick(624);
    WO_CHECK_EQ(CountNpcSpeakEvents(events, npc.id), 2);
    return true;
}

bool AutonomousRuntimeRefreshesContinuousObservation() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    ActorRecord actor;
    actor.id = NpcId::New(8);
    actor.data_key = ResourceId::New(7008);
    actor.faction = Faction::Security;
    actor.cognition = CognitionTier::SemiHuman;
    actor.role = Role::Guard;
    WO_CHECK(systemic.AddActor(actor));

    EventBus events;
    DeterministicRNG rng(0x5678);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance npc;
    npc.id = NpcId::New(8);
    npc.cognition = CognitionTier::SemiHuman;
    npc.faction = Faction::Security;
    npc.role = Role::Guard;
    npc.position = Vec3{1.5f, 1.5f, 0.0f};
    npc.yaw = 0.0f;
    npc.health = 100;
    WO_CHECK(runtime.AddNpc(npc, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{4.5f, 1.5f, 0.0f}, kEyeStand);

    for (uint64_t frame = 12; frame <= 600; frame += 12) {
        runtime.Tick(frame);
    }
    WO_CHECK(systemic.MemoryCount() == 1);
    const MemoryRecord* refreshed = systemic.GetMemory(
        systemic.MemoriesOf(EntityId::New(8)).front().id);
    WO_CHECK(refreshed != nullptr && refreshed->frame == 600);

    // A genuinely separated stimulus after a silent gap may form a new memory
    // rather than being merged forever.  Continuous sight itself keeps
    // refreshing the same semantic record.
    runtime.SetPlayerPose(Vec3{10.5f, 4.5f, 0.0f}, kEyeStand);
    runtime.Tick(1212);
    runtime.SetPlayerPose(Vec3{4.5f, 1.5f, 0.0f}, kEyeStand);
    runtime.Tick(1224);
    WO_CHECK(systemic.MemoryCount() == 2);
    return true;
}

bool AutonomousCleanerMustArriveBeforeDiscovery() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    BodyRecord body;
    body.id = EntityId::New(20);
    body.npc = NpcId::New(7);
    body.status = BodyStatus::Unconscious;
    body.disposition = BodyDisposition::Exposed;
    body.position = Vec3{1.5f, 1.5f, 0.0f};
    body.room = RoomId::New(1);
    HideableContainer cart;
    cart.id = ContainerId::New(30);
    cart.position = Vec3{4.5f, 1.5f, 0.0f};
    cart.room = RoomId::New(1);
    cart.accessibility = 80;
    cart.capacity_volume = 1.0f;
    cart.routine_tags.push_back(RoutineTag::Cleaner);
    WO_CHECK(systemic.AddBody(body));
    WO_CHECK(systemic.AddContainer(cart));
    WO_CHECK(systemic.BeginDrag(EntityId::New(1), body.id, 1));
    WO_CHECK(systemic.EndDrag(body.id, 2));
    WO_CHECK(systemic.HideBody(body.id, cart.id, 3));

    EventBus events;
    DeterministicRNG rng(0x9876);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance cleaner;
    cleaner.id = NpcId::New(11);
    cleaner.role = Role::Cleaner;
    cleaner.cognition = CognitionTier::SemiHuman;
    cleaner.position = Vec3{1.5f, 1.5f, 0.0f};
    cleaner.yaw = 0.0f;
    WO_CHECK(runtime.AddNpc(cleaner, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{7.0f, 5.0f, 0.0f}, kEyeStand);
    WO_CHECK(runtime.ConfigureBodyDiscovery(cleaner.id, body.id, cart.id, 12));

    runtime.Tick(12);
    const BodyRecord* before_arrival = systemic.GetBody(body.id);
    WO_CHECK(before_arrival != nullptr &&
             before_arrival->disposition == BodyDisposition::HiddenInContainer);
    WO_CHECK(runtime.Npcs().front().instance.position.x > cleaner.position.x);

    for (uint64_t frame = 24; frame <= 480; frame += 12) {
        runtime.Tick(frame);
    }
    const BodyRecord* after_arrival = systemic.GetBody(body.id);
    WO_CHECK(after_arrival != nullptr &&
             after_arrival->disposition == BodyDisposition::Exposed);
    WO_CHECK(runtime.DiscoveryResponseCount() == 1);
    return true;
}

bool AutonomousCleanerBlockedCannotDiscoverOnDueFrame() {
    Grid grid = MakeViewGrid();
    for (int32_t row = 0; row < grid.Height(); ++row) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(2, row, wall);
    }
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    BodyRecord body;
    body.id = EntityId::New(60);
    body.npc = NpcId::New(16);
    body.status = BodyStatus::Unconscious;
    body.disposition = BodyDisposition::Exposed;
    body.position = Vec3{4.5f, 1.5f, 0.0f};
    body.room = RoomId::New(1);
    HideableContainer cart;
    cart.id = ContainerId::New(61);
    cart.position = Vec3{4.5f, 1.5f, 0.0f};
    cart.room = RoomId::New(1);
    cart.capacity_volume = 1.0f;
    cart.accessibility = 80;
    cart.routine_tags.push_back(RoutineTag::Cleaner);
    WO_CHECK(systemic.AddBody(body));
    WO_CHECK(systemic.AddContainer(cart));
    WO_CHECK(systemic.HideBody(body.id, cart.id, 1));

    EventBus events;
    DeterministicRNG rng(0x369c);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance cleaner;
    cleaner.id = NpcId::New(17);
    cleaner.role = Role::Cleaner;
    cleaner.cognition = CognitionTier::SemiHuman;
    cleaner.position = Vec3{1.5f, 1.5f, 0.0f};
    WO_CHECK(runtime.AddNpc(cleaner, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{7.0f, 5.0f, 0.0f}, kEyeStand);
    WO_CHECK(runtime.ConfigureBodyDiscovery(cleaner.id, body.id, cart.id, 12));

    for (uint64_t frame = 12; frame <= 480; frame += 12) {
        runtime.Tick(frame);
    }
    const BodyRecord* still_hidden = systemic.GetBody(body.id);
    WO_CHECK(still_hidden != nullptr &&
             still_hidden->disposition == BodyDisposition::HiddenInContainer);
    WO_CHECK(runtime.DiscoveryResponseCount() == 0);
    WO_CHECK(runtime.Npcs().front().instance.position.x < 2.0f);
    return true;
}

bool AutonomousCleanerDurableHistoryChangesDiscoveryResponse() {
    const auto run_case = [](bool trusted_history, bool round_trip) {
        Grid grid = MakeViewGrid();
        GridWorldQuery query(&grid);
        SystemicWorld systemic;
        BodyRecord body;
        body.id = EntityId::New(80);
        body.npc = NpcId::New(81);
        body.status = BodyStatus::Unconscious;
        body.disposition = BodyDisposition::Exposed;
        body.position = Vec3{1.5f, 1.5f, 0.0f};
        body.room = RoomId::New(1);
        HideableContainer cart;
        cart.id = ContainerId::New(82);
        cart.position = Vec3{4.5f, 1.5f, 0.0f};
        cart.room = RoomId::New(1);
        cart.accessibility = 80;
        cart.capacity_volume = 1.0f;
        cart.routine_tags.push_back(RoutineTag::Cleaner);
        if (!systemic.AddBody(body) || !systemic.AddContainer(cart) ||
            !systemic.HideBody(body.id, cart.id, 3)) {
            return false;
        }

        const EntityId cleaner_entity = EntityId::New(83);
        if (trusted_history) {
            RelationshipRecord relation;
            relation.a = cleaner_entity;
            relation.b = EntityId::New(1);
            relation.trust = 0.80f;
            relation.debt = 0.70f;
            if (!systemic.SetRelationship(relation)) return false;
        }
        if (round_trip) {
            const std::vector<uint8_t> bytes = systemic.Serialize();
            const Result<SystemicWorld> restored =
                SystemicWorld::Deserialize(bytes.data(), bytes.size());
            if (restored.IsError()) return false;
            systemic = restored.Value();
        }

        EventBus events;
        DeterministicRNG rng(0x83u + (trusted_history ? 1u : 0u));
        AutonomousNpcSystem runtime;
        runtime.Attach(&systemic, &events, &rng);
        NPCInstance cleaner;
        cleaner.id = NpcId::New(83);
        cleaner.role = Role::Cleaner;
        cleaner.cognition = CognitionTier::SemiHuman;
        cleaner.position = Vec3{1.5f, 1.5f, 0.0f};
        if (!runtime.AddNpc(cleaner, RoomId::New(1))) return false;
        runtime.SetWorldQuery(&query);
        runtime.SetActiveRoom(RoomId::New(1));
        runtime.SetPlayerPose(Vec3{7.0f, 5.0f, 0.0f}, kEyeStand);
        if (!runtime.ConfigureBodyDiscovery(cleaner.id, body.id, cart.id, 12)) {
            return false;
        }

        for (uint64_t frame = 12; frame <= 600; frame += 12) {
            runtime.Tick(frame);
        }
        if (runtime.DiscoveryResponseCount() != 1) return false;
        bool saw_medical = false;
        bool saw_cover_up = false;
        for (const auto& event : systemic.SystemEvents()) {
            saw_medical = saw_medical ||
                (event.type == SystemicEventType::MedicalCall &&
                 event.actor == cleaner_entity);
            saw_cover_up = saw_cover_up ||
                (event.type == SystemicEventType::HelpCoverUp &&
                 event.actor == cleaner_entity);
        }
        return trusted_history ? saw_cover_up && !saw_medical
                                : saw_medical && !saw_cover_up;
    };

    // Same present scene, different durable history: the cleaner's response
    // changes after an explicit relationship has survived serialization.
    WO_CHECK(run_case(false, false));
    WO_CHECK(run_case(true, true));
    return true;
}

bool AutonomousRepeatedGunshotsRefreshOneMemory() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    ActorRecord actor;
    actor.id = NpcId::New(13);
    actor.data_key = ResourceId::New(7013);
    actor.faction = Faction::Security;
    actor.cognition = CognitionTier::SemiHuman;
    actor.role = Role::Guard;
    WO_CHECK(systemic.AddActor(actor));

    EventBus events;
    DeterministicRNG rng(0x1357);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance npc;
    npc.id = NpcId::New(13);
    npc.cognition = CognitionTier::SemiHuman;
    npc.faction = Faction::Security;
    npc.role = Role::Guard;
    npc.position = Vec3{1.5f, 1.5f, 0.0f};
    npc.yaw = 0.0f;
    WO_CHECK(runtime.AddNpc(npc, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    // The player is outside the guard's sight cone. Only the posted gunshot
    // should produce the event-recall memory.
    runtime.SetPlayerPose(Vec3{1.5f, 7.0f, 0.0f}, kEyeStand);

    const auto post_gunshot = [&](uint64_t frame) {
        events.Post(EventWeaponFire{EntityId::New(1), WeaponSlot::Pistol,
                                    Vec3{1.5f, 1.5f, kEyeStand}, 0.0f, 0.0f,
                                    0.9f},
                    EventKind::Notification, EntityId::New(1),
                    EntityId::Invalid(), EventId::Invalid(), frame);
        // EventBus delivery is intentionally deferred by one dispatch turn.
        events.Dispatch();
        events.Dispatch();
        runtime.Tick(frame);
    };

    post_gunshot(12);
    post_gunshot(24);
    WO_CHECK(systemic.MemoryCount() == 1);
    const auto first = systemic.MemoriesOf(EntityId::New(13));
    WO_CHECK(first.size() == 1 && first.front().frame == 24);
    // A sound heard without line-of-sight is a real observation, but it does
    // not prove who fired.  Attribution must wait for a visual/provenance
    // event rather than being smuggled in as a player memory.
    WO_CHECK(first.front().source == KnowledgeSource::HeardSound &&
             !first.front().subject.IsValid() && !first.front().target.IsValid());

    // After the refresh window, the same sound semantic is a new event.
    post_gunshot(636);
    WO_CHECK(systemic.MemoryCount() == 2);
    return true;
}

bool AutonomousIncapacitatedCleanerCannotWitness() {
    Grid grid = MakeViewGrid();
    GridWorldQuery query(&grid);
    SystemicWorld systemic;
    BodyRecord body;
    body.id = EntityId::New(40);
    body.npc = NpcId::New(14);
    body.status = BodyStatus::Unconscious;
    body.disposition = BodyDisposition::Exposed;
    body.position = Vec3{1.5f, 1.5f, 0.0f};
    body.room = RoomId::New(1);
    HideableContainer cart;
    cart.id = ContainerId::New(41);
    cart.position = Vec3{2.5f, 1.5f, 0.0f};
    cart.room = RoomId::New(1);
    cart.capacity_volume = 1.0f;
    cart.accessibility = 80;
    cart.routine_tags.push_back(RoutineTag::Cleaner);
    WO_CHECK(systemic.AddBody(body));
    WO_CHECK(systemic.AddContainer(cart));
    WO_CHECK(systemic.HideBody(body.id, cart.id, 1));

    EventBus events;
    DeterministicRNG rng(0x2468);
    AutonomousNpcSystem runtime;
    runtime.Attach(&systemic, &events, &rng);
    NPCInstance cleaner;
    cleaner.id = NpcId::New(15);
    cleaner.role = Role::Cleaner;
    cleaner.cognition = CognitionTier::SemiHuman;
    cleaner.position = Vec3{1.5f, 1.5f, 0.0f};
    cleaner.state = NPCState::Stunned;
    WO_CHECK(runtime.AddNpc(cleaner, RoomId::New(1)));
    runtime.SetWorldQuery(&query);
    runtime.SetActiveRoom(RoomId::New(1));
    runtime.SetPlayerPose(Vec3{7.0f, 5.0f, 0.0f}, kEyeStand);
    WO_CHECK(runtime.ConfigureBodyDiscovery(cleaner.id, body.id, cart.id, 12));

    for (uint64_t frame = 12; frame <= 240; frame += 12) {
        runtime.Tick(frame);
    }
    const BodyRecord* still_hidden = systemic.GetBody(body.id);
    WO_CHECK(still_hidden != nullptr &&
             still_hidden->disposition == BodyDisposition::HiddenInContainer);
    WO_CHECK(systemic.MemoryCount() == 0);
    WO_CHECK(runtime.DiscoveryResponseCount() == 0);
    return true;
}

bool AutonomousLoadDoesNotPartiallyMutateOnUnknownNpc() {
    SystemicWorld systemic;
    EventBus events;
    DeterministicRNG rng(0x8642);
    AutonomousNpcSystem source;
    source.Attach(&systemic, &events, &rng);

    NPCInstance first;
    first.id = NpcId::New(71);
    first.position = Vec3{1.5f, 1.5f, 0.0f};
    NPCInstance second;
    second.id = NpcId::New(72);
    second.position = Vec3{3.5f, 1.5f, 0.0f};
    WO_CHECK(source.AddNpc(first, RoomId::New(1)));
    WO_CHECK(source.AddNpc(second, RoomId::New(1)));

    std::vector<uint8_t> bytes;
    Serializer serializer(bytes);
    source.Save(serializer);
    // v2 runtime records are 45 bytes each after the version/count header.
    // Replace only the second ID with a valid-but-unregistered identity.
    constexpr size_t kSecondNpcIdOffset = 8 + 45;
    WO_CHECK(bytes.size() >= kSecondNpcIdOffset + sizeof(uint64_t));
    const uint64_t unknown_id = 0x72000000000003E7ull;
    for (size_t i = 0; i < sizeof(unknown_id); ++i) {
        bytes[kSecondNpcIdOffset + i] =
            static_cast<uint8_t>((unknown_id >> (i * 8)) & 0xffu);
    }

    AutonomousNpcSystem target;
    target.Attach(&systemic, &events, &rng);
    first.position = Vec3{9.5f, 9.5f, 0.0f};
    WO_CHECK(target.AddNpc(first, RoomId::New(1)));
    WO_CHECK(target.AddNpc(second, RoomId::New(1)));
    const Vec3 before = target.Npcs().front().instance.position;
    Deserializer deserializer(bytes.data(), bytes.size());
    WO_CHECK(!target.Load(deserializer));
    WO_CHECK(deserializer.HasError());
    WO_CHECK(target.Npcs().front().instance.position.x == before.x &&
             target.Npcs().front().instance.position.y == before.y);
    WO_CHECK(target.Npcs()[1].instance.position.x == second.position.x &&
             target.Npcs()[1].instance.position.y == second.position.y);
    return true;
}

} // namespace

void RegisterAiTests(TestHarness& test) {
    test.Add("ai.perception_sees_open", &PerceptionSeesInOpen);
    test.Add("ai.perception_blocked_wall", &PerceptionBlockedByWall);
    test.Add("ai.perception_hears_noise", &PerceptionHearsNoise);
    test.Add("ai.perception_wall_muffles_noise", &PerceptionWallMufflesNoise);
    test.Add("ai.perception_future_noise_timestamp_rejected",
             &PerceptionIgnoresFutureNoiseTimestamp);
    test.Add("ai.goap_plans_chain", &GoapPlansSimpleChain);
    test.Add("ai.goap_no_plan_impossible", &GoapNoPlanWhenImpossible);
    test.Add("ai.memory_recall_order", &MemoryRecallOrdered);
    test.Add("ai.autonomous_runtime_five_phase_loop",
             &AutonomousRuntimeRunsFivePhaseLoop);
    test.Add("ai.full_npc_speaks_on_sight_transition_only",
             &AutonomousFullNpcSpeaksOnSightTransitionOnly);
    test.Add("ai.autonomous_runtime_refreshes_continuous_observation",
             &AutonomousRuntimeRefreshesContinuousObservation);
    test.Add("ai.cleaner_must_arrive_before_discovery",
             &AutonomousCleanerMustArriveBeforeDiscovery);
    test.Add("ai.cleaner_blocked_cannot_discover_on_due_frame",
             &AutonomousCleanerBlockedCannotDiscoverOnDueFrame);
    test.Add("ai.cleaner_history_changes_discovery_response",
             &AutonomousCleanerDurableHistoryChangesDiscoveryResponse);
    test.Add("ai.repeated_gunshots_refresh_one_memory",
             &AutonomousRepeatedGunshotsRefreshOneMemory);
    test.Add("ai.incapacitated_cleaner_cannot_witness",
             &AutonomousIncapacitatedCleanerCannotWitness);
    test.Add("ai.patrol_grid_route_motor", &AutonomousPatrolUsesGridRouteAndMotor);
    test.Add("ai.motor_keeps_player_personal_space",
             &AutonomousMotorKeepsPersonalSpaceFromPlayer);
    test.Add("ai.guard_combat_los_and_cadence",
             &AutonomousGuardCombatRepeatsAndHonoursLineOfSight);
    test.Add("ai.guard_replans_moving_target",
             &AutonomousGuardReplansWhenPlayerMovesOutsideRange);
    test.Add("ai.visibility_counterfactual", &AutonomousVisibilityChangesCombatCounterfactual);
    test.Add("ai.investigate_routes_noise_without_teleporting",
             &AutonomousInvestigateRoutesNoiseWithoutTeleporting);
    test.Add("ai.load_unknown_npc_is_atomic",
             &AutonomousLoadDoesNotPartiallyMutateOnUnknownNpc);
}

} // namespace writeover
