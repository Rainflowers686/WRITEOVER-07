#include "tests/test_harness.h"

#include <cstdio>
#include <string_view>

namespace writeover {

void RegisterCommonTests(TestHarness&);
void RegisterCoreTests(TestHarness&);
void RegisterRenderTests(TestHarness&);
void RegisterPlayerTests(TestHarness&);
void RegisterInputTests(TestHarness&);
void RegisterWorldTests(TestHarness&);
void RegisterAiTests(TestHarness&);
void RegisterNarrativeTests(TestHarness&);
void RegisterSaveReplayTests(TestHarness&);
void RegisterSystemicTests(TestHarness&);
void RegisterCampaignTests(TestHarness&);
void RegisterProductTests(TestHarness&);

} // namespace writeover

int RunAllTestsAndExit(std::string_view group) {
    writeover::TestHarness harness;

    using RegisterFn = void (*)(writeover::TestHarness&);
    struct Group { std::string_view name; RegisterFn add; };
    const Group registers[] = {
        {"common", &writeover::RegisterCommonTests},
        {"core", &writeover::RegisterCoreTests},
        {"render", &writeover::RegisterRenderTests},
        {"player", &writeover::RegisterPlayerTests},
        {"input", &writeover::RegisterInputTests},
        {"world", &writeover::RegisterWorldTests},
        {"ai", &writeover::RegisterAiTests},
        {"narrative", &writeover::RegisterNarrativeTests},
        {"save", &writeover::RegisterSaveReplayTests},
        {"systemic", &writeover::RegisterSystemicTests},
        {"campaign", &writeover::RegisterCampaignTests},
        {"product", &writeover::RegisterProductTests},
    };
    bool found = false;
    for (const auto& reg : registers) {
        if (group.empty() || group == reg.name) {
            reg.add(harness);
            found = true;
        }
    }
    if (!found) { std::fprintf(stderr, "Unknown test group\n"); return 1; }
    return harness.RunAll();
}

int main(int argc, char** argv) {
    if (argc != 1 && (argc != 3 || std::string_view(argv[1]) != "--group")) {
        std::fprintf(stderr, "Usage: writeover_tests [--group NAME]\n");
        return 2;
    }
    const int failed = RunAllTestsAndExit(argc == 3 ? std::string_view(argv[2]) : std::string_view{});
    return failed == 0 ? 0 : 1;
}
