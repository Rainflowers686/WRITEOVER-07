#include "tests/test_harness.h"

#include "writeover/common/rng.h"
#include "writeover/common/serialize.h"
#include "writeover/common/world_event.h"

#include <cmath>

namespace writeover {

namespace {

bool RngRepeatableDeterministic() {
    DeterministicRNG a(123), b(123);
    for (int i = 0; i < 20; ++i) {
        if (a.Next() != b.Next()) {
            return false;
        }
    }
    return true;
}

bool RngStateRoundTrip() {
    DeterministicRNG a(42);
    (void)a.Next();
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    a.Save(s);
    DeterministicRNG b;
    Deserializer d(bytes.data(), bytes.size());
    b.Load(d);
    return a.Next() == b.Next();  // full state, not just seed
}

bool SerializeRoundTrip() {
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    s.WriteU32(0xDEADBEEF);
    s.WriteF32(3.25f);
    s.WriteString("hello");
    Deserializer d(bytes.data(), bytes.size());
    const bool ok = d.ReadU32() == 0xDEADBEEF &&
                    std::fabs(d.ReadF32() - 3.25f) < 1e-6f &&
                    d.ReadString() == "hello" && !d.HasError();
    return ok;
}

bool WorldEventPayloadRoundTrip() {
    WorldEvent e;
    e.id = EventId::New(7);
    e.sim_frame = 99;
    e.kind = EventKind::Mutation;
    e.payload = EventWeaponFire{EntityId::New(1), WeaponSlot::Pistol,
                                Vec3{1, 2, 3}, 0.5f, -0.2f, 0.6f};
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    SerializeWorldEvent(s, e);
    Deserializer d(bytes.data(), bytes.size());
    const WorldEvent e2 = DeserializeWorldEvent(d);
    WO_CHECK(!d.HasError());
    WO_CHECK_EQ(static_cast<int64_t>(e2.id.GetValue()), 7);
    WO_CHECK_EQ(static_cast<int64_t>(e2.sim_frame), 99);
    if (!std::holds_alternative<EventWeaponFire>(e2.payload)) {
        return false;
    }
    const auto& fire = std::get<EventWeaponFire>(e2.payload);
    return fire.shooter == EntityId::New(1) && fire.slot == WeaponSlot::Pistol &&
           std::fabs(fire.origin.x - 1.0f) < 1e-4f;
}

} // namespace

namespace {

// Verifies Check() returns false for a failing assertion (proves detection).
bool CheckDetectsFailure() {
    return !::writeover::Check(false, __FILE__, __LINE__, "intentional");
}

// Verifies the fail-fast macro semantics: a function whose WO_CHECK fails
// returns false (proving the runner will report FAIL).
bool FailFastMacroProvesFailure() {
    const auto fails = []() -> bool {
        WO_CHECK(false);
        return true; // unreachable when macro is correct
    };
    return !fails();
}

// Verifies a passing assertion keeps the test alive.
bool CheckPassKeepsGoing() {
    WO_CHECK(true);
    return true;
}

} // namespace

void RegisterCommonTests(TestHarness& test) {
    test.Add("rng.repeatable", &RngRepeatableDeterministic);
    test.Add("rng.state_round_trip", &RngStateRoundTrip);
    test.Add("serialize.round_trip", &SerializeRoundTrip);
    test.Add("event.payload_round_trip", &WorldEventPayloadRoundTrip);
    // Meta-tests: prove the test oracle is trustworthy (G0 gate).
    test.Add("test_harness.check_detects_failure", &CheckDetectsFailure);
    test.Add("test_harness.failfast_macro_proves_failure", &FailFastMacroProvesFailure);
    test.Add("test_harness.check_pass_keeps_going", &CheckPassKeepsGoing);
}

} // namespace writeover
