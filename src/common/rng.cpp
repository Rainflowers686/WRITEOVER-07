#include "writeover/common/rng.h"

#include "writeover/common/serialize.h"

namespace writeover {

void DeterministicRNG::Seed(uint64_t seed) {
    // xorshift128+ requires a non-zero state pair; 0x7f4a7c15 expands it.
    state0_ = seed ? seed : 0x7F4A7C15u;
    state1_ = 0xEDB885A3u ^ (seed >> 1);
    if (state0_ == 0 && state1_ == 0) {
        state0_ = 0x9E3779B97F4A7C15ull;
    }
    (void)Next();  // discard one value per splitmix-ish initialization
}

uint64_t DeterministicRNG::Next() {
    uint64_t x = state0_;
    const uint64_t y = state1_;
    state0_ = y;
    x ^= x << 23;
    state1_ = x ^ y ^ (x >> 17) ^ (y >> 26);
    return state1_ + y;
}

float DeterministicRNG::NextFloat() {
    // 53-bit mantissa -> [0,1)
    return static_cast<float>((Next() >> 11) * 0x1.0p-53);
}

int32_t DeterministicRNG::NextInt(int32_t min, int32_t max_inclusive) {
    if (max_inclusive <= min) {
        return min;
    }
    const uint64_t range = static_cast<uint64_t>(max_inclusive - min) + 1;
    return min + static_cast<int32_t>(Next() % range);
}

void DeterministicRNG::Save(Serializer& s) const {
    s.WriteU64(state0_);
    s.WriteU64(state1_);
}

void DeterministicRNG::Load(Deserializer& d) {
    state0_ = d.ReadU64();
    state1_ = d.ReadU64();
}

} // namespace writeover