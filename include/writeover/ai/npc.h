#pragma once
// NPC data model. Content keys are ResourceIds, not raw const char*
// (M 17.3 closure). States are enum classes; deterministic scheduling is by
// ascending NpcId.

#include "writeover/common/ids.h"
#include "writeover/common/types.h"

#include <cstdint>

namespace writeover {

enum class NPCClass : uint8_t {
    Full = 0,
    Medium = 1,
    Light = 2,
    Guard = 3,
};

enum class NPCState : uint8_t {
    Idle = 0,
    Patrol = 1,
    Alert = 2,
    Investigate = 3,
    Combat = 4,
    Stunned = 5,
    Flee = 6,
    Dead = 7,
    Dialogue = 8,
    Busy = 9,
    Sleeping = 10,
    Count = 11,
};

struct NPCInstance {
    NpcId id;
    NPCClass npc_class = NPCClass::Light;
    ResourceId data_key;             // content-defined stable id
    Vec3 position;
    float yaw = 0.0f;
    uint16_t health = 40;
    uint8_t alertness = 0;           // 0-100
    uint8_t faction = 0;             // 0=guard 1=staff 2=civilian 3=player
    NPCState state = NPCState::Idle;
    uint32_t state_timer_frames = 0;
    bool is_critical = false;        // main-quest invariant: never offscreen-killed
    uint32_t plan_step = 0;
};

// Advances the per-NPC state timer deterministically.
void UpdateNpcTimer(NPCInstance& npc, uint32_t ticks);

} // namespace writeover