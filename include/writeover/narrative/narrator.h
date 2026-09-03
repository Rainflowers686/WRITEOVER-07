#pragma once
// Narrator: ONE voice identity, three persona states. The narrator can NOT
// mutate the FactStore directly (M-011 closure: the reliable truth layer must
// stay trustworthy). It holds a NarratorCapabilitySet and issues TYPED
// WorldCommands; world authoritative systems validate capability, apply the
// mutation, and emit WorldEvents the player can observe and counter.

#include "writeover/common/command.h"
#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"

#include <cstdint>

namespace writeover {

enum class NarratorPersona : uint8_t {
    Guide = 0,      // normal, instructive, mostly truthful
    Director = 1,   // authoritative, manipulative, agenda-driven
    Corrupted = 2,  // adversarial / inconsistent
};

struct NarratorCapabilitySet {
    bool lock_doors = false;
    bool open_doors = false;
    bool toggle_power = false;
    bool control_comms = false;
};

struct NarratorState {
    NarratorPersona persona = NarratorPersona::Guide;
    uint8_t tension = 0;      // 0-100
    uint8_t trust = 100;      // 0-100
    uint8_t corruption = 0;   // 0-100
    uint32_t line_cooldown_frames = 0;
    NarratorCapabilitySet capabilities;
};

class NarratorSystem {
public:
    // Capability-gated command production. Returns false when the capability
    // is unavailable; the caller must not fabricate the command.
    bool TryIssueLockDoor(DoorId door, NarratorState& state, WorldCommand& out) const;
    bool TryIssueOpenDoor(DoorId door, NarratorState& state, WorldCommand& out) const;
    bool TryIssueTogglePower(SystemId system, bool powered,
                             NarratorState& state, WorldCommand& out) const;

    // Persona dynamics (presentation-only; never touches facts).
    void ApplyPersonaShift(NarratorState& state, NarratorPersona persona);

    void Save(Serializer& s) const;
    void Load(Deserializer& d);
};

} // namespace writeover