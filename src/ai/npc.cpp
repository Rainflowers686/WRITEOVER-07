#include "writeover/ai/npc.h"

namespace writeover {

// Simple per-NPC timer advance; used by the AI module and tests.
void UpdateNpcTimer(NPCInstance& npc, uint32_t ticks) {
    if (npc.state_timer_frames > ticks) {
        npc.state_timer_frames -= ticks;
    } else {
        npc.state_timer_frames = 0;
    }
}

} // namespace writeover