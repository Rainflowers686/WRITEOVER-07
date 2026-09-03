#pragma once
// InputRuntime: minimal integration seam that samples a keyboard backend and
// a mouse backend each sim tick and resolves them into an InputState via an
// InputMapper. Backends are injectable, so unit tests can prove the full
// path (fake mouse backend -> Poll/ConsumeMouseDelta -> InputState.mouse_delta
// -> ApplyMouseLook -> yaw/pitch) without Win32. The composition root owns an
// InputRuntime wired to the platform backends.
//
// Focus authority: the KEYBOARD/console backend focus is authoritative for
// gameplay. A Raw-Input pointer backend uses a message-only window and has no
// traditional foreground focus; its HasFocus is advisory only. On focus loss
// all input state clears and Raw-Input backends (RIDEV_INPUTSINK) keep being
// drained in the background so no backlog is applied on focus regain. On the
// regain tick the pointer backend is rebased (backlog discarded) before any
// new sample is applied.

#include "writeover/player/input.h"

#include <memory>

namespace writeover {

class InputRuntime {
public:
    InputRuntime(std::unique_ptr<IInputBackend> keyboard,
                 std::unique_ptr<IInputBackend> mouse);
    ~InputRuntime();
    InputRuntime(const InputRuntime&) = delete;
    InputRuntime& operator=(const InputRuntime&) = delete;
    InputRuntime(InputRuntime&&) noexcept = default;
    InputRuntime& operator=(InputRuntime&&) noexcept = default;

    bool Init();
    void Shutdown();
    bool HasFocus() const;

    // Samples both backends for one sim tick and writes the resolved state.
    //  1. clears action_pressed / action_released (keeps action_down)
    //  2. clears stale action_down when the active context changed
    //  3. drains all keyboard events, mapping PhysicalKey -> GameAction via
    //     the ACTIVE input context (never hardcoded Gameplay)
    //  4. drives the mouse backend Poll until no new pointer sample/event
    //  5. ConsumeMouseDelta -> InputState.mouse_delta
    //  6. on focus loss clears all input state and keeps background pointer
    //     drained/discarded; on focus regain rebases the pointer
    void SampleTick(InputState& state, const InputMapper& mapper);

    // Active input context (default Gameplay). Changing it clears stale
    // action_down on the next SampleTick so held keys in the previous context
    // never stick into the new one.
    void SetActiveContext(InputContext ctx);
    InputContext ActiveContext() const { return active_context_; }

    IInputBackend* Keyboard() const { return keyboard_.get(); }
    IInputBackend* Mouse() const { return mouse_.get(); }

private:
    std::unique_ptr<IInputBackend> keyboard_;
    std::unique_ptr<IInputBackend> mouse_;
    InputContext active_context_ = InputContext::Gameplay;
    InputContext last_context_ = InputContext::Gameplay;
    bool previous_focus_ = true;  // assume focused until first loss is seen
};

} // namespace writeover