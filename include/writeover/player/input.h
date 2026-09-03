#pragma once
// Input system: PhysicalKey stream (backend) -> InputMapper -> GameAction.
// Gameplay reads only InputState actions. Backends are implemented in the
// platform layer; fallbacks (RawInput -> cursor delta -> keyboard-only) swap
// the backend implementation without touching gameplay (M-013 closure).
//
// InputMapper is CONTEXT-AWARE: the same PhysicalKey may map to different
// GameActions in different contexts (e.g. Gameplay Num1 -> WeaponSlot1 while
// Dialogue Num1 -> DialogOption1). Within a single context the replace
// conflict policy applies. Contexts are orthogonal; no key is ever shared
// between contexts.

#include "writeover/common/input_types.h"
#include "writeover/common/serialize.h"
#include "writeover/common/types.h"

#include <array>
#include <cstdint>
#include <memory>

namespace writeover {

// Input context: which surface consumes a physical key. Gameplay and
// Dialogue legally share keys (number row); Menu and Developer are separate.
enum class InputContext : uint8_t {
    Gameplay = 0,
    Dialogue = 1,
    Menu = 2,
    Developer = 3,
    Count = 4,
};

inline constexpr size_t kInputContextCount = static_cast<size_t>(InputContext::Count);

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

// PhysicalKey -> GameAction binding table, per input context. Serialized
// with the settings. The gameplay default methods operate on Gameplay.
class InputMapper {
public:
    InputMapper();

    // Context-free overloads default to InputContext::Gameplay.
    void SetBinding(GameAction action, PhysicalKey key);
    PhysicalKey GetBinding(GameAction action) const;
    GameAction MapKey(PhysicalKey key) const;

    // Context-aware mapping: same key may differ across contexts.
    void SetBinding(InputContext ctx, GameAction action, PhysicalKey key);
    PhysicalKey GetBinding(InputContext ctx, GameAction action) const;
    // First matching binding in ctx, or GameAction::Count.
    GameAction MapKey(InputContext ctx, PhysicalKey key) const;

    void ResetToDefaults();

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

    const std::array<std::array<PhysicalKey, kGameActionCount>, kInputContextCount>&
    Bindings() const {
        return bindings_;
    }

private:
    // [context][action] -> key.
    std::array<std::array<PhysicalKey, kGameActionCount>, kInputContextCount> bindings_ = {};
};

// Platform backend factories (implemented in src/platform/windows/).
std::unique_ptr<IInputBackend> CreateKeyboardOnlyBackend();
std::unique_ptr<IInputBackend> CreateCursorDeltaBackend();

// Focus-lost helper: clears all action states (called by the app when the
// backend reports loss of focus, per input contract).
void ClearInputState(InputState& state);

} // namespace writeover