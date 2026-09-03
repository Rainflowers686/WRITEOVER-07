#include "writeover/player/input.h"

namespace writeover {

InputMapper::InputMapper() { ResetToDefaults(); }

void InputMapper::ResetToDefaults() {
    bindings_.fill(PhysicalKey::Unknown);
    bindings_[static_cast<size_t>(GameAction::MoveForward)] = PhysicalKey::W;
    bindings_[static_cast<size_t>(GameAction::MoveBackward)] = PhysicalKey::S;
    bindings_[static_cast<size_t>(GameAction::MoveLeft)] = PhysicalKey::A;
    bindings_[static_cast<size_t>(GameAction::MoveRight)] = PhysicalKey::D;
    bindings_[static_cast<size_t>(GameAction::Sprint)] = PhysicalKey::Shift;
    bindings_[static_cast<size_t>(GameAction::Jump)] = PhysicalKey::Space;
    bindings_[static_cast<size_t>(GameAction::Crouch)] = PhysicalKey::Ctrl;
    bindings_[static_cast<size_t>(GameAction::Prone)] = PhysicalKey::Z;
    bindings_[static_cast<size_t>(GameAction::LeanLeft)] = PhysicalKey::Q;
    bindings_[static_cast<size_t>(GameAction::LeanRight)] = PhysicalKey::E;
    bindings_[static_cast<size_t>(GameAction::Interact)] = PhysicalKey::F;
    bindings_[static_cast<size_t>(GameAction::Reload)] = PhysicalKey::R;
    bindings_[static_cast<size_t>(GameAction::Fire)] = PhysicalKey::MouseLeft;
    bindings_[static_cast<size_t>(GameAction::AimDownSights)] = PhysicalKey::MouseRight;
    bindings_[static_cast<size_t>(GameAction::Melee)] = PhysicalKey::V;
    bindings_[static_cast<size_t>(GameAction::WeaponSlot1)] = PhysicalKey::Num1;
    bindings_[static_cast<size_t>(GameAction::WeaponSlot2)] = PhysicalKey::Num2;
    bindings_[static_cast<size_t>(GameAction::WeaponSlot3)] = PhysicalKey::Num3;
    bindings_[static_cast<size_t>(GameAction::Pause)] = PhysicalKey::Escape;
    bindings_[static_cast<size_t>(GameAction::DevPanel)] = PhysicalKey::F3;
    bindings_[static_cast<size_t>(GameAction::Help)] = PhysicalKey::F1;
    bindings_[static_cast<size_t>(GameAction::DialogOption1)] = PhysicalKey::Num1;
    bindings_[static_cast<size_t>(GameAction::DialogOption2)] = PhysicalKey::Num2;
    bindings_[static_cast<size_t>(GameAction::DialogOption3)] = PhysicalKey::Num3;
    bindings_[static_cast<size_t>(GameAction::DialogOption4)] = PhysicalKey::Num4;
}

void InputMapper::SetBinding(GameAction action, PhysicalKey key) {
    // Clear any existing binding that maps to the same PhysicalKey
    // (replace conflict policy). This ensures MapKey returns the most
    // recently set action for a given key.
    for (size_t i = 0; i < kGameActionCount; ++i) {
        if (bindings_[i] == key) {
            bindings_[i] = PhysicalKey::Unknown;
        }
    }
    bindings_[static_cast<size_t>(action)] = key;
}

PhysicalKey InputMapper::GetBinding(GameAction action) const {
    return bindings_[static_cast<size_t>(action)];
}

GameAction InputMapper::MapKey(PhysicalKey key) const {
    for (size_t i = 0; i < kGameActionCount; ++i) {
        if (bindings_[i] == key) {
            return static_cast<GameAction>(i);
        }
    }
    return GameAction::Count;  // caller treats Count as "unmapped"
}

void InputMapper::Save(Serializer& s) const {
    s.WriteU16(static_cast<uint16_t>(kGameActionCount));
    for (const auto key : bindings_) {
        s.WriteU16(static_cast<uint16_t>(key));
    }
}

void InputMapper::Load(Deserializer& d) {
    ResetToDefaults();
    const uint16_t count = d.ReadU16();
    for (uint16_t i = 0; i < count && i < kGameActionCount; ++i) {
        bindings_[i] = static_cast<PhysicalKey>(d.ReadU16());
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