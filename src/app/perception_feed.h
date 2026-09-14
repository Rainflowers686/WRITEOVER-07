#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace writeover {

enum class PerceptionCategory : uint8_t { Environment, Dialogue, Threat, Progress };

struct PerceivedMessage {
    PerceptionCategory category = PerceptionCategory::Environment;
    std::string text;
    std::string key;
    std::string room;
    uint64_t frame = 0;
    uint64_t expires = 0;
    bool important = true;
};

// Presentation only: callers must prove perception before publishing. Neither
// delivery nor filtering may change gameplay. Nothing here enters the save wire.
class PerceptionFeed {
public:
    static constexpr size_t kCapacity = 48;
    bool Publish(PerceptionCategory category, const std::string& text,
                 const std::string& key, const std::string& room, uint64_t frame,
                 bool perceptible = true, bool important = true,
                 uint64_t duration = 480) {
        if (!perceptible || text.empty()) return false;
        const std::string bounded_key = key.substr(0, 96);
        const std::string bounded_text = text.substr(0, 480);
        const std::string bounded_room = room.substr(0, 128);
        for (auto it = messages_.rbegin(); it != messages_.rend(); ++it) {
            if (frame >= it->frame && frame - it->frame > 600) break;
            if (it->key == bounded_key && it->room == bounded_room && it->text == bounded_text) return false;
        }
        if (messages_.size() == kCapacity) messages_.pop_front();
        messages_.push_back({category, bounded_text, bounded_key,
                             bounded_room, frame, frame + duration, important});
        return true;
    }
    void Clear() { messages_.clear(); }
    size_t Size() const { return messages_.size(); }
    const std::deque<PerceivedMessage>& Messages() const { return messages_; }
    std::vector<std::string> History(bool dialogue_only) const {
        std::vector<std::string> rows;
        rows.push_back(dialogue_only ? "DIALOGUE / RECENTLY HEARD" : "RECENT EVENTS / WHAT YOU PERCEIVED");
        for (auto it = messages_.rbegin(); it != messages_.rend(); ++it) {
            if (dialogue_only && it->category != PerceptionCategory::Dialogue) continue;
            rows.push_back(std::string(Label(it->category)) + " / " + it->text);
        }
        if (rows.size() == 1) rows.push_back("No recent entries. Loading a save clears this history.");
        return rows;
    }
    std::vector<std::string> Visible(uint64_t frame, uint8_t verbosity,
                                     const std::string& subtitle = {}, const std::string& objective = {}) const {
        std::vector<std::string> rows;
        if (verbosity == 0) return rows;
        // Threats have display precedence; history still retains insertion order.
        for (int pass = 0; pass < 2 && rows.size() < 2; ++pass) {
            for (auto it = messages_.rbegin(); it != messages_.rend() && rows.size() < 2; ++it) {
                if (it->frame > frame || it->expires <= frame || (!it->important && verbosity == 1)) continue;
                if ((!subtitle.empty() && it->text == subtitle) ||
                    (!objective.empty() && it->text == "Objective: " + objective)) continue;
                if ((it->category == PerceptionCategory::Threat) != (pass == 0)) continue;
                rows.push_back(std::string(Label(it->category)) + " / " + it->text);
            }
        }
        return rows;
    }
    static const char* Label(PerceptionCategory category) {
        switch (category) {
        case PerceptionCategory::Dialogue: return "HEARD";
        case PerceptionCategory::Threat: return "!";
        case PerceptionCategory::Progress: return "+";
        default: return "NEARBY";
        }
    }
private:
    std::deque<PerceivedMessage> messages_;
};

} // namespace writeover
