#include "writeover/ai/goap.h"

#include <algorithm>
#include <set>

namespace writeover {

namespace {
struct StateKey {
    FactId fact_id;
    bool value;
    bool operator==(const StateKey& o) const {
        return fact_id == o.fact_id && value == o.value;
    }
    bool operator<(const StateKey& o) const {
        if (fact_id != o.fact_id) {
            return fact_id < o.fact_id;
        }
        return value < o.value;
    }
};

std::set<StateKey> CurrentState(const FactStore& facts) {
    std::set<StateKey> state;
    for (const auto& fact : facts.Snapshot()) {
        const bool value = std::holds_alternative<bool>(fact.value)
                               ? std::get<bool>(fact.value)
                               : true;
        state.insert(StateKey{fact.id, value});
    }
    return state;
}

bool Satisfies(const std::set<StateKey>& state, const std::vector<FactPredicate>& preds) {
    for (const auto& p : preds) {
        if (state.count(StateKey{p.fact_id, p.desired_value}) == 0) {
            return false;
        }
    }
    return true;
}

void ApplyEffects(std::set<StateKey>& state, const std::vector<FactPredicate>& effects) {
    for (const auto& e : effects) {
        state.erase(StateKey{e.fact_id, !e.desired_value});
        state.insert(StateKey{e.fact_id, e.desired_value});
    }
}
} // namespace

std::vector<PlanStep> PlanGoap(const std::vector<FactPredicate>& goals,
                               const FactStore& facts,
                               const std::array<GOAPAction, kGoapActionArraySize>& actions,
                               int max_depth) {
    std::vector<PlanStep> plan;
    std::set<StateKey> state = CurrentState(facts);

    const int steps = std::min(max_depth, kGoapMaxDepth);
    for (int depth = 0; depth < steps; ++depth) {
        bool progressed = false;
        // Deterministic order: iterate the fixed action array sorted by cost.
        std::vector<size_t> order;
        order.reserve(actions.size());
        for (size_t i = 0; i < actions.size(); ++i) {
            order.push_back(i);
        }
        std::sort(order.begin(), order.end(), [&actions](size_t a, size_t b) {
            return actions[a].cost < actions[b].cost;
        });

        for (const size_t index : order) {
            const GOAPAction& action = actions[index];
            if (!Satisfies(state, action.preconditions)) {
                continue;
            }
            std::set<StateKey> next_state = state;
            ApplyEffects(next_state, action.effects);
            if (next_state == state) {
                continue;  // no-op action achieves nothing new
            }
            state = next_state;
            PlanStep step;
            step.action = action;
            step.target_pos = Vec3{};
            plan.push_back(step);
            progressed = true;
            break;
        }
        if (!progressed) {
            break;
        }
        if (Satisfies(state, goals)) {
            break;
        }
    }

    if (!Satisfies(state, goals)) {
        return std::vector<PlanStep>();  // no plan found
    }
    return plan;
}

} // namespace writeover