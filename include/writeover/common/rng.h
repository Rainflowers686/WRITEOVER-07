#pragma once
// Deterministic RNG (XorShift128+). Save/load full 128-bit state, never just
// the seed. Sim RNG drives all gameplay randomness; a separate visual RNG is
// for cosmetic effects only and is never saved.

#include <cstdint>

namespace writeover {

class Serializer;
class Deserializer;

class DeterministicRNG {
public:
    explicit DeterministicRNG(uint64_t seed = 0x9E3779B97F4A7C15ull) { Seed(seed); }

    uint64_t Next();
    uint32_t NextU32() { return static_cast<uint32_t>(Next()); }

    // [0, 1)
    float NextFloat();

    // inclusive [min, max]; degenerate range returns min
    int32_t NextInt(int32_t min, int32_t max_inclusive);

    void Seed(uint64_t seed);

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

    uint64_t GetState0() const { return state0_; }
    uint64_t GetState1() const { return state1_; }

private:
    uint64_t state0_ = 0;
    uint64_t state1_ = 0;
};

} // namespace writeover