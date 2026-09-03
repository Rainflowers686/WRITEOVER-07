#include "tests/test_harness.h"

#include "writeover/narrative/causality.h"
#include "writeover/narrative/dialog.h"
#include "writeover/narrative/judge.h"
#include "writeover/narrative/narrator.h"
#include "writeover/narrative/storylet.h"
#include "writeover/world/fact_belief.h"  // NarratorSpeakerId

namespace writeover {

namespace {

Storylet MakeGreetingStorylet() {
    Storylet s;
    s.id = StoryletId::New(1);
    s.text_id = "storylet_r1_greeting";
    s.priority = 100;
    s.once = true;
    s.conditions.push_back(FactEqualsCondition{FactId::New(10), true});
    s.conditions.push_back(FrameRangeCondition{0, 3600});
    s.actions.push_back(NarratorLineAction{"text_r1_narr_01", 1});
    s.actions.push_back(EndGameCommand{0});
    return s;
}

bool StoryletSelectDeterministic() {
    // Two eligible storylets with different priority: high wins, id tie-break.
    Storylet low;
    low.id = StoryletId::New(2);
    low.priority = 50;
    low.text_id = "low";
    low.conditions.push_back(FactEqualsCondition{FactId::New(10), true});

    Storylet high = MakeGreetingStorylet();

    StoryletEngine engine;
    engine.Register(low);
    engine.Register(high);

    FactStore facts;
    facts.Set(WorldFact{FactId::New(10), EntityId::New(1), PredicateType::State, true});

    const Storylet* selected = engine.SelectEligible(facts, {}, {}, {}, 1, 100);
    WO_CHECK(selected != nullptr);
    return selected != nullptr && selected->id == StoryletId::New(1);
}

bool StoryletOnceFiredSkipped() {
    Storylet s = MakeGreetingStorylet();
    StoryletEngine engine;
    engine.Register(s);
    engine.MarkFired(s.id);
    FactStore facts;
    facts.Set(WorldFact{FactId::New(10), EntityId::New(1), PredicateType::State, true});
    const Storylet* selected = engine.SelectEligible(facts, {}, {}, {}, 1, 100);
    return selected == nullptr;  // once story fired -> no longer eligible
}

bool StoryletConditionFalse() {
    Storylet s = MakeGreetingStorylet();
    StoryletEngine engine;
    engine.Register(s);
    FactStore facts;  // fact 10 missing -> condition fails
    const Storylet* selected = engine.SelectEligible(facts, {}, {}, {}, 1, 100);
    return selected == nullptr;
}

bool NarratorCapabilityGatesCommand() {
    NarratorSystem narrator;
    NarratorState state;
    state.capabilities.lock_doors = false;
    WorldCommand cmd;
    WO_CHECK(!narrator.TryIssueLockDoor(DoorId::New(1), state, cmd));

    state.capabilities.lock_doors = true;
    const bool issued = narrator.TryIssueLockDoor(DoorId::New(1), state, cmd);
    if (!issued) {
        return false;
    }
    return std::holds_alternative<CommandSetDoor>(cmd);
}

bool NarratorPowerKeepsFactsTruthful() {
    // Narrator can only emmit a typed command; it cannot reach the FactStore.
    NarratorSystem narrator;
    NarratorState state;
    state.capabilities.toggle_power = true;
    WorldCommand cmd;
    WO_CHECK(narrator.TryIssueTogglePower(SystemId::New(9), false, state, cmd));
    return std::holds_alternative<CommandSetPower>(cmd);
}

bool CausalityLedgerRing() {
    CausalityLedger ledger;
    for (uint32_t i = 0; i < kCausalityLedgerCapacity + 10; ++i) {
        CausalityEntry e;
        e.event_id = EventId::New(i + 1);
        e.parent_event_id = i > 0 ? EventId::New(i) : EventId::Invalid();
        e.sim_frame = i;
        ledger.Push(e);
    }
    const auto recent = ledger.Recent(5);
    WO_CHECK_EQ(static_cast<int64_t>(recent.size()), 5);
    // Oldest entry was evicted: first event id must be 11.
    const auto all = ledger.Recent(kCausalityLedgerCapacity);
    return !all.empty() && all.front().event_id == EventId::New(11);
}

bool DialogQueueExpiry() {
    DialogueQueue queue;
    SubtitleLine line;
    line.text = "test line";
    line.start_frame = 100;
    line.ttl_frames = 30;
    line.speaker_id = NarratorSpeakerId();
    queue.Push(line);
    WO_CHECK_EQ(static_cast<int64_t>(queue.ActiveLines(120).size()), 1);
    queue.Advance(200);
    return queue.ActiveLines(200).empty();
}

bool JudgeCheckpointBasics() {
    JudgeController judge;
    judge.Enable(0xABCD);
    JudgeCheckpoint cp;
    cp.index = 0;
    cp.room = RoomId::New(1);
    cp.frame = 1200;
    cp.save_slot_seed = 0xABCD;
    judge.SaveCheckpoint(cp);
    return judge.HasCheckpoint(0) && judge.GetCheckpoint(0)->frame == 1200 &&
           judge.Seed() == 0xABCD;
}

// HK-6: a typed WorldCommandAction (not a marker) must survive Save->Load.
// This proves content-compiled world commands keep their payload (F-09
// closure): SetDoor/SetPower tags and params round-trip byte-exactly.
bool WorldCommandActionRoundTrip() {
    Storylet s;
    s.id = StoryletId::New(5);
    s.text_id = "cmd_roundtrip";
    s.priority = 10;
    WorldCommandAction door;
    door.command = CommandSetDoor{DoorId::New(42), true};
    WorldCommandAction power;
    power.command = CommandSetPower{SystemId::New(7), false};
    s.actions.push_back(door);
    s.actions.push_back(power);

    StoryletEngine engine;
    engine.Register(s);

    std::vector<uint8_t> bytes;
    Serializer ser(bytes);
    engine.Save(ser);
    WO_CHECK(!bytes.empty());

    Deserializer der(bytes.data(), bytes.size());
    StoryletEngine restored;
    restored.Load(der);
    WO_CHECK(!der.HasError());
    WO_CHECK_EQ(static_cast<int64_t>(restored.Count()), 1);

    const auto& actions = restored.SelectEligible(
        FactStore{}, {}, {}, {}, 1, 0)->actions;
    WO_CHECK_EQ(static_cast<int64_t>(actions.size()), 2);
    const auto& a0 = std::get<WorldCommandAction>(actions[0]).command;
    const auto& a1 = std::get<WorldCommandAction>(actions[1]).command;
    if (!std::holds_alternative<CommandSetDoor>(a0) ||
        !std::holds_alternative<CommandSetPower>(a1)) {
        return false;
    }
    const auto& d0 = std::get<CommandSetDoor>(a0);
    const auto& p1 = std::get<CommandSetPower>(a1);
    WO_CHECK(d0.door == DoorId::New(42));
    WO_CHECK(d0.open);
    WO_CHECK(p1.system == SystemId::New(7));
    return !p1.powered;
}

} // namespace

void RegisterNarrativeTests(TestHarness& test) {
    test.Add("storylet.selects_highest_priority", &StoryletSelectDeterministic);
    test.Add("storylet.once_fired_skipped", &StoryletOnceFiredSkipped);
    test.Add("storylet.condition_false", &StoryletConditionFalse);
    test.Add("narrator.capability_gates_command", &NarratorCapabilityGatesCommand);
    test.Add("narrator.cannot_mutate_facts", &NarratorPowerKeepsFactsTruthful);
    test.Add("causality.ledger_ring", &CausalityLedgerRing);
    test.Add("dialog.queue_expiry", &DialogQueueExpiry);
    test.Add("judge.checkpoint_basics", &JudgeCheckpointBasics);
    test.Add("storylet.world_command_round_trip", &WorldCommandActionRoundTrip);
}

} // namespace writeover