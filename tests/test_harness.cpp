#include "tests/test_harness.h"

#include <cstdio>
#include <cstdint>
#include <cmath>

namespace writeover {

void TestHarness::Add(const char* name, bool (*fn)()) {
    entries_.push_back({name, fn});
}

int TestHarness::RunAll() {
    int failed = 0;
    for (const auto& entry : entries_) {
        const bool ok = entry.fn();
        std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", entry.name);
        if (!ok) {
            ++failed;
        }
    }
    std::printf("%d tests, %d failed\n", static_cast<int>(entries_.size()), failed);
    return failed;
}

bool Check(bool ok, const char* file, int line, const char* expr) {
    if (!ok) {
        std::printf("  ASSERT FAILED %s:%d: %s\n", file, line, expr);
    }
    return ok;
}

bool CheckEqI(int64_t a, int64_t b, const char* file, int line) {
    if (a != b) {
        std::printf("  ASSERT FAILED %s:%d: %lld != %lld\n", file, line,
                    static_cast<long long>(a), static_cast<long long>(b));
        return false;
    }
    return true;
}

bool CheckNear(float a, float b, float eps, const char* file, int line) {
    if (!(std::fabs(a - b) <= eps)) {
        std::printf("  ASSERT FAILED %s:%d: %f != %f (eps %f)\n", file, line,
                    static_cast<double>(a), static_cast<double>(b),
                    static_cast<double>(eps));
        return false;
    }
    return true;
}

} // namespace writeover