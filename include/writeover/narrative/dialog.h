#pragma once
// Dialogue/subtitle queue. Speaker is a strong EntityId or the narrator
// special id — never a uint8_t (M 17.4 closure).

#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace writeover {

struct SubtitleLine {
    std::string text;          // UTF-8 display text or text id
    uint32_t start_frame = 0;
    uint32_t ttl_frames = 0;
    NpcId speaker_id;          // or NarratorSpeakerId() special value
    uint8_t persona = 0;       // narrator persona filter
    uint32_t tag = 0;          // stale-VO cancellation group
};

class DialogueQueue {
public:
    void Push(const SubtitleLine& line);
    void CancelByTag(uint32_t tag);
    void Advance(uint32_t frame);  // drops expired lines
    std::vector<SubtitleLine> ActiveLines(uint32_t frame) const;
    size_t Count() const { return queue_.size(); }

    void Save(Serializer& s) const;
    void Load(Deserializer& d);

private:
    std::deque<SubtitleLine> queue_;
};

} // namespace writeover