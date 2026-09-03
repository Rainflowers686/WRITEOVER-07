#include "writeover/narrative/dialog.h"

#include <algorithm>

namespace writeover {

void DialogueQueue::Push(const SubtitleLine& line) {
    queue_.push_back(line);
}

void DialogueQueue::CancelByTag(uint32_t tag) {
    queue_.erase(std::remove_if(queue_.begin(), queue_.end(),
                                [tag](const SubtitleLine& l) { return l.tag == tag; }),
                 queue_.end());
}

void DialogueQueue::Advance(uint32_t frame) {
    queue_.erase(std::remove_if(queue_.begin(), queue_.end(),
                                [frame](const SubtitleLine& l) {
                                    return l.start_frame + l.ttl_frames < frame;
                                }),
                 queue_.end());
}

std::vector<SubtitleLine> DialogueQueue::ActiveLines(uint32_t frame) const {
    std::vector<SubtitleLine> out;
    for (const auto& line : queue_) {
        if (line.start_frame <= frame &&
            line.start_frame + line.ttl_frames >= frame) {
            out.push_back(line);
        }
    }
    return out;
}

void DialogueQueue::Save(Serializer& s) const {
    s.WriteU32(static_cast<uint32_t>(queue_.size()));
    for (const auto& line : queue_) {
        s.WriteString(line.text);
        s.WriteU32(line.start_frame);
        s.WriteU32(line.ttl_frames);
        WriteId(s, line.speaker_id);
        s.WriteU8(line.persona);
        s.WriteU32(line.tag);
    }
}

void DialogueQueue::Load(Deserializer& d) {
    queue_.clear();
    const uint32_t count = d.ReadU32();
    for (uint32_t i = 0; i < count; ++i) {
        SubtitleLine line;
        line.text = d.ReadString();
        line.start_frame = d.ReadU32();
        line.ttl_frames = d.ReadU32();
        line.speaker_id = ReadId<NpcId>(d);
        line.persona = d.ReadU8();
        line.tag = d.ReadU32();
        queue_.push_back(line);
    }
}

} // namespace writeover