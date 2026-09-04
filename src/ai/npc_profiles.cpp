#include "writeover/ai/npc.h"

#include "writeover/common/io.h"
#include "writeover/common/serialize.h"

#include <algorithm>
#include <cmath>

namespace writeover {

namespace {
constexpr uint32_t kNpcProfileMagic = 0x574E5043u; // "WNPC"
constexpr uint32_t kNpcProfileVersion = 1;
constexpr uint32_t kMaxNpcProfiles = 64;
constexpr float kMaxFov = 6.2832f;

bool ValidCognition(uint8_t value) {
    return value <= static_cast<uint8_t>(CognitionTier::SemiHuman);
}

bool ValidFaction(uint8_t value) {
    return value <= static_cast<uint8_t>(Faction::Civilian);
}

bool ValidRole(uint8_t value) {
    return value <= static_cast<uint8_t>(Role::Other);
}
} // namespace

Result<std::vector<NpcProfile>> LoadNpcProfiles(const std::string& path) {
    const auto bytes = ReadFileBinary(path);
    if (bytes.IsError()) {
        return Result<std::vector<NpcProfile>>::Err(bytes.Error().code,
                                                     bytes.Error().message);
    }
    Deserializer deserializer(bytes.Value().data(), bytes.Value().size());
    const uint32_t magic = deserializer.ReadU32();
    const uint32_t version = deserializer.ReadU32();
    const uint32_t count = deserializer.ReadU32();
    if (deserializer.HasError() || magic != kNpcProfileMagic ||
        version != kNpcProfileVersion || count > kMaxNpcProfiles) {
        return Result<std::vector<NpcProfile>>::Err(1,
                                                     "invalid NPC profile header");
    }

    std::vector<NpcProfile> profiles;
    profiles.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        NpcProfile profile;
        const uint64_t id = deserializer.ReadU64();
        const uint64_t data_key = deserializer.ReadU64();
        const uint64_t room = deserializer.ReadU64();
        const uint8_t cognition = deserializer.ReadU8();
        const uint8_t faction = deserializer.ReadU8();
        const uint8_t role = deserializer.ReadU8();
        profile.spawn_position.x = deserializer.ReadF32();
        profile.spawn_position.y = deserializer.ReadF32();
        profile.spawn_position.z = deserializer.ReadF32();
        profile.spawn_yaw = deserializer.ReadF32();
        profile.health = deserializer.ReadU16();
        profile.is_critical = deserializer.ReadU8() != 0;
        profile.sight_range = deserializer.ReadF32();
        profile.sight_fov_rad = deserializer.ReadF32();
        profile.hearing_range = deserializer.ReadF32();
        profile.id = NpcId::New(id);
        profile.spawn_room = RoomId::New(room);

        if (deserializer.HasError() || !profile.id.IsValid() ||
            !profile.spawn_room.IsValid() || data_key != id ||
            !ValidCognition(cognition) || !ValidFaction(faction) ||
            !ValidRole(role) || profile.health == 0 || profile.health > 1000 ||
            !std::isfinite(profile.spawn_position.x) ||
            !std::isfinite(profile.spawn_position.y) ||
            !std::isfinite(profile.spawn_position.z) ||
            !std::isfinite(profile.spawn_yaw) ||
            !std::isfinite(profile.sight_range) ||
            !std::isfinite(profile.sight_fov_rad) ||
            !std::isfinite(profile.hearing_range) ||
            profile.sight_range < 0.0f || profile.sight_range > 64.0f ||
            profile.sight_fov_rad < 0.0f || profile.sight_fov_rad > kMaxFov ||
            profile.hearing_range < 0.0f || profile.hearing_range > 64.0f) {
            return Result<std::vector<NpcProfile>>::Err(2,
                                                         "invalid NPC profile record");
        }
        if (std::any_of(profiles.begin(), profiles.end(),
                        [&](const NpcProfile& prior) { return prior.id == profile.id; })) {
            return Result<std::vector<NpcProfile>>::Err(3,
                                                         "duplicate NPC profile id");
        }
        profiles.push_back(profile);
    }
    if (deserializer.HasError() || !deserializer.AtEnd()) {
        return Result<std::vector<NpcProfile>>::Err(4,
                                                     "trailing NPC profile data");
    }
    return Result<std::vector<NpcProfile>>::Ok(std::move(profiles));
}

} // namespace writeover
