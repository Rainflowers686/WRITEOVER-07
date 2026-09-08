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
    WO_CHECK(runtime.Npcs().front().instance.state == NPCState::Alert);

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
    test.Add("ai.repeated_gunshots_refresh_one_memory",
             &AutonomousRepeatedGunshotsRefreshOneMemory);
    test.Add("ai.incapacitated_cleaner_cannot_witness",
             &AutonomousIncapacitatedCleanerCannotWitness);
    test.Add("ai.load_unknown_npc_is_atomic",
             &AutonomousLoadDoesNotPartiallyMutateOnUnknownNpc);
}

} // namespace writeover
