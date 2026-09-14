#pragma once

#include "writeover/core/save.h"

#include <array>
#include <filesystem>
#include <string>

namespace writeover {

enum class ProductSaveRole : uint8_t { Manual, Checkpoint, PreFinal, Completion };
inline const char* ProductSaveName(ProductSaveRole role) {
    switch (role) {
    case ProductSaveRole::Checkpoint: return "pvs_checkpoint";
    case ProductSaveRole::PreFinal: return "pvs_pre_final";
    case ProductSaveRole::Completion: return "pvs_completion";
    default: return "pvs_manual";
    }
}
// This is menu availability, not semantic load authority. Actual Continue uses
// the existing full staged validation/rollback path and can report a safe error.
inline bool ProductSaveEnvelopeValid(const std::filesystem::path& base) {
    SaveManager save;
    const auto loaded = save.LoadWorld(base.string());
    if (loaded.IsError()) return false;
    std::array<bool, 8> present{};
    for (const auto& section : loaded.Value()) {
        const size_t index = static_cast<size_t>(section.id);
        if (index >= present.size() || present[index] || section.data.empty()) return false;
        present[index] = true;
    }
    return present[0] && present[1] && present[2] && present[3] && present[4] && present[5] && present[7];
}
inline std::string ProductResumeName(const std::filesystem::path& directory) {
    std::error_code error;
    // A corrupt current save must not silently resume an unrelated older run.
    if (std::filesystem::exists(directory / "pvs_resume.wo07", error)) return "pvs_resume";
    return "pvs_manual"; // backwards-compatible entry for earlier candidates
}

} // namespace writeover
