#include "tests/test_harness.h"

#include "writeover/player/controller.h"
#include "writeover/player/input.h"
#include "writeover/player/input_runtime.h"

#include <deque>
#include <memory>
#include <utility>

namespace writeover {

namespace {

// Fake keyboard backend: feeds events that the runtime drains via Poll.
class FakeKeyboardBackend final : public IInputBackend {
public:
    bool Init() override { return true; }
    void Shutdown() override {}
    bool Poll(InputEvent& out) override {
        if (queue_.empty()) {
            return false;
        }
        out = queue_.front();
        queue_.pop_front();
        return true;
    }
    bool HasFocus() const override { return true; }
    const char* Name() const override { return "fake-keyboard"; }
    void Feed(const InputEvent& evt) { queue_.push_back(evt); }

private:
    std::deque<InputEvent> queue_;
};

// Fake mouse backend that mirrors the real CursorDeltaBackend contract:
// delta is accumulated DURING Poll (sampling), and ConsumeMouseDelta returns
// the accumulated total. If the runtime never calls Poll (the b455fc8 bug),
// the delta never reaches the accumulated buffer, so a missing Poll is
// caught by the integration tests.
class FakeMouseBackend final : public IInputBackend {
public:
    bool Init() override { return true; }
    void Shutdown() override {}
    bool Poll(InputEvent& out) override {
        out.key = PhysicalKey::Unknown;
        out.pressed = false;
        out.analog = 0.0f;
        if (!pending_) {
            return false;
        }
        // Deliver the queued sample into the accumulated buffer.
        delta_x_ += queued_x_;
        delta_y_ += queued_y_;
        queued_x_ = 0.0f;
        queued_y_ = 0.0f;
        pending_ = false;
        return true;
    }
    bool ConsumeMouseDelta(Vec2& out) override {
        if (delta_x_ == 0.0f && delta_y_ == 0.0f) {
            return false;
        }
        out.x = delta_x_;
        out.y = delta_y_;
        delta_x_ = 0.0f;
        delta_y_ = 0.0f;
        return true;
    }
    bool HasFocus() const override { return true; }
    const char* Name() const override { return "fake-mouse"; }
    void FeedDelta(float dx, float dy) {
        queued_x_ += dx;
        queued_y_ += dy;
        pending_ = true;
    }

private:
    float queued_x_ = 0.0f;
    float queued_y_ = 0.0f;
    float delta_x_ = 0.0f;
    float delta_y_ = 0.0f;
    bool pending_ = false;
};

// Builds an InputRuntime with fresh fakes; returns the runtime and hands back
// non-owning pointers to the fakes (the runtime owns them).
InputRuntime MakeRuntime(FakeKeyboardBackend** kb, FakeMouseBackend** ms) {
    auto keyboard = std::make_unique<FakeKeyboardBackend>();
    auto mouse = std::make_unique<FakeMouseBackend>();
    *kb = keyboard.get();
    *ms = mouse.get();
    return InputRuntime(std::move(keyboard), std::move(mouse));
}

// BLOCKER A integration: a fake mouse backend's delta must reach InputState
// through the runtime. Under b455fc8 the runtime never called mouse->Poll(),
// so ConsumeMouseDelta returned false and this test fails.
bool MouseBackendDeltaReachesInputState() {
    FakeKeyboardBackend* kb = nullptr;
    FakeMouseBackend* ms = nullptr;
    InputRuntime rt = MakeRuntime(&kb, &ms);
    rt.Init();
    InputState state;
    InputMapper mapper;
    ms->FeedDelta(120.0f, -30.0f);
    rt.SampleTick(state, mapper);
    WO_CHECK_NEAR(state.mouse_delta.x, 120.0f, 0.001f);
    WO_CHECK_NEAR(state.mouse_delta.y, -30.0f, 0.001f);
    return true;
}

// BLOCKER A integration (full path): mouse delta reaches InputState and then
// drives player look (yaw change) exactly as PlayerModule does each tick.
bool MouseBackendDeltaReachesPlayerLook() {
    FakeKeyboardBackend* kb = nullptr;
    FakeMouseBackend* ms = nullptr;
    InputRuntime rt = MakeRuntime(&kb, &ms);
    rt.Init();
    InputState state;
    InputMapper mapper;
    ms->FeedDelta(200.0f, 0.0f);
    rt.SampleTick(state, mapper);
    LocomotionState ls;
    ls.yaw = 0.0f;
    ls.pitch = 0.0f;
    ApplyMouseLook(ls, state.mouse_delta, 50);
    WO_CHECK(ls.yaw > 0.0f);          // horizontal look from positive dx
    WO_CHECK(ls.pitch == 0.0f);       // no vertical delta
    return true;
}

// BLOCKER B: LMB down -> up transitions via the platform-neutral diff helper.
bool MouseLeftDownUp() {
    uint8_t prev = 0;
    MouseButtonTransition trans[3];
    size_t n = DiffMouseButtons(kMouseMaskLeft, prev, trans, 3);
    WO_CHECK_EQ(static_cast<int64_t>(n), 1);
    WO_CHECK(trans[0].key == PhysicalKey::MouseLeft);
    WO_CHECK(trans[0].pressed);
    n = DiffMouseButtons(0, prev, trans, 3);
    WO_CHECK_EQ(static_cast<int64_t>(n), 1);
    WO_CHECK(trans[0].key == PhysicalKey::MouseLeft);
    WO_CHECK(!trans[0].pressed);
    return true;
}

// BLOCKER B: RMB down -> up transitions.
bool MouseRightDownUp() {
    uint8_t prev = 0;
    MouseButtonTransition trans[3];
    size_t n = DiffMouseButtons(kMouseMaskRight, prev, trans, 3);
    WO_CHECK_EQ(static_cast<int64_t>(n), 1);
    WO_CHECK(trans[0].key == PhysicalKey::MouseRight);
    WO_CHECK(trans[0].pressed);
    n = DiffMouseButtons(0, prev, trans, 3);
    WO_CHECK_EQ(static_cast<int64_t>(n), 1);
    WO_CHECK(trans[0].key == PhysicalKey::MouseRight);
    WO_CHECK(!trans[0].pressed);
    return true;
}

// BLOCKER B: one ReadConsoleInput-style batch mixing keys and mouse buttons
// must preserve every event in order (no drops).
bool MixedKeyboardMouseBatchPreserved() {
    std::deque<InputEvent> queue;
    bool has_focus = true;
    uint8_t prev_mask = 0;
    InputBatchRecord batch[] = {
        {InputBatchRecord::Kind::Key, PhysicalKey::W, true, 0, true},
        {InputBatchRecord::Kind::MouseButton, PhysicalKey::Unknown, false,
         kMouseMaskLeft, true},
        {InputBatchRecord::Kind::Key, PhysicalKey::W, false, 0, true},
        {InputBatchRecord::Kind::MouseButton, PhysicalKey::Unknown, false, 0,
         true},
    };
    for (const auto& rec : batch) {
        ApplyInputBatchRecord(rec, queue, has_focus, prev_mask);
    }
    WO_CHECK_EQ(static_cast<int64_t>(queue.size()), 4);
    if (queue.size() != 4) {
        return false;
    }
    const InputEvent e0 = queue[0];
    const InputEvent e1 = queue[1];
    const InputEvent e2 = queue[2];
    const InputEvent e3 = queue[3];
    WO_CHECK(e0.key == PhysicalKey::W && e0.pressed);
    WO_CHECK(e1.key == PhysicalKey::MouseLeft && e1.pressed);
    WO_CHECK(e2.key == PhysicalKey::W && !e2.pressed);
    WO_CHECK(e3.key == PhysicalKey::MouseLeft && !e3.pressed);
    return true;
}

// Focus record in a batch must update the tracked focus state.
bool FocusRecordUpdatesHasFocus() {
    std::deque<InputEvent> queue;
    bool has_focus = true;
    uint8_t prev_mask = 0;
    InputBatchRecord rec;
    rec.kind = InputBatchRecord::Kind::Focus;
    rec.focused = false;
    ApplyInputBatchRecord(rec, queue, has_focus, prev_mask);
    WO_CHECK(!has_focus);
    WO_CHECK(queue.empty());
    return true;
}

} // namespace

void RegisterInputTests(TestHarness& test) {
    test.Add("input.mouse_backend_delta_reaches_input_state",
             &MouseBackendDeltaReachesInputState);
    test.Add("input.mouse_backend_delta_reaches_player_look",
             &MouseBackendDeltaReachesPlayerLook);
    test.Add("input.mouse_left_down_up", &MouseLeftDownUp);
    test.Add("input.mouse_right_down_up", &MouseRightDownUp);
    test.Add("input.mixed_keyboard_mouse_batch_preserved",
             &MixedKeyboardMouseBatchPreserved);
    test.Add("input.focus_record_updates_has_focus", &FocusRecordUpdatesHasFocus);
}

} // namespace writeover