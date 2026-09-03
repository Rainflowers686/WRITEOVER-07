#pragma once
// Settings registry. Persisted as UTF-8 key=value text. Key bindings are a
// serializable binding table (std::array of PhysicalKey), NOT a pointer
// (M-013 closure). Settings do not change simulation determinism for a fixed
// settings snapshot.

#include "writeover/common/input_types.h"
#include "writeover/common/result.h"
#include "writeover/common/serialize.h"

#include <array>
#include <cstdint>
#include <string>

namespace writeover {

enum class QualityPreset : uint8_t {
    Ultra120 = 0,
    HighRefresh = 1,
    Presentation60 = 2,
    Compatibility = 3,
};

struct Settings {
    // --- Main page ---
    QualityPreset preset = QualityPreset::Compatibility;
    uint8_t frame_rate_cap = 0;         // 0=uncapped
    uint8_t fov = 90;                   // 60..120
    uint8_t mouse_sensitivity = 50;     // 0..100
    uint8_t gamepad_sensitivity = 50;   // 0..100
    std::array<PhysicalKey, kGameActionCount> key_bindings;
    bool aim_assist = false;
    uint8_t master_volume = 70;         // 0..100
    uint8_t narrator_volume = 70;       // 0..100
    uint8_t sfx_volume = 70;            // 0..100
    uint8_t difficulty = 1;             // 0=easy 1=normal 2=hard
    bool interaction_highlight = true;
    bool tactical_focus = false;
    bool subtitles = true;

    // --- Accessibility ---
    bool reduce_camera_shake = false;
    bool reduce_flicker = false;
    bool high_contrast = false;

    // Defaults for all fields (including the binding table).
    static Settings Defaults();

    void Save(Serializer& s) const;
    void Load(Deserializer& d);
};

class SettingsRegistry {
public:
    Result<Settings> Load(const std::string& path);              // corrupted -> Err
    Result<void> Save(const std::string& path, const Settings& s);
    void ResetToDefaults() { values_ = Settings::Defaults(); }
    Settings& Values() { return values_; }
    const Settings& Values() const { return values_; }

private:
    Settings values_ = Settings::Defaults();
};

} // namespace writeover