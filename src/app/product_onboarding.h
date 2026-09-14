#pragma once

#include "src/app/player_product.h"

namespace writeover {

struct OnboardingContext {
    uint64_t frame = 0;
    bool in_b1 = false;
    bool moved = false;
    bool interacted = false;
    bool case_file_used = false;
    bool needs_reload = false;
    bool has_records = false;
    bool has_checkpoint = false;
};

// Process-local presentation only. Loading an existing run suppresses hints;
// rebuilding the composition for New Game naturally gives it fresh hints.
class ProductOnboarding {
public:
    void Continued() { enabled_ = false; active_ = Hint::None; }

    std::string Line(const OnboardingContext& state, const Settings& settings) {
        if (!enabled_) return {};
        if (state.moved) seen_[static_cast<size_t>(Hint::Move)] = true;
        if (state.interacted) seen_[static_cast<size_t>(Hint::Focus)] = true;
        if (state.case_file_used) {
            seen_[static_cast<size_t>(Hint::CaseFile)] = true;
            seen_[static_cast<size_t>(Hint::Records)] = true;
        }
        if (state.needs_reload && active_ != Hint::Reload &&
            !seen_[static_cast<size_t>(Hint::Reload)]) {
            active_ = Hint::None;
            next_allowed_ = state.frame;
        }
        const auto relevant = [&](Hint hint) {
            switch (hint) {
            case Hint::Move: return state.in_b1 && !state.moved;
            case Hint::Focus: return state.in_b1 && state.moved && !state.interacted;
            case Hint::CaseFile: return state.in_b1 && state.interacted && !state.case_file_used;
            case Hint::Reload: return state.needs_reload;
            case Hint::Records: return state.has_records && !state.case_file_used;
            case Hint::Checkpoint: return state.has_checkpoint;
            default: return false;
            }
        };
        if (active_ != Hint::None && (!relevant(active_) || state.frame >= until_)) {
            active_ = Hint::None;
            next_allowed_ = state.frame + 60;
        }
        if (active_ == Hint::None) {
            if (state.frame < next_allowed_) return {};
            for (const Hint candidate : {Hint::Reload, Hint::Checkpoint, Hint::Records,
                                         Hint::Move, Hint::Focus, Hint::CaseFile}) {
                if (seen_[static_cast<size_t>(candidate)] || !relevant(candidate)) continue;
                active_ = candidate;
                seen_[static_cast<size_t>(candidate)] = true;
                until_ = state.frame + 480;
                break;
            }
        }
        switch (active_) {
        case Hint::Move:
            return ProductBinding(settings, GameAction::MoveForward) + "/" +
                ProductBinding(settings, GameAction::MoveLeft) + "/" +
                ProductBinding(settings, GameAction::MoveBackward) + "/" +
                ProductBinding(settings, GameAction::MoveRight) + ": move. Mouse: look.";
        case Hint::Focus:
            return "Look directly at an object, then press " + ProductBinding(settings, GameAction::Interact) + ".";
        case Hint::CaseFile:
            return ProductBinding(settings, GameAction::Help) + ": review your objective and leads.";
        case Hint::Reload:
            return "Empty magazine. Press " + ProductBinding(settings, GameAction::Reload) + " to reload.";
        case Hint::Records:
            return ProductBinding(settings, GameAction::Help) + ": read the records you acquired.";
        case Hint::Checkpoint:
            return ProductBinding(settings, GameAction::Pause) + ": recovery points are in Pause.";
        default: return {};
        }
    }

private:
    enum class Hint : size_t { Move, Focus, CaseFile, Reload, Records, Checkpoint, None };
    std::array<bool, static_cast<size_t>(Hint::None)> seen_{};
    Hint active_ = Hint::None;
    uint64_t until_ = 0;
    uint64_t next_allowed_ = 0;
    bool enabled_ = true;
};

} // namespace writeover
