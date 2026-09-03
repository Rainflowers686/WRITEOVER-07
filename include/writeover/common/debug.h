#pragma once
// Unified metrics API. Every module publishes diagnostics through
// DebugMetrics (EngineContext.metrics); no per-module printf.

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

enum class MetricKind : uint8_t {
    Gauge = 0,
    Counter = 1,
    Histogram = 2,
};

struct MetricSample {
    const char* name;
    uint64_t value;
    MetricKind kind;
};

class DebugMetrics {
public:
    DebugMetrics() = default;

    void Set(const char* name, uint64_t value);
    void Inc(const char* name, uint64_t delta = 1);
    void AddHistogram(const char* name, uint64_t value);

    const std::vector<MetricSample>& Snapshot() const { return samples_; }
    void Reset() { samples_.clear(); }

private:
    std::vector<MetricSample> samples_;
};

} // namespace writeover