#pragma once
// NPC data model. Content keys are ResourceIds, not raw const char*
// (M 17.3 closure). States are enum classes; deterministic scheduling is by
// ascending NpcId.

#include "writeover/common/ids.h"
#include "writeover/common/result.h"
#include "writeover/common/types.h"
#include "writeover/systemic/systemic.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

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
    // Cognition and occupation are intentionally separate dimensions. A
    // guard is a role/faction, never a third cognitive tier.
    CognitionTier cognition = CognitionTier::SemiHuman;
    Faction faction = Faction::GeneralStaff;
    Role role = Role::Other;
    ResourceId data_key;             // content-defined stable id
    Vec3 position;
    float yaw = 0.0f;
    uint16_t health = 40;
    uint8_t alertness = 0;           // 0-100
    NPCState state = NPCState::Idle;
    uint32_t state_timer_frames = 0;
    bool is_critical = false;        // main-quest invariant: never offscreen-killed
    uint32_t plan_step = 0;
    // Authored perception overrides. Zero keeps the cognition-tier default;
    // the production NPC profile binary fills these from data/npcs.
    float sight_range = 0.0f;
    float sight_fov_rad = 0.0f;
    float hearing_range = 0.0f;
};

struct NpcProfile {
    NpcId id;
    RoomId spawn_room;
    Vec3 spawn_position;
    float spawn_yaw = 0.0f;
    uint16_t health = 100;
    bool is_critical = false;
    float sight_range = 0.0f;
    float sight_fov_rad = 0.0f;
    float hearing_range = 0.0f;
};

// Loads the bounded binary produced by tools/contentc/contentc.py. Runtime
// never parses NPC authoring JSON directly.
Result<std::vector<NpcProfile>> LoadNpcProfiles(const std::string& path);

// Advances the per-NPC state timer deterministically.
void UpdateNpcTimer(NPCInstance& npc, uint32_t ticks);

} // namespace writeover
