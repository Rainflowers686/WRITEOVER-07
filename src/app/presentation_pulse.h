#pragma once

#include <algorithm>
#include <cstdint>

namespace writeover {

// Cosmetic game-clock interval. Rendering at 30/60/120 Hz must not stretch
// flashes or narrator intrusions; the paused game clock naturally freezes it.
class PresentationPulse {
public:
    void Trigger(uint64_t frame, uint64_t duration) { start_ = frame; until_ = frame + duration; }
    void Extend(uint64_t frame, uint64_t duration) {
        if (!Active(frame)) start_ = frame;
        until_ = std::max(until_, frame + duration);
    }
    bool Active(uint64_t frame) const { return frame >= start_ && frame < until_; }
    uint64_t Elapsed(uint64_t frame) const { return frame >= start_ ? frame - start_ : 0; }
    uint64_t Duration() const { return until_ - start_; }
    void Reset() { start_ = 0; until_ = 0; }
private:
    uint64_t start_ = 0;
    uint64_t until_ = 0;
};

} // namespace writeover
