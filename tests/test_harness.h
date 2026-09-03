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

#define WO_CHECK(expr)                    ::writeover::Check((expr), __FILE__, __LINE__, #expr)
#define WO_CHECK_EQ(a, b)                 ::writeover::CheckEqI((a), (b), __FILE__, __LINE__)
#define WO_CHECK_NEAR(a, b, eps)          ::writeover::CheckNear((a), (b), (eps), __FILE__, __LINE__)