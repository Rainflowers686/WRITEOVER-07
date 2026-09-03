#pragma once
// Deterministic simulation clock. Fixed 120Hz, one tick per frame; wall-clock
// never drives gameplay (M-006 / round-3 corrections).

#include <cstdint>

namespace writeover {

class Serializer;
class Deserializer;

class SimClock {
public:
    static constexpr uint64_t kSimHz = 120;
    static constexpr float kFixedDeltaTime = 1.0f / static_cast<float>(kSimHz);

    uint64_t FrameCount() const { return frame_count_; }
    float CurrentTime() const { return static_cast<float>(frame_count_) * kFixedDeltaTime; }
    float DeltaTime() const { return kFixedDeltaTime; }

    void Tick() { ++frame_count_; }

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    uint64_t frame_count_ = 0;
};

} // namespace writeover