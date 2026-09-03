#pragma once
// Benchmark helpers. Frame-time statistics use the CORRECT 1%-low definition:
// the 99th percentile frame time (the slowest 1% of frames), not the 1st
// percentile (M 17.6 closure).

#include <cstddef>
#include <cstdint>
#include <vector>

namespace writeover {

struct FrameStats {
    size_t count = 0;
    double avg_ms = 0.0;
    double worst_1pct_avg_ms = 0.0;  // average frame time of the slowest 1%
    double one_pct_low_fps = 0.0;     // 1000 / worst_1pct_avg_ms
    double min_ms = 0.0;
    double max_ms = 0.0;
};

class FrameTimeSampler {
public:
    void AddSample(double ms);
    size_t Count() const { return samples_.size(); }
    FrameStats Compute() const;

private:
    std::vector<double> samples_;
};

// Prints one CSV line: scenario,count,avg_ms,worst_1pct_avg_ms,min_ms,max_ms
void PrintCsv(const char* scenario, const FrameStats& stats);

} // namespace writeover