#pragma once
// Input system: PhysicalKey stream (backend) -> InputMapper -> GameAction.
// Gameplay reads only InputState actions. Backends are implemented in the
// platform layer; fallbacks (RawInput -> cursor delta -> keyboard-only) swap
// the backend implementation without touching gameplay (M-013 closure).

#include "writeover/common/input_types.h"
#include "writeover/common/serialize.h"
#include "writeover/common/types.h"

#include <array>
#include <cstdint>
#include <memory>

namespace writeover {

// Resolved per-frame input state read by gameplay.
struct InputState {
    Vec2 mouse_delta;             // raw, unbounded
    Vec2 left_stick;              // -1..1
    Vec2 right_stick;             // -1..1
    float left_trigger = 0.0f;
    float right_trigger = 0.0f;
    std::array<bool, kGameActionCount> action_down = {};
    std::array<bool, kGameActionCount> action_pressed = {};
    std::array<bool, kGameActionCount> action_released = {};
    bool has_focus = true;
    bool ime_composing = false;
};

// PhysicalKey -> GameAction binding table. Serialized with the settings.
class InputMapper {
public:
    InputMapper();

    void SetBinding(GameAction action, PhysicalKey key);
    PhysicalKey GetBinding(GameAction action) const;
    // First matching binding, or Unknown.
    GameAction MapKey(PhysicalKey key) const;
    void ResetToDefaults();

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

    const std::array<PhysicalKey, kGameActionCount>& Bindings() const { return bindings_; }

private:
    std::array<PhysicalKey, kGameActionCount> bindings_ = {};
};

// Platform backend factories (implemented in src/platform/windows/).
std::unique_ptr<IInputBackend> CreateKeyboardOnlyBackend();
std::unique_ptr<IInputBackend> CreateCursorDeltaBackend();

// Focus-lost helper: clears all action states (called by the app when the
// backend reports loss of focus, per input contract).
void ClearInputState(InputState& state);

} // namespace writeover