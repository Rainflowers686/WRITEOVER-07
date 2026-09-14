#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace writeover::detail {

// Wall time schedules presentation only. Simulation still owns its 120 Hz
// clock. Deadlines retain their phase instead of charging render time twice.
class PresentationCadence {
public:
    bool Due(double elapsed_ms, uint8_t requested_hz) {
        const int hz = std::min<int>(requested_hz, 120);
        if (hz == 0) { hz_ = 0; return true; }
        const double interval = 1000.0 / hz;
        if (hz != hz_) {
            hz_ = hz;
            next_ms_ = elapsed_ms;
        }
        constexpr double kWakeToleranceMs = 0.1;
        if (elapsed_ms + kWakeToleranceMs < next_ms_) return false;
        // Skip expired deadlines after a stall; never burst-render old frames.
        const double overdue = std::max(0.0, elapsed_ms - next_ms_);
        next_ms_ += (std::floor(overdue / interval) + 1.0) * interval;
        return true;
    }
private:
    int hz_ = 0;
    double next_ms_ = 0.0;
};

} // namespace writeover::detail
