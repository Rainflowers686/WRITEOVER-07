#pragma once
// Minimal concrete test harness (no external framework; students can read it).
// Assertions return a bool; the runner counts failures. No fake PASS:
// every registered test actually executes.

#include <cstddef>
#include <string>
#include <vector>

namespace writeover {

class TestHarness {
public:
    struct Entry {
        const char* name;
        bool (*fn)();
    };

    void Add(const char* name, bool (*fn)());
    int RunAll();  // returns failure count; 0 == success

private:
    std::vector<Entry> entries_;
};

// Assertion helpers (returns false and prints context on failure).
bool Check(bool ok, const char* file, int line, const char* expr);
bool CheckEqI(int64_t a, int64_t b, const char* file, int line);
bool CheckNear(float a, float b, float eps, const char* file, int line);

} // namespace writeover

// Fail-fast macros: assertion failure causes immediate test function return.
// This ensures the test process exit != 0 when assertions fail.
#define WO_CHECK(expr) \
    do { \
        if (!::writeover::Check((expr), __FILE__, __LINE__, #expr)) { \
            return false; \
        } \
    } while (0)
#define WO_CHECK_EQ(a, b) \
    do { \
        if (!::writeover::CheckEqI((a), (b), __FILE__, __LINE__)) { \
            return false; \
        } \
    } while (0)
#define WO_CHECK_NEAR(a, b, eps) \
    do { \
        if (!::writeover::CheckNear((a), (b), (eps), __FILE__, __LINE__)) { \
            return false; \
        } \
    } while (0)
