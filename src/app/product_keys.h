#pragma once

#include "writeover/player/input.h"
#include <array>
#include <memory>

namespace writeover {

// Presentation observer of the existing backend stream. It does not map or
// consume an additional stream, and gameplay still uses InputRuntime/Mapper.
struct ProductKeys {
    std::array<bool, kPhysicalKeyCount> down{};
    std::array<bool, kPhysicalKeyCount> pressed{};
    void BeginTick() { pressed.fill(false); }
    void Clear() { down.fill(false); pressed.fill(false); }
    void Observe(const InputEvent& event) {
        const size_t key = static_cast<size_t>(event.key);
        if (key >= down.size()) return;
        if (event.pressed && !down[key]) pressed[key] = true;
        down[key] = event.pressed;
    }
    bool Pressed(PhysicalKey key) const {
        const size_t index = static_cast<size_t>(key);
        return index < pressed.size() && pressed[index];
    }
    PhysicalKey FirstPressed() const {
        for (size_t i = 0; i < pressed.size(); ++i)
            if (pressed[i]) return static_cast<PhysicalKey>(i);
        return PhysicalKey::Unknown;
    }
};

class ProductKeyObserver final : public IInputBackend {
public:
    ProductKeyObserver(std::unique_ptr<IInputBackend> backend, ProductKeys& keys)
        : backend_(std::move(backend)), keys_(keys) {}
    bool Init() override { return backend_->Init(); }
    void Shutdown() override { backend_->Shutdown(); }
    bool Poll(InputEvent& event) override {
        if (!backend_->Poll(event)) return false;
        keys_.Observe(event);
        return true;
    }
    bool HasFocus() const override { return backend_->HasFocus(); }
    const char* Name() const override { return backend_->Name(); }
    bool ConsumeMouseDelta(Vec2& delta) override { return backend_->ConsumeMouseDelta(delta); }
    void RebasePointer() override { backend_->RebasePointer(); }
    bool NeedsBackgroundDrain() const override { return backend_->NeedsBackgroundDrain(); }
private:
    std::unique_ptr<IInputBackend> backend_;
    ProductKeys& keys_;
};

// Fixed overlay navigation is independent of user gameplay bindings. These
// keys remain available even if a legacy settings file left actions unbound.
inline InputState ProductNavigation(const InputState& input, const ProductKeys& keys,
                                    const InputMapper* mapper = nullptr) {
    InputState result = input;
    const std::pair<PhysicalKey, GameAction> shortcuts[] = {
        {PhysicalKey::Up, GameAction::MoveForward}, {PhysicalKey::Down, GameAction::MoveBackward},
        {PhysicalKey::Left, GameAction::MoveLeft}, {PhysicalKey::Right, GameAction::MoveRight},
        {PhysicalKey::Escape, GameAction::Pause}, {PhysicalKey::F, GameAction::Interact}};
    for (const auto& entry : shortcuts) {
        if (!keys.Pressed(entry.first) || !mapper) continue;
        const auto mapped = mapper->MapKey(entry.first);
        if (mapped != GameAction::Count) result.action_pressed[static_cast<size_t>(mapped)] = false;
    }
    for (const auto& entry : shortcuts)
        if (keys.Pressed(entry.first)) result.action_pressed[static_cast<size_t>(entry.second)] = true;
    return result;
}

} // namespace writeover
