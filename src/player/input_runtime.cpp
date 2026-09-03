#include "writeover/player/input_runtime.h"

namespace writeover {

InputRuntime::InputRuntime(std::unique_ptr<IInputBackend> keyboard,
                           std::unique_ptr<IInputBackend> mouse)
    : keyboard_(std::move(keyboard)), mouse_(std::move(mouse)) {}

InputRuntime::~InputRuntime() { Shutdown(); }

bool InputRuntime::Init() {
    bool ok = true;
    if (keyboard_) ok = keyboard_->Init() && ok;
    if (mouse_) ok = mouse_->Init() && ok;
    return ok;
}

void InputRuntime::Shutdown() {
    if (keyboard_) keyboard_->Shutdown();
    if (mouse_) mouse_->Shutdown();
}

bool InputRuntime::HasFocus() const {
    const bool kb = keyboard_ ? keyboard_->HasFocus() : false;
    const bool ms = mouse_ ? mouse_->HasFocus() : false;
    return kb && ms;
}

void InputRuntime::SampleTick(InputState& state, const InputMapper& mapper) {
    // Clear transient state from the previous tick.
    state.action_pressed.fill(false);
    state.action_released.fill(false);
    state.mouse_delta = Vec2{};

    // Focus check: when unfocused, clear all state.
    const bool focus = HasFocus();
    state.has_focus = focus;
    if (!focus) {
        ClearInputState(state);
        return;
    }

    // Poll keyboard: drain all pending events and map to GameActions.
    InputEvent evt;
    while (keyboard_ && keyboard_->Poll(evt)) {
        if (evt.key == PhysicalKey::Unknown) {
            continue;  // pointer-sample marker from a mouse backend; skip
        }
        const GameAction a = mapper.MapKey(InputContext::Gameplay, evt.key);
        if (a == GameAction::Count) {
            continue;
        }
        const size_t idx = static_cast<size_t>(a);
        if (evt.pressed) {
            if (!state.action_down[idx]) {
                state.action_pressed[idx] = true;
            }
            state.action_down[idx] = true;
        } else {
            state.action_down[idx] = false;
            state.action_released[idx] = true;
        }
    }

    // Drive the mouse backend sampling: Poll() is what samples the pointer
    // (CursorDeltaBackend accumulates delta + recenters inside Poll). Drain
    // until no new pointer sample/event is available, then consume the
    // accumulated delta. Never treat an Unknown-key sample as a keyboard
    // action.
    while (mouse_ && mouse_->Poll(evt)) {
        if (evt.key == PhysicalKey::Unknown) {
            continue;  // sample-only; delta is consumed below
        }
        const GameAction a = mapper.MapKey(InputContext::Gameplay, evt.key);
        if (a == GameAction::Count) {
            continue;
        }
        const size_t idx = static_cast<size_t>(a);
        if (evt.pressed) {
            if (!state.action_down[idx]) {
                state.action_pressed[idx] = true;
            }
            state.action_down[idx] = true;
        } else {
            state.action_down[idx] = false;
            state.action_released[idx] = true;
        }
    }

    // Consume the accumulated pointer delta.
    Vec2 delta;
    if (mouse_ && mouse_->ConsumeMouseDelta(delta)) {
        state.mouse_delta = delta;
    }
}

} // namespace writeover