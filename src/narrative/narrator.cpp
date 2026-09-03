#include "writeover/narrative/narrator.h"

#include <algorithm>

namespace writeover {

bool NarratorSystem::TryIssueLockDoor(DoorId door, NarratorState& state,
                                      WorldCommand& out) const {
    if (!state.capabilities.lock_doors) {
        return false;
    }
    // Emit CommandSetDoor{open=false} with the semantics "lock"; the world
    // system applies validation and posts DoorChange events.
    out = WorldCommand{CommandSetDoor{door, false}};
    return true;
}

bool NarratorSystem::TryIssueOpenDoor(DoorId door, NarratorState& state,
                                      WorldCommand& out) const {
    if (!state.capabilities.open_doors) {
        return false;
    }
    out = WorldCommand{CommandSetDoor{door, true}};
    return true;
}

bool NarratorSystem::TryIssueTogglePower(SystemId system, bool powered,
                                         NarratorState& state,
                                         WorldCommand& out) const {
    if (!state.capabilities.toggle_power) {
        return false;
    }
    out = WorldCommand{CommandSetPower{system, powered}};
    return true;
}

void NarratorSystem::ApplyPersonaShift(NarratorState& state, NarratorPersona persona) {
    state.persona = persona;
    // Presentation-only counters; never touches the FactStore.
    state.corruption = static_cast<uint8_t>(std::min(
        100, state.corruption + (persona == NarratorPersona::Corrupted ? 5 : 1)));
}

void NarratorSystem::Save(Serializer& s) const {
    // NarratorSystem is stateless (no owned persistent data); persona state is
    // owned and saved by the app inside the narrative save section.
    s.WriteU8(0);  // reserved length prefix for forward compat
}

void NarratorSystem::Load(Deserializer& d) {
    (void)d.ReadU8();
}

} // namespace writeover