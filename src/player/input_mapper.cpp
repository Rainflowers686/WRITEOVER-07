#include "writeover/player/input.h"

namespace writeover {

InputMapper::InputMapper() { ResetToDefaults(); }

void InputMapper::ResetToDefaults() {
    for (auto& table : bindings_) {
        table.fill(PhysicalKey::Unknown);
    }
    auto& g = bindings_[static_cast<size_t>(InputContext::Gameplay)];
    g[static_cast<size_t>(GameAction::MoveForward)] = PhysicalKey::W;
    g[static_cast<size_t>(GameAction::MoveBackward)] = PhysicalKey::S;
    g[static_cast<size_t>(GameAction::MoveLeft)] = PhysicalKey::A;
    g[static_cast<size_t>(GameAction::MoveRight)] = PhysicalKey::D;
    g[static_cast<size_t>(GameAction::Sprint)] = PhysicalKey::Shift;
    g[static_cast<size_t>(GameAction::Jump)] = PhysicalKey::Space;
    g[static_cast<size_t>(GameAction::Crouch)] = PhysicalKey::Ctrl;
    g[static_cast<size_t>(GameAction::Prone)] = PhysicalKey::Z;
    g[static_cast<size_t>(GameAction::LeanLeft)] = PhysicalKey::Q;
    g[static_cast<size_t>(GameAction::LeanRight)] = PhysicalKey::E;
    g[static_cast<size_t>(GameAction::Interact)] = PhysicalKey::F;
    g[static_cast<size_t>(GameAction::Reload)] = PhysicalKey::R;
    g[static_cast<size_t>(GameAction::Fire)] = PhysicalKey::MouseLeft;
    g[static_cast<size_t>(GameAction::AimDownSights)] = PhysicalKey::MouseRight;
    g[static_cast<size_t>(GameAction::Melee)] = PhysicalKey::V;
    g[static_cast<size_t>(GameAction::WeaponSlot1)] = PhysicalKey::Num1;
    g[static_cast<size_t>(GameAction::WeaponSlot2)] = PhysicalKey::Num2;
    g[static_cast<size_t>(GameAction::WeaponSlot3)] = PhysicalKey::Num3;
    g[static_cast<size_t>(GameAction::Pause)] = PhysicalKey::Escape;
    g[static_cast<size_t>(GameAction::DevPanel)] = PhysicalKey::F3;
    g[static_cast<size_t>(GameAction::Help)] = PhysicalKey::F1;
    g[static_cast<size_t>(GameAction::SaveGame)] = PhysicalKey::F5;
    g[static_cast<size_t>(GameAction::LoadGame)] = PhysicalKey::F9;

    auto& dlg = bindings_[static_cast<size_t>(InputContext::Dialogue)];
    dlg[static_cast<size_t>(GameAction::DialogOption1)] = PhysicalKey::Num1;
    dlg[static_cast<size_t>(GameAction::DialogOption2)] = PhysicalKey::Num2;
    dlg[static_cast<size_t>(GameAction::DialogOption3)] = PhysicalKey::Num3;
    dlg[static_cast<size_t>(GameAction::DialogOption4)] = PhysicalKey::Num4;
    dlg[static_cast<size_t>(GameAction::MoveForward)] = PhysicalKey::W;
    dlg[static_cast<size_t>(GameAction::MoveBackward)] = PhysicalKey::S;
    dlg[static_cast<size_t>(GameAction::MoveLeft)] = PhysicalKey::A;
    dlg[static_cast<size_t>(GameAction::MoveRight)] = PhysicalKey::D;
    dlg[static_cast<size_t>(GameAction::Pause)] = PhysicalKey::Escape;
}

void InputMapper::SetBinding(GameAction action, PhysicalKey key) {
    SetBinding(InputContext::Gameplay, action, key);
}

void InputMapper::SetBinding(InputContext ctx, GameAction action, PhysicalKey key) {
    const size_t ci = static_cast<size_t>(ctx);
    const size_t ai = static_cast<size_t>(action);
    if (ci >= kInputContextCount || ai >= kGameActionCount) {
        return;
    }
    // Replace-conflict policy WITHIN this context: clear any action in the
    // same context currently bound to this key.
    for (size_t i = 0; i < kGameActionCount; ++i) {
        if (bindings_[ci][i] == key) {
            bindings_[ci][i] = PhysicalKey::Unknown;
        }
    }
    bindings_[ci][ai] = key;
}

PhysicalKey InputMapper::GetBinding(GameAction action) const {
    return GetBinding(InputContext::Gameplay, action);
}

PhysicalKey InputMapper::GetBinding(InputContext ctx, GameAction action) const {
    const size_t ci = static_cast<size_t>(ctx);
    const size_t ai = static_cast<size_t>(action);
    if (ci >= kInputContextCount || ai >= kGameActionCount) {
        return PhysicalKey::Unknown;
    }
    return bindings_[ci][ai];
}

GameAction InputMapper::MapKey(PhysicalKey key) const {
    return MapKey(InputContext::Gameplay, key);
}

GameAction InputMapper::MapKey(InputContext ctx, PhysicalKey key) const {
    const size_t ci = static_cast<size_t>(ctx);
    if (ci >= kInputContextCount) {
        return GameAction::Count;
    }
    for (size_t i = 0; i < kGameActionCount; ++i) {
        if (bindings_[ci][i] == key) {
            return static_cast<GameAction>(i);
        }
    }
    return GameAction::Count;  // caller treats Count as "unmapped"
}

void InputMapper::Save(Serializer& s) const {
    s.WriteU16(static_cast<uint16_t>(kInputContextCount));
    for (const auto& table : bindings_) {
        for (const auto key : table) {
            s.WriteU16(static_cast<uint16_t>(key));
        }
    }
}

void InputMapper::Load(Deserializer& d) {
    ResetToDefaults();
    const uint16_t contexts = d.ReadU16();
    const uint16_t limit = static_cast<uint16_t>(
        contexts > kInputContextCount ? kInputContextCount : contexts);
    for (uint16_t c = 0; c < limit; ++c) {
        for (uint16_t i = 0; i < kGameActionCount; ++i) {
            bindings_[c][i] = static_cast<PhysicalKey>(d.ReadU16());
        }
    }
}

void ClearInputState(InputState& state) {
    state.action_down.fill(false);
    state.action_pressed.fill(false);
    state.action_released.fill(false);
    state.mouse_delta = Vec2{};
    state.left_stick = Vec2{};
    state.right_stick = Vec2{};
    state.left_trigger = 0.0f;
    state.right_trigger = 0.0f;
}

} // namespace writeover
