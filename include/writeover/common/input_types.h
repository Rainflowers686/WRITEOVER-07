#pragma once
// Input vocabulary (platform-agnostic). Gameplay code reads GameAction, never
// Win32 virtual key codes. PhysicalKey is the raw event vocabulary; the
// InputMapper (player module) maps PhysicalKey -> GameAction with a
// serializable binding table (M-013 closure: no `uint8_t keyBindings` pointer).
// InputContext lives here so core/settings.h can reference it without
// depending on player/input.h.

#include "writeover/common/types.h"

#include <cstddef>
#include <cstdint>
#include <deque>

namespace writeover {

enum class PhysicalKey : uint16_t {
    // Keyboard
    W = 0, A, S, D, Q, E, R, F, Z, X, C, V, B, N, M,
    Space, Shift, Ctrl, Tab, Escape,
    Up, Down, Left, Right,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    // Mouse
    MouseLeft, MouseRight, MouseMiddle, MouseX1, MouseX2,
    // Gamepad
    GamepadDPadUp, GamepadDPadDown, GamepadDPadLeft, GamepadDPadRight,
    GamepadA, GamepadB, GamepadX, GamepadY,
    GamepadLB, GamepadRB, GamepadLT, GamepadRT,
    GamepadStart, GamepadBack, GamepadLStick, GamepadRStick,
    Count,          // NOT a key; array sizing only
    Unknown = 0xFFFF,
};

inline constexpr size_t kPhysicalKeyCount = static_cast<size_t>(PhysicalKey::Count);

enum class GameAction : uint8_t {
    MoveForward = 0, MoveBackward, MoveLeft, MoveRight,
    Sprint, Jump, Crouch, Prone,
    LeanLeft, LeanRight, Interact, Reload,
    Fire, AimDownSights, Melee,
    WeaponSlot1, WeaponSlot2, WeaponSlot3,
    Pause, DevPanel, Help,
    DialogOption1, DialogOption2, DialogOption3, DialogOption4,
    Count,
};

inline constexpr size_t kGameActionCount = static_cast<size_t>(GameAction::Count);

// Input context: which surface consumes a physical key. Gameplay and
// Dialogue legally share keys (number row); Menu and Developer are separate.
// Lives here so Settings can reference it without depending on player/input.h.
enum class InputContext : uint8_t {
    Gameplay = 0,
    Dialogue = 1,
    Menu = 2,
    Developer = 3,
    Count = 4,
};

inline constexpr size_t kInputContextCount = static_cast<size_t>(InputContext::Count);

// Raw per-frame input event produced by a backend.
struct InputEvent {
    PhysicalKey key = PhysicalKey::Unknown;
    bool pressed = false;       // true = down this frame, false = released
    float analog = 0.0f;        // 0..1 for analog inputs
};

// Input backend interface. Implementations live in the platform layer;
// gameplay code only ever sees the resolved InputState.
class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    virtual bool Init() = 0;
    virtual void Shutdown() = 0;
    // Non-blocking. Returns false when no more events are available.
    virtual bool Poll(InputEvent& out_event) = 0;
    virtual bool HasFocus() const = 0;
    virtual const char* Name() const = 0;
    // Consumes accumulated pointer movement since the last call. Returns
    // false when the backend has no pointer delta source (e.g. keyboard-only).
    // Default: no pointer delta.
    virtual bool ConsumeMouseDelta(Vec2& out) {
        (void)out;
        return false;
    }
};

// ---------------------------------------------------------------------------
// Platform-neutral console-input translation seam (unit-testable without
// Win32). The Windows keyboard backend converts INPUT_RECORDs into these
// records and applies them through ApplyInputBatchRecord; tests exercise the
// same seam to prove that one ReadConsoleInput batch never drops events.
// ---------------------------------------------------------------------------

// Mouse button masks (bit positions used by DiffMouseButtons).
inline constexpr uint8_t kMouseMaskLeft = 0x01;
inline constexpr uint8_t kMouseMaskRight = 0x02;
inline constexpr uint8_t kMouseMaskMiddle = 0x04;

// One transition produced by DiffMouseButtons (down or up for a button).
struct MouseButtonTransition {
    PhysicalKey key = PhysicalKey::Unknown;
    bool pressed = false;
};

// Diffs `current` mouse-button mask against `previous` (updated in place) and
// writes one transition per changed button (order: left, right, middle).
// Returns the number of transitions written. Pure / platform-agnostic.
inline size_t DiffMouseButtons(uint8_t current, uint8_t& previous,
                               MouseButtonTransition* out, size_t capacity) {
    size_t n = 0;
    const uint8_t changed = static_cast<uint8_t>(current ^ previous);
    for (uint8_t i = 0; i < 3 && n < capacity; ++i) {
        const uint8_t bit = static_cast<uint8_t>(1u << i);
        if ((changed & bit) == 0) {
            continue;
        }
        PhysicalKey key = PhysicalKey::Unknown;
        switch (bit) {
        case kMouseMaskLeft: key = PhysicalKey::MouseLeft; break;
        case kMouseMaskRight: key = PhysicalKey::MouseRight; break;
        default: key = PhysicalKey::MouseMiddle; break;
        }
        out[n++] = {key, (current & bit) != 0};
    }
    previous = current;
    return n;
}

// One translated console record in a batch (platform-neutral).
struct InputBatchRecord {
    enum class Kind : uint8_t { Key = 0, MouseButton = 1, Focus = 2 };
    Kind kind = Kind::Key;
    PhysicalKey key = PhysicalKey::Unknown;  // Key records
    bool pressed = false;                    // Key records
    uint8_t button_mask = 0;                 // MouseButton records
    bool focused = true;                     // Focus records
};

// Applies one batch record to the queue:
//   Key         -> queued as-is when key != Unknown (never dropped)
//   MouseButton -> diffed against `prev_mouse_mask` (down/up transitions)
//   Focus       -> updates `has_focus`
inline void ApplyInputBatchRecord(const InputBatchRecord& rec,
                                  std::deque<InputEvent>& queue,
                                  bool& has_focus, uint8_t& prev_mouse_mask) {
    if (rec.kind == InputBatchRecord::Kind::Focus) {
        has_focus = rec.focused;
        return;
    }
    if (rec.kind == InputBatchRecord::Kind::MouseButton) {
        MouseButtonTransition trans[3];
        const size_t n = DiffMouseButtons(rec.button_mask, prev_mouse_mask,
                                          trans, 3);
        for (size_t i = 0; i < n; ++i) {
            queue.push_back({trans[i].key, trans[i].pressed, 0.0f});
        }
        return;
    }
    if (rec.key != PhysicalKey::Unknown) {
        queue.push_back({rec.key, rec.pressed, 0.0f});
    }
}

} // namespace writeover