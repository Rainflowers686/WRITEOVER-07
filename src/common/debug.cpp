#include "writeover/common/debug.h"

namespace writeover {

void DebugMetrics::Set(const char* name, uint64_t value) {
    for (auto& s : samples_) {
        if (s.name == name) {
            s.value = value;
            return;
        }
    }
    samples_.push_back(MetricSample{name, value, MetricKind::Gauge});
}

void DebugMetrics::Inc(const char* name, uint64_t delta) {
    for (auto& s : samples_) {
        if (s.name == name) {
            s.value += delta;
            return;
        }
    }
    samples_.push_back(MetricSample{name, delta, MetricKind::Counter});
}

void DebugMetrics::AddHistogram(const char* name, uint64_t value) {
    samples_.push_back(MetricSample{name, value, MetricKind::Histogram});
}

} // namespace writeover