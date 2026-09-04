#include "writeover/core/settings.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace writeover {

Settings Settings::Defaults() {
    Settings s;
    s.preset = QualityPreset::Compatibility;
    s.frame_rate_cap = 0;
    s.fov = 90;
    s.mouse_sensitivity = 50;
    s.gamepad_sensitivity = 50;
    s.aim_assist = false;
    s.master_volume = 70;
    s.narrator_volume = 70;
    s.sfx_volume = 70;
    s.difficulty = 1;
    s.interaction_highlight = true;
    s.tactical_focus = false;
    s.subtitles = true;
    s.reduce_camera_shake = false;
    s.reduce_flicker = false;
    s.high_contrast = false;
    for (auto& table : s.key_bindings) {
        table.fill(PhysicalKey::Unknown);
    }
    auto& g = s.key_bindings[static_cast<size_t>(InputContext::Gameplay)];
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

    auto& dlg = s.key_bindings[static_cast<size_t>(InputContext::Dialogue)];
    dlg[static_cast<size_t>(GameAction::DialogOption1)] = PhysicalKey::Num1;
    dlg[static_cast<size_t>(GameAction::DialogOption2)] = PhysicalKey::Num2;
    dlg[static_cast<size_t>(GameAction::DialogOption3)] = PhysicalKey::Num3;
    dlg[static_cast<size_t>(GameAction::DialogOption4)] = PhysicalKey::Num4;
    dlg[static_cast<size_t>(GameAction::MoveForward)] = PhysicalKey::W;
    dlg[static_cast<size_t>(GameAction::MoveBackward)] = PhysicalKey::S;
    dlg[static_cast<size_t>(GameAction::MoveLeft)] = PhysicalKey::A;
    dlg[static_cast<size_t>(GameAction::MoveRight)] = PhysicalKey::D;
    dlg[static_cast<size_t>(GameAction::Pause)] = PhysicalKey::Escape;

    auto& menu = s.key_bindings[static_cast<size_t>(InputContext::Menu)];
    menu[static_cast<size_t>(GameAction::Pause)] = PhysicalKey::Escape;
    menu[static_cast<size_t>(GameAction::Help)] = PhysicalKey::F1;

    auto& dev = s.key_bindings[static_cast<size_t>(InputContext::Developer)];
    dev[static_cast<size_t>(GameAction::DevPanel)] = PhysicalKey::F3;
    dev[static_cast<size_t>(GameAction::Pause)] = PhysicalKey::Escape;
    return s;
}

void Settings::Save(Serializer& s) const {
    s.WriteU8(static_cast<uint8_t>(preset));
    s.WriteU8(frame_rate_cap);
    s.WriteU8(fov);
    s.WriteU8(mouse_sensitivity);
    s.WriteU8(gamepad_sensitivity);
    s.WriteU8(aim_assist ? 1 : 0);
    s.WriteU8(master_volume);
    s.WriteU8(narrator_volume);
    s.WriteU8(sfx_volume);
    s.WriteU8(difficulty);
    s.WriteU8(interaction_highlight ? 1 : 0);
    s.WriteU8(tactical_focus ? 1 : 0);
    s.WriteU8(subtitles ? 1 : 0);
    s.WriteU8(reduce_camera_shake ? 1 : 0);
    s.WriteU8(reduce_flicker ? 1 : 0);
    s.WriteU8(high_contrast ? 1 : 0);
    s.WriteU16(static_cast<uint16_t>(kInputContextCount));
    for (const auto& table : key_bindings) {
        for (const auto key : table) {
            s.WriteU16(static_cast<uint16_t>(key));
        }
    }
}

void Settings::Load(Deserializer& d) {
    // Reset to defaults FIRST so scalar fields keep the loaded values below.
    *this = Defaults();
    preset = static_cast<QualityPreset>(d.ReadU8());
    frame_rate_cap = d.ReadU8();
    fov = d.ReadU8();
    mouse_sensitivity = d.ReadU8();
    gamepad_sensitivity = d.ReadU8();
    aim_assist = d.ReadU8() != 0;
    master_volume = d.ReadU8();
    narrator_volume = d.ReadU8();
    sfx_volume = d.ReadU8();
    difficulty = d.ReadU8();
    interaction_highlight = d.ReadU8() != 0;
    tactical_focus = d.ReadU8() != 0;
    subtitles = d.ReadU8() != 0;
    reduce_camera_shake = d.ReadU8() != 0;
    reduce_flicker = d.ReadU8() != 0;
    high_contrast = d.ReadU8() != 0;
    const uint16_t context_count = d.ReadU16();
    const uint16_t contexts = static_cast<uint16_t>(
        context_count > kInputContextCount ? kInputContextCount : context_count);
    for (uint16_t c = 0; c < contexts; ++c) {
        for (uint16_t i = 0; i < kGameActionCount; ++i) {
            key_bindings[c][i] = static_cast<PhysicalKey>(d.ReadU16());
        }
    }
}

namespace {
bool ParseBool(const std::string& value, bool& out) {
    if (value == "true" || value == "1") {
        out = true;
        return true;
    }
    if (value == "false" || value == "0") {
        out = false;
        return true;
    }
    return false;
}

bool ParseU8(const std::string& value, uint8_t& out) {
    unsigned long parsed = 0;
    const char* begin = value.c_str();
    char* end = nullptr;
    parsed = std::strtoul(begin, &end, 10);
    if (end == begin || *end != '\0' || parsed > 255) {
        return false;
    }
    out = static_cast<uint8_t>(parsed);
    return true;
}
} // namespace

// --- key=value text persistence (settings.cfg) ---
namespace {

bool ParseU16(const std::string& value, uint16_t& out) {
    unsigned long parsed = 0;
    const char* begin = value.c_str();
    char* end = nullptr;
    parsed = std::strtoul(begin, &end, 10);
    if (end == begin || *end != '\0' || parsed > 0xFFFF) {
        return false;
    }
    out = static_cast<uint16_t>(parsed);
    return true;
}

// PhysicalKey text parser: accepts values 0..kPhysicalKeyCount-1 or
// 0xFFFF (PhysicalKey::Unknown). Other 16-bit values are rejected.
bool ParsePhysicalKey(const std::string& value, uint16_t& out) {
    if (!ParseU16(value, out)) return false;
    if (out == 0xFFFF) return true;  // PhysicalKey::Unknown
    if (out >= kPhysicalKeyCount) return false;
    return true;
}

struct KeyValue {
    std::string key;
    std::string value;
};

std::vector<KeyValue> SplitKeyValues(const std::string& text) {
    std::vector<KeyValue> out;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        KeyValue kv;
        kv.key = line.substr(0, eq);
        kv.value = line.substr(eq + 1);
        out.push_back(std::move(kv));
    }
    return out;
}

void ApplyKeyValue(Settings& s, const KeyValue& kv) {
    uint8_t tmp8 = 0;
    bool tmpb = false;
    // Never trust unknown/duplicate keys: safe defaults stay for malformed.
    if (kv.key == "preset" && ParseU8(kv.value, tmp8) && tmp8 <= 3) {
        s.preset = static_cast<QualityPreset>(tmp8);
    } else if (kv.key == "frameratecap" && ParseU8(kv.value, tmp8)) {
        s.frame_rate_cap = tmp8;
    } else if (kv.key == "fov" && ParseU8(kv.value, tmp8)) {
        if (tmp8 >= 60 && tmp8 <= 120) {
            s.fov = tmp8;
        }
    } else if (kv.key == "mousesensitivity" && ParseU8(kv.value, tmp8) && tmp8 <= 100) {
        s.mouse_sensitivity = tmp8;
    } else if (kv.key == "gamepadsensitivity" && ParseU8(kv.value, tmp8) && tmp8 <= 100) {
        s.gamepad_sensitivity = tmp8;
    } else if (kv.key == "aimassist" && ParseBool(kv.value, tmpb)) {
        s.aim_assist = tmpb;
    } else if (kv.key == "mastervolume" && ParseU8(kv.value, tmp8) && tmp8 <= 100) {
        s.master_volume = tmp8;
    } else if (kv.key == "narratorvolume" && ParseU8(kv.value, tmp8) && tmp8 <= 100) {
        s.narrator_volume = tmp8;
    } else if (kv.key == "sfxvolume" && ParseU8(kv.value, tmp8) && tmp8 <= 100) {
        s.sfx_volume = tmp8;
    } else if (kv.key == "difficulty" && ParseU8(kv.value, tmp8) && tmp8 <= 2) {
        s.difficulty = tmp8;
    } else if (kv.key == "interactionhighlight" && ParseBool(kv.value, tmpb)) {
        s.interaction_highlight = tmpb;
    } else if (kv.key == "tacticalfocus" && ParseBool(kv.value, tmpb)) {
        s.tactical_focus = tmpb;
    } else if (kv.key == "subtitles" && ParseBool(kv.value, tmpb)) {
        s.subtitles = tmpb;
    } else if (kv.key == "reducecamerashake" && ParseBool(kv.value, tmpb)) {
        s.reduce_camera_shake = tmpb;
    } else if (kv.key == "reduceflicker" && ParseBool(kv.value, tmpb)) {
        s.reduce_flicker = tmpb;
    } else if (kv.key == "highcontrast" && ParseBool(kv.value, tmpb)) {
        s.high_contrast = tmpb;
    } else if (kv.key.rfind("bind.", 0) == 0) {
        // New format: bind.<contextIndex>.<actionIndex>=<PhysicalKeyValue>
        const std::string spec = kv.key.substr(5);
        const size_t dot = spec.find('.');
        uint8_t ctx_index = 0;
        uint8_t action_index = 0;
        uint16_t key_value = 0;
        if (dot == std::string::npos) {
            // Legacy format: bind.<actionIndex>=<PhysicalKeyValue>
            // Interpreted as Gameplay context (backward compatible).
            if (ParseU8(spec, action_index) &&
                static_cast<size_t>(action_index) < kGameActionCount &&
                ParsePhysicalKey(kv.value, key_value)) {
                s.key_bindings[static_cast<size_t>(InputContext::Gameplay)]
                              [action_index] = static_cast<PhysicalKey>(key_value);
            }
        } else if (ParseU8(spec.substr(0, dot), ctx_index) &&
                   static_cast<size_t>(ctx_index) < kInputContextCount &&
                   ParseU8(spec.substr(dot + 1), action_index) &&
                   static_cast<size_t>(action_index) < kGameActionCount &&
                   ParsePhysicalKey(kv.value, key_value)) {
            s.key_bindings[ctx_index][action_index] =
                static_cast<PhysicalKey>(key_value);
        }
    }
}

std::string ComposeKeyValues(const Settings& s) {
    std::ostringstream out;
    out << "# WRITEOVER-07 settings\n";
    out << "preset=" << static_cast<uint32_t>(s.preset) << "\n";
    out << "frameratecap=" << static_cast<uint32_t>(s.frame_rate_cap) << "\n";
    out << "fov=" << static_cast<uint32_t>(s.fov) << "\n";
    out << "mousesensitivity=" << static_cast<uint32_t>(s.mouse_sensitivity) << "\n";
    out << "gamepadsensitivity=" << static_cast<uint32_t>(s.gamepad_sensitivity) << "\n";
    out << "aimassist=" << (s.aim_assist ? "true" : "false") << "\n";
    out << "mastervolume=" << static_cast<uint32_t>(s.master_volume) << "\n";
    out << "narratorvolume=" << static_cast<uint32_t>(s.narrator_volume) << "\n";
    out << "sfxvolume=" << static_cast<uint32_t>(s.sfx_volume) << "\n";
    out << "difficulty=" << static_cast<uint32_t>(s.difficulty) << "\n";
    out << "interactionhighlight=" << (s.interaction_highlight ? "true" : "false") << "\n";
    out << "tacticalfocus=" << (s.tactical_focus ? "true" : "false") << "\n";
    out << "subtitles=" << (s.subtitles ? "true" : "false") << "\n";
    out << "reducecamerashake=" << (s.reduce_camera_shake ? "true" : "false") << "\n";
    out << "reduceflicker=" << (s.reduce_flicker ? "true" : "false") << "\n";
    out << "highcontrast=" << (s.high_contrast ? "true" : "false") << "\n";
    for (size_t c = 0; c < kInputContextCount; ++c) {
        for (size_t i = 0; i < kGameActionCount; ++i) {
            out << "bind." << c << "." << i << "="
                << static_cast<uint32_t>(s.key_bindings[c][i]) << "\n";
        }
    }
    return out.str();
}
} // namespace

Result<Settings> SettingsRegistry::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return Result<Settings>::Err(100, "cannot open settings file");
    }
    std::string text((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    Settings loaded = Settings::Defaults();
    for (const auto& kv : SplitKeyValues(text)) {
        ApplyKeyValue(loaded, kv);
    }
    values_ = loaded;
    return Result<Settings>::Ok(loaded);
}

Result<void> SettingsRegistry::Save(const std::string& path, const Settings& s) {
    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        return Result<void>::Err(101, "cannot write settings file");
    }
    file << ComposeKeyValues(s);
    if (!file) {
        return Result<void>::Err(102, "settings write failed");
    }
    values_ = s;
    return Result<void>::Ok();
}

} // namespace writeover
