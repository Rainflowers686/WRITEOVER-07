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
std::unique_ptr<IInputBackend> CreateRawInputMouseBackend();

// Pointer backend preference used by the app + the input probe. Auto tries
// Raw Input first and falls back to CursorDelta only on Init failure.
enum class PointerBackendPreference : uint8_t {
    Auto = 0,   // Raw Input primary, CursorDelta fallback
    RawInput = 1,
    Cursor = 2,
};

// Result of building the runtime keyboard + pointer backends. `keyboard` is
// the console keyboard backend (also carries focus authority); `pointer` is
// either RawInputMouseBackend or CursorDeltaBackend. When Raw Input is
// active, the keyboard backend's console MOUSE_EVENT buttons are disabled so
// no double press/release occurs (single mouse-button source).
struct PointerBackendSelection {
    std::unique_ptr<IInputBackend> keyboard;
    std::unique_ptr<IInputBackend> pointer;
    bool raw_active = false;
    bool raw_init_failed = false;
    bool fallback_used = false;      // raw failed -> cursor fallback taken
    const char* pointer_name = "cursor-delta";       // "raw-input" / "cursor-delta"
    const char* mouse_button_source = "console";     // "raw-input" / "console"
};

// Builds the production keyboard + pointer backend pair honoring `pref`.
// Implemented in the Windows platform layer.
PointerBackendSelection CreateRuntimeBackendSelection(PointerBackendPreference pref);

// Focus-lost helper: clears all action states (called by the app when the
// backend reports loss of focus, per input contract).
void ClearInputState(InputState& state);

} // namespace writeover