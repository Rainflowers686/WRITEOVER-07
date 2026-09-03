#include "writeover/common/clock.h"

#include "writeover/common/serialize.h"

namespace writeover {

void SimClock::Save(Serializer& s) const { s.WriteU64(frame_count_); }

void SimClock::Load(Deserializer& d) { frame_count_ = d.ReadU64(); }

} // namespace writeover