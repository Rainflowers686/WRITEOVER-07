#include "writeover/render/benchmark.h"

#include <algorithm>
#include <cstdio>
#include <numeric>

namespace writeover {

void FrameTimeSampler::AddSample(double ms) { samples_.push_back(ms); }

FrameStats FrameTimeSampler::Compute() const {
    FrameStats stats;
    stats.count = samples_.size();
    if (samples_.empty()) {
        return stats;
    }
    std::vector<double> sorted = samples_;
    std::sort(sorted.begin(), sorted.end());
    stats.min_ms = sorted.front();
    stats.max_ms = sorted.back();
    const double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
    stats.avg_ms = sum / static_cast<double>(sorted.size());
    // 1% low frame time == average of the slowest 1% of frames.
    const size_t worst_count =
        std::max<size_t>(1, sorted.size() / 100);
    double worst_sum = 0.0;
    for (size_t i = 0; i < worst_count; ++i) {
        worst_sum += sorted[sorted.size() - 1 - i];
    }
    stats.worst_1pct_avg_ms = worst_sum / static_cast<double>(worst_count);
    stats.one_pct_low_fps = stats.worst_1pct_avg_ms > 0.0
                                ? 1000.0 / stats.worst_1pct_avg_ms
                                : 0.0;
    return stats;
}

void PrintCsv(const char* scenario, const FrameStats& stats) {
    std::printf("scenario,count,avg_ms,worst_1pct_avg_ms,min_ms,max_ms\n");
    std::printf("%s,%zu,%.3f,%.3f,%.3f,%.3f\n", scenario, stats.count,
                stats.avg_ms, stats.worst_1pct_avg_ms, stats.min_ms, stats.max_ms);
}

} // namespace writeover