#pragma once
#include <cstdint>

namespace writeover {

// Scheduler cadence remains fixed; presentation pause reasons do not become
// durable gameplay state and cannot clear one another during resize recovery.
class RuntimeTimeGate {
public:
    void ObserveSchedulerFrame(uint64_t scheduler_frame) {
        if (!observed_) {
            observed_ = true;
            last_scheduler_frame_ = scheduler_frame;
            return;
        }
        if (scheduler_frame <= last_scheduler_frame_) return;
        if (!Paused()) game_frame_ += scheduler_frame - last_scheduler_frame_;
        last_scheduler_frame_ = scheduler_frame;
    }
    void SetPaused(bool paused) { paused_ = paused; }
    void SetSurfacePaused(bool paused) { surface_paused_ = paused; }
    bool Paused() const { return paused_ || surface_paused_; }
    uint64_t GameFrame() const { return game_frame_; }
private:
    uint64_t last_scheduler_frame_ = 0;
    uint64_t game_frame_ = 0;
    bool observed_ = false;
    bool paused_ = false;
    bool surface_paused_ = false;
};

} // namespace writeover
