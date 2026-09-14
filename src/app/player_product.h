#pragma once

#include "src/app/perception_feed.h"
#include "src/app/product_keys.h"
#include "writeover/core/settings.h"
#include "writeover/player/input.h"

#include <array>
#include <functional>

namespace writeover {

// The backend retains key-down state between events; filter gameplay only.
class ProductInputLease {
public:
    ProductInputLease(InputState& input, std::array<bool, kGameActionCount>& held, bool reentry)
        : input_(input), raw_down_(input.action_down) {
        for (size_t i = 0; i < held.size(); ++i) {
            if (!raw_down_[i] || input.action_released[i]) held[i] = false;
            if (held[i]) { input.action_down[i] = false; input.action_pressed[i] = false; }
        }
        if (reentry) input.mouse_delta = Vec2{};
    }
    ~ProductInputLease() { input_.action_down = raw_down_; }
    ProductInputLease(const ProductInputLease&) = delete;
    ProductInputLease& operator=(const ProductInputLease&) = delete;
private:
    InputState& input_;
    std::array<bool, kGameActionCount> raw_down_;
};

inline std::string ProductKeyName(PhysicalKey key) {
    static constexpr const char* names[] = {
        "W","A","S","D","Q","E","R","F","Z","X","C","V","B","N","M",
        "SPACE","SHIFT","CTRL","TAB","ESC","UP","DOWN","LEFT","RIGHT",
        "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
        "0","1","2","3","4","5","6","7","8","9",
        "MOUSE1","MOUSE2","MOUSE3","MOUSE4","MOUSE5",
        "PAD UP","PAD DOWN","PAD LEFT","PAD RIGHT","PAD A","PAD B","PAD X","PAD Y",
        "PAD LB","PAD RB","PAD LT","PAD RT","PAD START","PAD BACK","PAD LS","PAD RS"
    };
    const size_t index = static_cast<size_t>(key);
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "UNBOUND";
}

inline std::string ProductBinding(const Settings& settings, GameAction action) {
    return ProductKeyName(settings.key_bindings[0][static_cast<size_t>(action)]);
}

inline std::string ProductControlText(std::string text, const Settings& settings) {
    const auto replace = [&](const std::string& from, const std::string& to) {
        size_t start = 0;
        while ((start = text.find(from, start)) != std::string::npos) {
            text.replace(start, from.size(), to);
            start += to.size();
        }
    };
    replace("[F]", "[" + ProductBinding(settings, GameAction::Interact) + "]");
    replace("WASD MOVE", ProductBinding(settings, GameAction::MoveForward) + "/" +
        ProductBinding(settings, GameAction::MoveLeft) + "/" +
        ProductBinding(settings, GameAction::MoveBackward) + "/" +
        ProductBinding(settings, GameAction::MoveRight) + " MOVE");
    replace("F INTERACT", ProductBinding(settings, GameAction::Interact) + " INTERACT");
    replace("LMB FIRE", ProductBinding(settings, GameAction::Fire) + " FIRE");
    replace("W/S", ProductBinding(settings, GameAction::MoveForward) + "/" +
                    ProductBinding(settings, GameAction::MoveBackward));
    replace("F CONFIRM", ProductBinding(settings, GameAction::Interact) + " CONFIRM");
    replace("F TRAVEL", ProductBinding(settings, GameAction::Interact) + " TRAVEL");
    replace("F records", ProductBinding(settings, GameAction::Interact) + " records");
    replace("F commits", ProductBinding(settings, GameAction::Interact) + " commits");
    replace("ESC / F CLOSE", ProductBinding(settings, GameAction::Pause) + " / " +
        ProductBinding(settings, GameAction::Interact) + " CLOSE");
    replace("ESC CLOSE", ProductBinding(settings, GameAction::Pause) + " CLOSE");
    replace("F1 CASE FILE", ProductBinding(settings, GameAction::Help) + " CASE FILE");
    replace("F9 LOAD", ProductBinding(settings, GameAction::LoadGame) + " LOAD");
    replace("A/D SCROLL", ProductBinding(settings, GameAction::MoveLeft) + "/" +
        ProductBinding(settings, GameAction::MoveRight) + " SCROLL");
    return text;
}

inline const std::vector<std::pair<GameAction, const char*>>& ProductBindableActions() {
    static const std::vector<std::pair<GameAction, const char*>> actions{
        {GameAction::MoveForward,"Forward"},{GameAction::MoveBackward,"Backward"},
        {GameAction::MoveLeft,"Left"},{GameAction::MoveRight,"Right"},
        {GameAction::Sprint,"Sprint"},{GameAction::Jump,"Jump"},{GameAction::Crouch,"Crouch"},
        {GameAction::Prone,"Prone"},{GameAction::LeanLeft,"Lean left"},{GameAction::LeanRight,"Lean right"},
        {GameAction::Interact,"Interact / confirm"},{GameAction::AimDownSights,"Examine"},
        {GameAction::Fire,"Fire"},{GameAction::Reload,"Reload"},{GameAction::Melee,"Stunner shortcut"},
        {GameAction::WeaponSlot1,"Pistol"},{GameAction::WeaponSlot2,"SMG"},{GameAction::WeaponSlot3,"Stunner"},
        {GameAction::SaveGame,"Manual save"},{GameAction::LoadGame,"Load last save"},
        {GameAction::Help,"Case File"},{GameAction::Pause,"Pause / history / settings"}};
    return actions;
}

enum class ProductPage : uint8_t { None, Boot, Pause, Controls, Settings, History, Dialogue, Inspect, Ending, NewGame, Language, Rebind };
enum class ProductCommand : uint8_t { None, Resume, Continue, NewGame, Save, Load, Checkpoint, PreFinal, CaseFile, Quit, SettingsChanged };

// One bounded application menu, not a widget framework. The composition root
// owns commands; this class owns only selection, pages, and perceived history.
class PlayerProductRuntime {
public:
    PerceptionFeed feed;
    ProductPage page = ProductPage::None;
    size_t selection = 0;
    size_t scroll = 0;
    bool continue_available = false;
    bool checkpoint_available = false;
    bool pre_final_available = false;
    bool boot_context = true;
    bool dead = false;
    bool completed = false;
    std::string location;
    std::string notice;
    std::vector<std::string> inspect_rows;
    std::vector<std::string> ending_rows;
    uint32_t visited_pages = 0; // process-local UI evidence, never gameplay state
    size_t preference_changes = 0;
    bool capturing_binding = false;
    PhysicalKey pending_binding = PhysicalKey::Unknown;

    void CancelBindingCapture() { capturing_binding = false; pending_binding = PhysicalKey::Unknown; }

    bool Active() const { return page != ProductPage::None; }
    void Open(ProductPage next) {
        CancelBindingCapture();
        notice.clear();
        page = next; selection = next == ProductPage::Boot && !continue_available ? 1 : 0; scroll = 0;
        visited_pages |= 1u << static_cast<unsigned>(next);
    }
    bool Visited(ProductPage item) const { return (visited_pages & (1u << static_cast<unsigned>(item))) != 0; }
    void Close() { page = ProductPage::None; selection = 0; scroll = 0; }
    std::vector<std::string> Options(const Settings& s) const {
        if (page == ProductPage::Rebind) {
            std::vector<std::string> rows;
            for (const auto& entry : ProductBindableActions()) rows.push_back(
                std::string(entry.second) + " / " + ProductBinding(s, entry.first));
            rows.push_back("RESTORE DEFAULT BINDINGS");
            return rows;
        }
        if (page == ProductPage::Language) return {"简体中文", "English"};
        if (page == ProductPage::Boot) return {continue_available ? "CONTINUE" : "CONTINUE / unavailable",
            "NEW GAME", "CONTROLS / HELP", "ACCESSIBILITY / SETTINGS", "QUIT"};
        if (page == ProductPage::Pause) return {dead ? "RESUME / dead" : "RESUME", dead ? "SAVE / unavailable" : "MANUAL SAVE",
            "LOAD LAST SAVE", checkpoint_available ? "RESTART CHECKPOINT" : "CHECKPOINT / unavailable",
            pre_final_available ? "REPLAY FINAL CHOICE" : "PRE-FINAL / unavailable", "CASE FILE",
            "RECENT EVENTS", "DIALOGUE HISTORY", "CONTROLS / HELP", "ACCESSIBILITY / SETTINGS", "NEW GAME",
            completed ? "ENDING SUMMARY" : "ENDING / not reached", "QUIT"};
        if (page == ProductPage::NewGame) return {"CANCEL", "START FRESH / existing saves retained"};
        if (page == ProductPage::Settings) {
            const char* verbosity[] = {"OFF", "IMPORTANT", "DETAILED"};
            const char* duration[] = {"SHORT", "NORMAL", "LONG"};
            return {std::string("SENSORY / ") + verbosity[std::min<uint8_t>(s.sensory_verbosity, 2)],
                std::string("TEXT DURATION / ") + duration[std::min<uint8_t>(s.text_duration, 2)],
                std::string("SUBTITLES / ") + (s.subtitles ? "ON" : "OFF"),
                std::string("HIGH CONTRAST / ") + (s.high_contrast ? "ON" : "OFF"),
                std::string("REDUCE SHAKE / ") + (s.reduce_camera_shake ? "ON" : "OFF"),
                std::string("REDUCE FLICKER / ") + (s.reduce_flicker ? "ON" : "OFF"),
                "MOUSE SENSITIVITY / " + std::to_string(s.mouse_sensitivity),
                "MASTER VOLUME / " + std::to_string(s.master_volume),
                "FRAME LIMIT / " + (s.frame_rate_cap == 0 ? std::string("AUTO (UP TO 120)") :
                    std::to_string(s.frame_rate_cap) + " FPS"),
                s.language == "zh-CN" ? "LANGUAGE / 简体中文" : "LANGUAGE / English",
                "REBIND KEYS"};
        }
        return {};
    }
    ProductCommand Handle(const InputState& input, Settings& settings, const ProductKeys* keys = nullptr) {
        if (!Active() || !input.has_focus) return ProductCommand::None;
        const auto pressed = [&](GameAction a) { return input.action_pressed[static_cast<size_t>(a)]; };
        if (page == ProductPage::Rebind && (capturing_binding || pending_binding != PhysicalKey::Unknown)) {
            if (keys && keys->Pressed(PhysicalKey::Escape)) { CancelBindingCapture(); notice = "Binding change cancelled."; return ProductCommand::None; }
            if (capturing_binding) {
                const PhysicalKey key = keys ? keys->FirstPressed() : PhysicalKey::Unknown;
                if (key == PhysicalKey::Unknown) return ProductCommand::None;
                if (key == PhysicalKey::Up || key == PhysicalKey::Down || key == PhysicalKey::Left || key == PhysicalKey::Right) {
                    notice = "Arrow keys are reserved for safe menu navigation."; return ProductCommand::None;
                }
                const auto action = ProductBindableActions()[selection].first;
                for (size_t i = 0; i < kGameActionCount; ++i) {
                    if (i != static_cast<size_t>(action) && settings.key_bindings[0][i] == key) {
                        notice = "Key already used. Choose another key or cancel."; return ProductCommand::None;
                    }
                }
                pending_binding = key; capturing_binding = false;
                notice = "PENDING / " + ProductKeyName(key) + " / F CONFIRM / ESC CANCEL";
            } else if (keys && keys->Pressed(PhysicalKey::F)) {
                settings.key_bindings[0][static_cast<size_t>(ProductBindableActions()[selection].first)] = pending_binding;
                CancelBindingCapture(); ++preference_changes;
                return ProductCommand::SettingsChanged;
            }
            return ProductCommand::None;
        }
        if (pressed(GameAction::Pause)) {
            if (page == ProductPage::Boot || page == ProductPage::Language) return ProductCommand::None;
            if (page == ProductPage::Pause) {
                if (dead) return ProductCommand::None;
                Close(); return ProductCommand::Resume;
            }
            Open(boot_context ? ProductPage::Boot : ProductPage::Pause);
            return ProductCommand::None;
        }
        const auto options = Options(settings);
        if (pressed(GameAction::MoveLeft)) scroll = scroll > 0 ? scroll - 1 : 0;
        if (pressed(GameAction::MoveRight)) scroll = std::min<size_t>(scroll + 1, 1024);
        if (options.empty()) {
            if (pressed(GameAction::MoveForward)) scroll = scroll > 0 ? scroll - 1 : 0;
            if (pressed(GameAction::MoveBackward)) scroll = std::min<size_t>(scroll + 1, 1024);
            return ProductCommand::None;
        }
        if (pressed(GameAction::MoveForward) || pressed(GameAction::MoveBackward)) scroll = 0;
        if (pressed(GameAction::MoveForward)) selection = selection == 0 ? options.size() - 1 : selection - 1;
        if (pressed(GameAction::MoveBackward)) selection = (selection + 1) % options.size();
        if (!pressed(GameAction::Interact)) return ProductCommand::None;
        if (page == ProductPage::Rebind) {
            if (selection == ProductBindableActions().size()) {
                settings.key_bindings = Settings::Defaults().key_bindings;
                ++preference_changes;
                return ProductCommand::SettingsChanged;
            }
            capturing_binding = true; pending_binding = PhysicalKey::Unknown;
            notice = "Press a new key. ESC cancels; conflicts are not overwritten.";
        } else if (page == ProductPage::Language) {
            settings.language = selection == 0 ? "zh-CN" : "en";
            Open(ProductPage::Boot);
            ++preference_changes;
            return ProductCommand::SettingsChanged;
        } else if (page == ProductPage::Boot) {
            switch (selection) {
            case 0:
                if (continue_available) return ProductCommand::Continue;
                notice = "No usable recent save. Choose New Game to begin.";
                break;
            case 1: Open(ProductPage::NewGame); break;
            case 2: Open(ProductPage::Controls); break;
            case 3: Open(ProductPage::Settings); break;
            default: return ProductCommand::Quit;
            }
        } else if (page == ProductPage::Pause) {
            switch (selection) {
            case 0: if (!dead) { Close(); return ProductCommand::Resume; } break;
            case 1: return dead ? ProductCommand::None : ProductCommand::Save;
            case 2: return ProductCommand::Load;
            case 3:
                if (checkpoint_available) return ProductCommand::Checkpoint;
                notice = "No checkpoint yet. Use Load Last Save or New Game.";
                break;
            case 4:
                if (pre_final_available) return ProductCommand::PreFinal;
                notice = "Final-choice recovery becomes available near the campaign ending.";
                break;
            case 5: Close(); return ProductCommand::CaseFile;
            case 6: Open(ProductPage::History); break;
            case 7: Open(ProductPage::Dialogue); break;
            case 8: Open(ProductPage::Controls); break;
            case 9: Open(ProductPage::Settings); break;
            case 10: Open(ProductPage::NewGame); break;
            case 11:
                if (completed) Open(ProductPage::Ending);
                else notice = "No ending reached in this playthrough.";
                break;
            default: return ProductCommand::Quit;
            }
        } else if (page == ProductPage::NewGame) {
            if (selection == 1) return ProductCommand::NewGame;
            Open(boot_context ? ProductPage::Boot : ProductPage::Pause);
        } else if (page == ProductPage::Settings) {
            switch (selection) {
            case 0: settings.sensory_verbosity = static_cast<uint8_t>((settings.sensory_verbosity + 1) % 3); break;
            case 1: settings.text_duration = static_cast<uint8_t>((settings.text_duration + 1) % 3); break;
            case 2: settings.subtitles = !settings.subtitles; break;
            case 3: settings.high_contrast = !settings.high_contrast; break;
            case 4: settings.reduce_camera_shake = !settings.reduce_camera_shake; break;
            case 5: settings.reduce_flicker = !settings.reduce_flicker; break;
            case 6: settings.mouse_sensitivity = static_cast<uint8_t>((settings.mouse_sensitivity + 10) % 110); break;
            case 7: settings.master_volume = static_cast<uint8_t>((settings.master_volume + 10) % 110); break;
            case 8:
                settings.frame_rate_cap = settings.frame_rate_cap == 0 ? 30 :
                    settings.frame_rate_cap == 30 ? 60 : settings.frame_rate_cap == 60 ? 120 : 0;
                break;
            case 9: settings.language = settings.language == "zh-CN" ? "en" : "zh-CN"; break;
            case 10: Open(ProductPage::Rebind); return ProductCommand::None;
            default: break;
            }
            ++preference_changes;
            return ProductCommand::SettingsChanged;
        }
        return ProductCommand::None;
    }
    std::vector<std::string> Rows(const Settings& s,
        const std::function<std::string(const std::string&)>& translate = {}) const {
        if (!Active()) return {};
        std::vector<std::string> rows;
        const auto options = Options(s);
        if (!options.empty()) {
            rows = {page == ProductPage::Rebind ? "REBIND KEYS / SELECT AN ACTION" :
                page == ProductPage::Language ? "选择语言 / CHOOSE LANGUAGE" :
                page == ProductPage::Boot ? "WRITEOVER-07 / THE RECORD IS NOT THE EVENT" :
                page == ProductPage::Settings ? "ACCESSIBILITY / CHANGES SAVE AUTOMATICALLY" :
                page == ProductPage::NewGame ? "NEW GAME / RESET ALL LIVE PROGRESSION?" :
                dead ? "YOU DIED / CHOOSE A RECOVERY POINT" : "PAUSED / " + location};
            // Keep choices in a stable order; the panel follows the selection
            // when a small terminal cannot show the whole list.
            for (size_t i = 0; i < options.size(); ++i)
                rows.push_back(std::string(i == selection % options.size() ? "> " : "  ") + options[i]);
            if (page == ProductPage::Boot) {
                rows.push_back("");
                rows.push_back("FIRST VISIT? Controls shows your current bindings.");
                rows.push_back("Explore, talk, and examine before committing to force.");
            }
        } else if (page == ProductPage::History || page == ProductPage::Dialogue) {
            rows = feed.History(page == ProductPage::Dialogue);
        } else if (page == ProductPage::Inspect) {
            rows = inspect_rows;
        } else if (page == ProductPage::Ending) {
            rows = ending_rows;
        } else {
            rows = {"CONTROLS / CURRENT BINDINGS", "Mouse: look. Keyboard movement is camera-relative."};
            const std::pair<GameAction, const char*> labels[] = {
                {GameAction::MoveForward,"Forward"},{GameAction::MoveBackward,"Backward"},
                {GameAction::MoveLeft,"Left"},{GameAction::MoveRight,"Right"},
                {GameAction::Sprint,"Sprint"},{GameAction::Jump,"Jump"},{GameAction::Crouch,"Crouch"},
                {GameAction::Prone,"Prone"},{GameAction::LeanLeft,"Lean left"},{GameAction::LeanRight,"Lean right"},
                {GameAction::Interact,"Interact / confirm"},{GameAction::AimDownSights,"Examine (ADS is disabled)"},
                {GameAction::Fire,"Fire"},{GameAction::Reload,"Reload"},{GameAction::Melee,"Stunner shortcut"},
                {GameAction::WeaponSlot1,"Pistol"},{GameAction::WeaponSlot2,"SMG"},{GameAction::WeaponSlot3,"Stunner"},
                {GameAction::SaveGame,"Manual save"},{GameAction::LoadGame,"Load last save"},
                {GameAction::Help,"Case File"},{GameAction::Pause,"Pause / history / settings"}};
            for (const auto& item : labels) rows.push_back(ProductBinding(s, item.first) + " / " + item.second);
            rows.push_back("Bindings use the existing settings.cfg table; no hidden fixed gameplay keys.");
        }
        if (!notice.empty()) rows.insert(rows.begin() + 1, notice);
        if (page == ProductPage::Rebind) {
            rows.push_back("ARROWS SELECT / F CONFIRM / ESC BACK");
            return rows;
        }
        if (page == ProductPage::Language) {
            rows.push_back(ProductBinding(s, GameAction::MoveForward) + "/" +
                ProductBinding(s, GameAction::MoveBackward) + " 选择 / SELECT    " +
                ProductBinding(s, GameAction::Interact) + " 确认 / CONFIRM");
            return rows;
        }
        const auto label = [&](const std::string& value) { return translate ? translate(value) : value; };
        rows.push_back(ProductBinding(s, GameAction::MoveForward) + "/" + ProductBinding(s, GameAction::MoveBackward) +
            (options.empty() ? " " + label("SCROLL") + "  " : " " + label("SELECT") + "  " + ProductBinding(s, GameAction::Interact) + " " + label("CONFIRM") + "  ") +
            ProductBinding(s, GameAction::MoveLeft) + "/" + ProductBinding(s, GameAction::MoveRight) +
            " " + label("SCROLL") + "  " + ProductBinding(s, GameAction::Pause) + " " + label("BACK"));
        return rows;
    }
};

} // namespace writeover
