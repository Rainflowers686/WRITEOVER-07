#pragma once

#include <algorithm>
#include <array>
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
        std::vector<std::pair<std::string, std::string>> patterns;
        for (const auto& entry : translations) {
            if (entry.first.find_first_of("{}") == std::string::npos &&
                entry.second.find_first_of("{}") == std::string::npos) continue;
            // A template must contain an actual authored label, not just a
            // capture that would recursively translate itself forever.
            if (!ValidPattern(entry.first, entry.second)) return false;
            patterns.push_back(entry);
        }
        // Prefer complete authored sentences over generic suffixes such as
        // "{0} CLOSE". Alphabetical order alone lets the generic capture eat
        // an entire footer before its more specific template can match.
        std::stable_sort(patterns.begin(), patterns.end(), [](const auto& a, const auto& b) {
            const auto weight = [](const std::string& value) {
                return value.size() - 3 * static_cast<size_t>(std::count(value.begin(), value.end(), '{'));
            };
            return weight(a.first) > weight(b.first);
        });
        translations_ = std::move(translations);
        patterns_ = std::move(patterns);
        return true;
    }

    std::string Present(const std::string& source, const std::string& language) const {
        if (language != "zh-CN" || source.empty()) return source;
        const auto exact = translations_.find(source);
        if (exact != translations_.end()) return exact->second;
        if (source.rfind("> ", 0) == 0 || source.rfind("  ", 0) == 0)
            return source.substr(0, 2) + Present(source.substr(2), language);
        for (const auto& pattern : patterns_) {
            std::array<std::string, 4> captures;
            if (!Match(pattern.first, source, captures)) continue;
            std::string output;
            size_t position = 0;
            while (position < pattern.second.size()) {
                const size_t marker = pattern.second.find('{', position);
                output += pattern.second.substr(position, marker == std::string::npos ? marker : marker - position);
                if (marker == std::string::npos) break;
                const size_t index = static_cast<size_t>(pattern.second[marker + 1] - '0');
                output += Present(captures[index], language);
                position = marker + 3;
            }
            return output;
        }
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
    static bool ValidPattern(const std::string& source, const std::string& target) {
        // Sequential, unique source captures with literal separators. Targets
        // may reorder captures but must retain each exactly once. No catch-all
        // or adjacent captures: both would make matching ambiguous.
        size_t cursor = 0, count = 0, literal_columns = 0;
        while (cursor < source.size()) {
            if (source[cursor] == '}') return false;
            if (source[cursor] != '{') { ++literal_columns; ++cursor; continue; }
            if (count == 4) return false;
            const std::string token = "{" + std::to_string(count++) + "}";
            if (source.compare(cursor, token.size(), token) != 0) return false;
            cursor += token.size();
            if (cursor < source.size() && source[cursor] == '{') return false;
        }
        if (!count || !literal_columns) return false;
        std::array<bool, 4> used{};
        for (cursor = 0; cursor < target.size(); ++cursor) {
            if (target[cursor] == '}') return false;
            if (target[cursor] != '{') continue;
            if (cursor + 2 >= target.size() || target[cursor + 2] != '}' ||
                target[cursor + 1] < '0' || target[cursor + 1] >= '0' + static_cast<int>(count)) return false;
            const size_t index = static_cast<size_t>(target[cursor + 1] - '0');
            if (used[index]) return false;
            used[index] = true; cursor += 2;
        }
        for (size_t i = 0; i < count; ++i) if (!used[i]) return false;
        return true;
    }
    static bool Match(const std::string& pattern, const std::string& source,
                      std::array<std::string, 4>& captures) {
        size_t p = 0, s = 0;
        for (size_t index = 0; index < captures.size(); ++index) {
            const std::string token = "{" + std::to_string(index) + "}";
            const size_t marker = pattern.find(token, p);
            if (marker == std::string::npos) return source.compare(s, std::string::npos, pattern, p, std::string::npos) == 0;
            const std::string prefix = pattern.substr(p, marker - p);
            if (source.compare(s, prefix.size(), prefix) != 0) return false;
            s += prefix.size(); p = marker + token.size();
            const size_t next = pattern.find('{', p);
            const std::string separator = pattern.substr(p, next == std::string::npos ? next : next - p);
            if (separator.empty() && next != std::string::npos) return false;
            const size_t end = separator.empty() ? source.size() : source.find(separator, s);
            if (end == std::string::npos) return false;
            captures[index] = source.substr(s, end - s);
            s = end;
        }
        return source.compare(s, std::string::npos, pattern, p, std::string::npos) == 0;
    }
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
    std::vector<std::pair<std::string, std::string>> patterns_;
};

} // namespace writeover
