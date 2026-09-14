#pragma once
#include "writeover/core/settings.h"
#include <algorithm>
#include <cstdint>

namespace writeover {

// Normal preserves the established combat baseline. The preference changes
// only damage received by the player, not AI senses, routes or weapon identity.
inline uint16_t IncomingDamageForDifficulty(uint16_t damage, uint8_t difficulty) {
    if (damage == 0) return 0;
    const uint32_t quarters = difficulty == 0 ? 3u : difficulty == 2 ? 5u : 4u;
    return static_cast<uint16_t>(std::clamp<uint32_t>(static_cast<uint32_t>(damage) * quarters / 4, 1, 65535));
}

inline Vec2 PreferenceMouseDelta(Vec2 delta, const Settings& settings) {
    if (settings.invert_y) delta.y = -delta.y;
    return delta;
}

} // namespace writeover
