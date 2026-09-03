#include "tests/test_harness.h"

namespace writeover {

void RegisterCommonTests(TestHarness&);
void RegisterCoreTests(TestHarness&);
void RegisterRenderTests(TestHarness&);
void RegisterPlayerTests(TestHarness&);
void RegisterWorldTests(TestHarness&);
void RegisterAiTests(TestHarness&);
void RegisterNarrativeTests(TestHarness&);
void RegisterSaveReplayTests(TestHarness&);

} // namespace writeover

int RunAllTestsAndExit() {
    writeover::TestHarness harness;

    using RegisterFn = void (*)(writeover::TestHarness&);
    const RegisterFn registers[] = {
        &writeover::RegisterCommonTests,
        &writeover::RegisterCoreTests,
        &writeover::RegisterRenderTests,
        &writeover::RegisterPlayerTests,
        &writeover::RegisterWorldTests,
        &writeover::RegisterAiTests,
        &writeover::RegisterNarrativeTests,
        &writeover::RegisterSaveReplayTests,
    };
    for (const auto reg : registers) {
        reg(harness);
    }
    return harness.RunAll();
}

int main() {
    const int failed = RunAllTestsAndExit();
    return failed == 0 ? 0 : 1;
}