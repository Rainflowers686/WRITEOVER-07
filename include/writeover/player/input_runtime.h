#pragma once
// InputRuntime: minimal integration seam that samples a keyboard backend and
// a mouse backend each sim tick and resolves them into an InputState via an
// InputMapper. Backends are injectable, so unit tests can prove the full
// path (fake mouse backend -> Poll/ConsumeMouseDelta -> InputState.mouse_delta
// -> ApplyMouseLook -> yaw/pitch) without Win32. The composition root owns an
// InputRuntime wired to the platform backends.

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
    //  2. drains all keyboard events, mapping PhysicalKey -> GameAction
    //  3. drives the mouse backend Poll until no new pointer sample/event
    //  4. ConsumeMouseDelta -> InputState.mouse_delta
    //  5. on focus loss, clears all input state
    void SampleTick(InputState& state, const InputMapper& mapper);

    IInputBackend* Keyboard() const { return keyboard_.get(); }
    IInputBackend* Mouse() const { return mouse_.get(); }

private:
    std::unique_ptr<IInputBackend> keyboard_;
    std::unique_ptr<IInputBackend> mouse_;
};

} // namespace writeover