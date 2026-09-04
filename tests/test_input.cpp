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
// Focus can be toggled to test focus-loss/regain behavior.
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
    bool HasFocus() const override { return focused_; }
    const char* Name() const override { return "fake-keyboard"; }
    void Feed(const InputEvent& evt) { queue_.push_back(evt); }
    void SetFocus(bool f) { focused_ = f; }

private:
    std::deque<InputEvent> queue_;
    bool focused_ = true;
};

// Fake mouse backend that mirrors the real CursorDeltaBackend contract:
// delta is accumulated DURING Poll (sampling), and ConsumeMouseDelta returns
// the accumulated total. Focus + RebasePointer are controllable so the R0
// focus-regain tests can prove backlog discard / rebaseline.
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
    bool HasFocus() const override { return focused_; }
    const char* Name() const override { return "fake-mouse"; }
    void FeedDelta(float dx, float dy) {
        queued_x_ += dx;
        queued_y_ += dy;
        pending_ = true;
    }
    void SetFocus(bool f) { focused_ = f; }
    void SetNeedsBackgroundDrain(bool b) { needs_drain_ = b; }
    bool NeedsBackgroundDrain() const override { return needs_drain_; }
    // R0 focus-regain rebaseline: discard any backlog.
    void RebasePointer() override {
        queued_x_ = 0.0f;
        queued_y_ = 0.0f;
        delta_x_ = 0.0f;
        delta_y_ = 0.0f;
        pending_ = false;
        ++rebase_count_;
    }
    int RebaseCount() const { return rebase_count_; }
    float PendingDelta() const { return queued_x_ + queued_y_; }

private:
    float queued_x_ = 0.0f;
    float queued_y_ = 0.0f;
    float delta_x_ = 0.0f;
    float delta_y_ = 0.0f;
    bool pending_ = false;
    bool focused_ = true;
    bool needs_drain_ = false;
    int rebase_count_ = 0;
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

// Raw-like fake: mirrors RawInputMouseBackend where Poll pumps accumulated
// movement into an internal delta and returns false when only movement
// occurred (no button events); ConsumeMouseDelta still delivers the delta.
// Proves the runtime applies raw pointer movement via ConsumeMouseDelta even
// when the raw Poll reports no button event.
class RawLikeMouseBackend final : public IInputBackend {
public:
    bool Init() override { return true; }
    void Shutdown() override {}
    bool Poll(InputEvent& out) override {
        out.key = PhysicalKey::Unknown;
        out.pressed = false;
        out.analog = 0.0f;
        // Absorb queued movement into the accumulated delta (raw pump).
        acc_x_ += queued_x_;
        acc_y_ += queued_y_;
        queued_x_ = 0.0f;
        queued_y_ = 0.0f;
        if (!button_queue_.empty()) {
            out = button_queue_.front();
            button_queue_.pop_front();
            return true;
        }
        return false;  // movement-only sample: no key event
    }
    bool ConsumeMouseDelta(Vec2& out) override {
        if (acc_x_ == 0.0f && acc_y_ == 0.0f) return false;
        out.x = acc_x_;
        out.y = acc_y_;
        acc_x_ = 0.0f;
        acc_y_ = 0.0f;
        return true;
    }
    bool HasFocus() const override { return true; }
    bool NeedsBackgroundDrain() const override { return true; }
    const char* Name() const override { return "fake-raw-mouse"; }
    void FeedMovement(float dx, float dy) {
        queued_x_ += dx;
        queued_y_ += dy;
    }
    void FeedButton(PhysicalKey key, bool pressed) {
        button_queue_.push_back({key, pressed, 0.0f});
    }

private:
    float queued_x_ = 0.0f;
    float queued_y_ = 0.0f;
    float acc_x_ = 0.0f;
    float acc_y_ = 0.0f;
    std::deque<InputEvent> button_queue_;
};

// R0: the runtime drives a Raw-Input-like pointer backend and applies its
// relative delta to InputState (movement-only Poll returns false, but the
// accumulated delta is still consumed).
bool RuntimeUsesRawPointerDelta() {
    auto kb = std::make_unique<FakeKeyboardBackend>();
    auto ms = std::make_unique<RawLikeMouseBackend>();
    RawLikeMouseBackend* mouse = ms.get();
    InputRuntime rt(std::move(kb), std::move(ms));
    rt.Init();
    InputState state;
    InputMapper mapper;
    mouse->FeedMovement(40.0f, -12.0f);
    rt.SampleTick(state, mapper);
    WO_CHECK_NEAR(state.mouse_delta.x, 40.0f, 0.001f);
    WO_CHECK_NEAR(state.mouse_delta.y, -12.0f, 0.001f);
    return true;
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

// ---------------------------------------------------------------------------
// R0: Raw Input translation seam (platform-neutral, no Win32 types)
// ---------------------------------------------------------------------------

// A relative raw packet contributes dx/dy to the accumulated delta.
bool RawRelativeDeltaTranslation() {
    Vec2 acc{};
    std::deque<InputEvent> queue;
    RawMousePacket packet;
    packet.dx = 25;
    packet.dy = -7;
    packet.flags = kRawMouseMoveRelative;
    const size_t n = TranslateRawMousePacket(packet, acc, queue);
    WO_CHECK_EQ(static_cast<int64_t>(n), 0);  // movement only
    WO_CHECK_NEAR(acc.x, 25.0f, 0.001f);
    WO_CHECK_NEAR(acc.y, -7.0f, 0.001f);
    return queue.empty();
}

// LMB down then up produce exactly one down + one up event each.
bool RawLeftDownUp() {
    Vec2 acc{};
    std::deque<InputEvent> queue;
    RawMousePacket down;
    down.button_flags = kRawLeftButtonDown;
    TranslateRawMousePacket(down, acc, queue);
    WO_CHECK_EQ(static_cast<int64_t>(queue.size()), 1);
    WO_CHECK(queue[0].key == PhysicalKey::MouseLeft && queue[0].pressed);
    queue.clear();
    RawMousePacket up;
    up.button_flags = kRawLeftButtonUp;
    TranslateRawMousePacket(up, acc, queue);
    WO_CHECK_EQ(static_cast<int64_t>(queue.size()), 1);
    WO_CHECK(queue[0].key == PhysicalKey::MouseLeft && !queue[0].pressed);
    return true;
}

// RMB down then up produce exactly one down + one up event each.
bool RawRightDownUp() {
    Vec2 acc{};
    std::deque<InputEvent> queue;
    RawMousePacket down;
    down.button_flags = kRawRightButtonDown;
    TranslateRawMousePacket(down, acc, queue);
    WO_CHECK_EQ(static_cast<int64_t>(queue.size()), 1);
    WO_CHECK(queue[0].key == PhysicalKey::MouseRight && queue[0].pressed);
    queue.clear();
    RawMousePacket up;
    up.button_flags = kRawRightButtonUp;
    TranslateRawMousePacket(up, acc, queue);
    WO_CHECK_EQ(static_cast<int64_t>(queue.size()), 1);
    WO_CHECK(queue[0].key == PhysicalKey::MouseRight && !queue[0].pressed);
    return true;
}

// A button flag bit produces exactly one transition (no duplicate).
bool RawButtonNoDuplicateTransition() {
    Vec2 acc{};
    std::deque<InputEvent> queue;
    RawMousePacket p;
    p.button_flags = kRawLeftButtonDown | kRawMiddleButtonDown;
    const size_t n = TranslateRawMousePacket(p, acc, queue);
    WO_CHECK_EQ(static_cast<int64_t>(n), 2);
    WO_CHECK_EQ(static_cast<int64_t>(queue.size()), 2);
    WO_CHECK(queue[0].key == PhysicalKey::MouseLeft);
    WO_CHECK(queue[1].key == PhysicalKey::MouseMiddle);
    // No duplicate down for the same button in a single packet.
    size_t lefts = 0;
    for (const auto& e : queue) {
        if (e.key == PhysicalKey::MouseLeft && e.pressed) ++lefts;
    }
    return lefts == 1;
}

// ---------------------------------------------------------------------------
// R0: Active InputContext through the runtime
// ---------------------------------------------------------------------------

// Gameplay context: Num1 -> WeaponSlot1 (and NOT DialogOption1).
bool RuntimeContextGameplayNum1Weapon() {
    FakeKeyboardBackend* kb = nullptr;
    FakeMouseBackend* ms = nullptr;
    InputRuntime rt = MakeRuntime(&kb, &ms);
    rt.Init();
    rt.SetActiveContext(InputContext::Gameplay);
    InputState state;
    InputMapper mapper;
    kb->Feed({PhysicalKey::Num1, true, 0.0f});
    rt.SampleTick(state, mapper);
    const size_t weapon = static_cast<size_t>(GameAction::WeaponSlot1);
    const size_t dialog = static_cast<size_t>(GameAction::DialogOption1);
    WO_CHECK(state.action_pressed[weapon]);
    WO_CHECK(!state.action_pressed[dialog]);
    return true;
}

// Dialogue context: Num1 -> DialogOption1 (and NOT WeaponSlot1). Under the
// old hardcoded-Gameplay runtime this FAILS.
bool RuntimeContextDialogueNum1Dialog() {
    FakeKeyboardBackend* kb = nullptr;
    FakeMouseBackend* ms = nullptr;
    InputRuntime rt = MakeRuntime(&kb, &ms);
    rt.Init();
    rt.SetActiveContext(InputContext::Dialogue);
    InputState state;
    InputMapper mapper;
    kb->Feed({PhysicalKey::Num1, true, 0.0f});
    rt.SampleTick(state, mapper);
    const size_t weapon = static_cast<size_t>(GameAction::WeaponSlot1);
    const size_t dialog = static_cast<size_t>(GameAction::DialogOption1);
    WO_CHECK(state.action_pressed[dialog]);
    WO_CHECK(!state.action_pressed[weapon]);
    return true;
}

// Switching context must clear stale action_down from the previous context.
bool ContextSwitchClearsStaleActions() {
    FakeKeyboardBackend* kb = nullptr;
    FakeMouseBackend* ms = nullptr;
    InputRuntime rt = MakeRuntime(&kb, &ms);
    rt.Init();
    InputState state;
    InputMapper mapper;
    // Hold W in Gameplay.
    rt.SetActiveContext(InputContext::Gameplay);
    kb->Feed({PhysicalKey::W, true, 0.0f});
    rt.SampleTick(state, mapper);
    WO_CHECK(state.action_down[static_cast<size_t>(GameAction::MoveForward)]);
    // Switch to Dialogue with W held: stale down must clear.
    rt.SetActiveContext(InputContext::Dialogue);
    rt.SampleTick(state, mapper);
    WO_CHECK(!state.action_down[static_cast<size_t>(GameAction::MoveForward)]);
    return true;
}

// ---------------------------------------------------------------------------
// R0: pointer backlog discard on focus regain
// ---------------------------------------------------------------------------

// While unfocused, a background-draining pointer backend is drained/discarded
// so its backlog is never applied after focus regain. The regain tick itself
// must not inject a huge delta; fresh movement on later ticks still applies.
bool FocusRegainDiscardsPointerBacklog() {
    FakeKeyboardBackend* kb = nullptr;
    FakeMouseBackend* ms = nullptr;
    InputRuntime rt = MakeRuntime(&kb, &ms);
    rt.Init();
    ms->SetNeedsBackgroundDrain(true);
    InputState state;
    InputMapper mapper;
    // Lose focus, then feed a huge background delta.
    kb->SetFocus(false);
    ms->SetFocus(false);
    ms->FeedDelta(10000.0f, 0.0f);
    rt.SampleTick(state, mapper);
    WO_CHECK(!state.has_focus);
    WO_CHECK_NEAR(state.mouse_delta.x, 0.0f, 0.001f);  // discarded while unfocused
    // Regain focus: no stale backlog may appear on the regain tick.
    kb->SetFocus(true);
    ms->SetFocus(true);
    rt.SampleTick(state, mapper);
    WO_CHECK(state.has_focus);
    WO_CHECK_NEAR(state.mouse_delta.x, 0.0f, 0.001f);  // NOT 10000
    // Fresh real movement on the next tick applies normally.
    ms->FeedDelta(3.0f, 0.0f);
    rt.SampleTick(state, mapper);
    WO_CHECK_NEAR(state.mouse_delta.x, 3.0f, 0.001f);
    return true;
}

// On focus regain the pointer is rebased so the first frame has no huge
// delta even if a stale sample is pending in the backend.
bool FocusRegainNoHugeFirstDelta() {
    FakeKeyboardBackend* kb = nullptr;
    FakeMouseBackend* ms = nullptr;
    InputRuntime rt = MakeRuntime(&kb, &ms);
    rt.Init();
    InputState state;
    InputMapper mapper;
    // Lose focus (cursor backend: no background drain), leave a pending delta.
    kb->SetFocus(false);
    ms->SetFocus(false);
    rt.SampleTick(state, mapper);
    ms->FeedDelta(8000.0f, 0.0f);  // stale cursor movement while unfocused
    // Regain focus: RebasePointer must discard the stale delta.
    kb->SetFocus(true);
    ms->SetFocus(true);
    rt.SampleTick(state, mapper);
    WO_CHECK(state.has_focus);
    WO_CHECK_NEAR(state.mouse_delta.x, 0.0f, 0.001f);  // rebased, no 8000 jump
    WO_CHECK(ms->RebaseCount() >= 1);
    // Subsequent fresh movement is applied.
    ms->FeedDelta(2.0f, 0.0f);
    rt.SampleTick(state, mapper);
    WO_CHECK_NEAR(state.mouse_delta.x, 2.0f, 0.001f);
    return true;
}

} // namespace

void RegisterInputTests(TestHarness& test) {
    test.Add("input.runtime_uses_raw_pointer_delta",
             &RuntimeUsesRawPointerDelta);
    test.Add("input.mouse_backend_delta_reaches_input_state",
             &MouseBackendDeltaReachesInputState);
    test.Add("input.mouse_backend_delta_reaches_player_look",
             &MouseBackendDeltaReachesPlayerLook);
    test.Add("input.mouse_left_down_up", &MouseLeftDownUp);
    test.Add("input.mouse_right_down_up", &MouseRightDownUp);
    test.Add("input.mixed_keyboard_mouse_batch_preserved",
             &MixedKeyboardMouseBatchPreserved);
    test.Add("input.focus_record_updates_has_focus", &FocusRecordUpdatesHasFocus);
    // R0: Raw Input translation seam.
    test.Add("input.raw_relative_delta_translation", &RawRelativeDeltaTranslation);
    test.Add("input.raw_left_down_up", &RawLeftDownUp);
    test.Add("input.raw_right_down_up", &RawRightDownUp);
    test.Add("input.raw_button_no_duplicate_transition",
             &RawButtonNoDuplicateTransition);
    // R0: Active InputContext.
    test.Add("input.runtime_context_gameplay_num1_weapon",
             &RuntimeContextGameplayNum1Weapon);
    test.Add("input.runtime_context_dialogue_num1_dialog",
             &RuntimeContextDialogueNum1Dialog);
    test.Add("input.context_switch_clears_stale_actions",
             &ContextSwitchClearsStaleActions);
    // R0: focus regain backlog discard / rebaseline.
    test.Add("input.focus_regain_discards_pointer_backlog",
             &FocusRegainDiscardsPointerBacklog);
    test.Add("input.focus_regain_no_huge_first_delta",
             &FocusRegainNoHugeFirstDelta);
}

} // namespace writeover
