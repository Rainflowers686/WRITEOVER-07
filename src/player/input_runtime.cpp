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
    // Keyboard/console backend focus is authoritative (§13). When no keyboard
    // exists (unusual), fall back to the pointer's view.
    if (keyboard_) return keyboard_->HasFocus();
    return mouse_ ? mouse_->HasFocus() : false;
}

void InputRuntime::SetActiveContext(InputContext ctx) {
    active_context_ = ctx;
}

void InputRuntime::SampleTick(InputState& state, const InputMapper& mapper) {
    state.action_pressed.fill(false);
    state.action_released.fill(false);
    state.mouse_delta = Vec2{};

    // Focus authority = keyboard backend (console focus). Pointer backend
    // focus is used only internally (Raw Input has no traditional focus).
    const bool authoritative_focus = keyboard_
        ? keyboard_->HasFocus()
        : (mouse_ ? mouse_->HasFocus() : false);
    state.has_focus = authoritative_focus;

    InputEvent evt;

    if (!authoritative_focus) {
        // Focus loss: clear everything. Backends that receive background
        // pointer input (Raw Input with RIDEV_INPUTSINK) are drained and
        // discarded so no backlog can be applied on regain.
        ClearInputState(state);
        if (mouse_ && mouse_->NeedsBackgroundDrain()) {
            while (mouse_->Poll(evt)) {}
            Vec2 discard;
            mouse_->ConsumeMouseDelta(discard);
        }
        previous_focus_ = false;
        return;
    }

    // Focus regain: rebase the pointer and discard any remaining backlog.
    // This tick's pointer delta is suppressed (no huge first delta); the
    // next real movement tick applies normally.
    bool suppress_delta_this_tick = false;
    if (!previous_focus_) {
        if (mouse_) {
            mouse_->RebasePointer();
            while (mouse_->Poll(evt)) {}
            Vec2 discard;
            mouse_->ConsumeMouseDelta(discard);
        }
        previous_focus_ = true;
        suppress_delta_this_tick = true;
    }

    // Context transition: clear stale action_down from the previous context
    // so held keys never stick into the new context.
    if (active_context_ != last_context_) {
        ClearInputState(state);
        last_context_ = active_context_;
    }

    // Poll keyboard: drain all pending events and map through the ACTIVE
    // input context (never hardcoded Gameplay).
    while (keyboard_ && keyboard_->Poll(evt)) {
        if (evt.key == PhysicalKey::Unknown) {
            continue;
        }
        const GameAction a = mapper.MapKey(active_context_, evt.key);
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

    // Drive the mouse backend sampling (raw or cursor). Buttons from the
    // pointer backend are also mapped through the active context.
    while (mouse_ && mouse_->Poll(evt)) {
        if (evt.key == PhysicalKey::Unknown) {
            continue;  // sample-only; delta consumed below
        }
        const GameAction a = mapper.MapKey(active_context_, evt.key);
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

    // Consume the accumulated pointer delta. On the focus-regain tick the
    // delta is discarded (already rebased); subsequent ticks apply normally.
    Vec2 delta;
    if (mouse_ && mouse_->ConsumeMouseDelta(delta)) {
        if (!suppress_delta_this_tick) {
            state.mouse_delta = delta;
        }
    }
}

} // namespace writeover