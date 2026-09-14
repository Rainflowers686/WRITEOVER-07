#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace writeover {

// Text resources use the existing stable-id TAB UTF-8 convention. The English
// text remains the event/history payload; only the display projection changes.
// This keeps saves, storylet eligibility and replay receipts language-neutral.
class PresentationText {
public:
    bool Load(const std::filesystem::path& directory) {
        std::map<std::string, std::string> english, chinese;
        if (!Read(directory / "recovery_text.txt", english) ||
            !Read(directory / "recovery_text.zh-CN.txt", chinese)) return false;
        if (english.size() != chinese.size()) return false;
        std::map<std::string, std::string> translations;
        for (const auto& entry : english) {
            const auto found = chinese.find(entry.first);
            if (found == chinese.end()) return false;
            const auto inserted = translations.emplace(entry.second, found->second);
            if (!inserted.second && inserted.first->second != found->second) return false;
        }
        std::map<std::string, std::string> ui_en, ui_zh;
        if (!Read(directory / "interface.en.txt", ui_en) ||
            !Read(directory / "interface.zh-CN.txt", ui_zh) || ui_en.size() != ui_zh.size()) return false;
        for (const auto& entry : ui_en) {
            const auto found = ui_zh.find(entry.first);
            if (found == ui_zh.end()) return false;
            const auto inserted = translations.emplace(entry.second, found->second);
            if (!inserted.second && inserted.first->second != found->second) return false;
        }
        translations_ = std::move(translations);
        return true;
    }

    std::string Present(const std::string& source, const std::string& language) const {
        if (language != "zh-CN" || source.empty()) return source;
        const auto exact = translations_.find(source);
        if (exact != translations_.end()) return exact->second;
        if (source.rfind("> ", 0) == 0 || source.rfind("  ", 0) == 0)
            return source.substr(0, 2) + Present(source.substr(2), language);
        // Existing menus compose independently meaningful labels with this
        // separator. Translate the labels without interpreting numbers/keys.
        if (source.find(" / ") != std::string::npos) {
            std::string output;
            size_t begin = 0;
            while (begin < source.size()) {
                const size_t end = source.find(" / ", begin);
                if (!output.empty()) output += " / ";
                output += Present(source.substr(begin, end == std::string::npos ? end : end - begin), language);
                if (end == std::string::npos) break;
                begin = end + 3;
            }
            return output;
        }
        return source; // Coverage is audited separately; never invent text.
    }
    size_t Count() const { return translations_.size(); }

private:
    static bool Read(const std::filesystem::path& path, std::map<std::string, std::string>& result) {
        std::ifstream stream(path);
        if (!stream) return false;
        std::string line;
        size_t count = 0;
        while (std::getline(stream, line)) {
            if (++count > 4096 || line.size() > 4096) return false;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line.front() == '#') continue;
            const size_t tab = line.find('\t');
            if (tab == std::string::npos || tab == 0 || tab > 128 || tab + 1 >= line.size()) return false;
            if (!result.emplace(line.substr(0, tab), line.substr(tab + 1)).second) return false;
        }
        return !stream.bad();
    }
    std::map<std::string, std::string> translations_;
};

} // namespace writeover
