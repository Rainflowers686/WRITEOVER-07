#pragma once
// Deterministic macro-ending resolver. The four public endings are the
// TruthBand x DominanceBand quadrants; the residual loop is a separate hidden
// meta condition and never becomes a fifth ordinary quadrant.

#include "writeover/systemic/systemic.h"

#include <cstdint>

namespace writeover {

enum class MacroEnding : uint8_t {
    Compliance = 0, // low truth, high narrator dominance
    Curator = 1,    // high truth, high narrator dominance
    Escape = 2,     // low truth, low narrator dominance
    Overwrite = 3,  // high truth, low narrator dominance
};

struct EndingResolution {
    MacroEnding macro = MacroEnding::Escape;
    bool hidden_loop = false;
};

inline constexpr float kTruthHighThreshold = 0.60f;
inline constexpr float kNarratorDominanceHighThreshold = 0.50f;
inline constexpr float kLoopSelfKnowledgeThreshold = 0.75f;
inline constexpr float kLoopTimelineInstabilityThreshold = 0.60f;
inline constexpr float kLoopResidualPressureThreshold = 0.50f;
inline constexpr uint32_t kLoopPreviousEvidenceMinimum = 3;

EndingResolution ResolveEnding(const GlobalPlayerState& state,
                               bool selected_npc_residual_callback,
                               bool death_load_rewind_occurred);

} // namespace writeover
