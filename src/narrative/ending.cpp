#include "writeover/narrative/ending.h"

namespace writeover {

EndingResolution ResolveEnding(const GlobalPlayerState& state,
                               bool selected_npc_residual_callback,
                               bool death_load_rewind_occurred) {
    const bool truth_high = state.truth_exposure >= kTruthHighThreshold;
    const bool dominance_high = state.narrator_dominance >=
                                 kNarratorDominanceHighThreshold;
    MacroEnding macro = MacroEnding::Escape;
    if (!truth_high && dominance_high) {
        macro = MacroEnding::Compliance;
    } else if (truth_high && dominance_high) {
        macro = MacroEnding::Curator;
    } else if (truth_high && !dominance_high) {
        macro = MacroEnding::Overwrite;
    }

    const bool hidden_loop =
        state.self_knowledge >= kLoopSelfKnowledgeThreshold &&
        state.timeline_instability >= kLoopTimelineInstabilityThreshold &&
        state.residual_memory_pressure >= kLoopResidualPressureThreshold &&
        state.operator_room_found &&
        state.previous_cycle_evidence_count >= kLoopPreviousEvidenceMinimum &&
        selected_npc_residual_callback && death_load_rewind_occurred;
    return EndingResolution{macro, hidden_loop};
}

} // namespace writeover
