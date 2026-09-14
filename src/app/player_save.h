#pragma once

#include "writeover/player/controller.h"
#include "writeover/player/combat.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace writeover {

// Private Player-section format, not a new global save schema. The marker
// cannot be a legacy room-name length (bounded to 128 bytes).
inline constexpr uint32_t kPlayerPayloadMagic = 0x32594C50; // PLY2
inline constexpr uint16_t kPlayerPayloadVersion = 1;
inline constexpr size_t kPlayerPayloadHeaderBytes = 8;

struct PlayerSaveData {
    std::string room;
    LocomotionState locomotion;
    CombatState combat;
    uint16_t health = 100;
    bool dead = false;
};

inline uint32_t LongestPlayerFireCooldown() {
    uint32_t frames = 0;
    for (const auto& weapon : DefaultWeapons()) {
        frames = std::max(frames, static_cast<uint32_t>(120.0f / weapon.fire_rate_hz + 0.5f));
    }
    return frames;
}

inline std::vector<uint8_t> SerializePlayerSave(const PlayerSaveData& player, uint32_t frame) {
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    s.WriteU32(kPlayerPayloadMagic);
    s.WriteU16(kPlayerPayloadVersion);
    s.WriteU16(0); // reserved; must remain zero
    s.WriteString(player.room);
    const auto& loco = player.locomotion;
    s.WriteF32(loco.position.x); s.WriteF32(loco.position.y); s.WriteF32(loco.position.z);
    s.WriteF32(loco.velocity.x); s.WriteF32(loco.velocity.y); s.WriteF32(loco.velocity.z);
    s.WriteF32(loco.yaw); s.WriteF32(loco.pitch);
    s.WriteU8(static_cast<uint8_t>(loco.posture));
    s.WriteU8(static_cast<uint8_t>(loco.traversal));
    s.WriteU8(static_cast<uint8_t>(loco.lean));
    s.WriteU8(loco.contact.grounded ? 1 : 0);
    s.WriteU8(loco.contact.on_ladder ? 1 : 0);
    s.WriteU8(loco.contact.on_climbable ? 1 : 0);
    CombatState relative = player.combat;
    const uint32_t remaining = relative.next_fire_frame - frame;
    relative.next_fire_frame = remaining <= LongestPlayerFireCooldown() ? remaining : 0;
    relative.last_shot_frame = 0;
    SerializeCombatState(s, relative);
    s.WriteU16(player.health);
    s.WriteU8(player.dead ? 1 : 0);
    s.WriteU16(loco.jump_cooldown_frames);
    return bytes;
}

inline bool ParsePlayerSave(const std::vector<uint8_t>& bytes, uint32_t frame, PlayerSaveData& out) {
    Deserializer d(bytes.data(), bytes.size());
    const uint32_t first = d.ReadU32();
    const bool versioned = first == kPlayerPayloadMagic;
    uint32_t room_len = first;
    if (versioned) {
        const uint16_t version = d.ReadU16();
        const uint16_t reserved = d.ReadU16();
        if (version != kPlayerPayloadVersion || reserved != 0) return false;
        room_len = d.ReadU32();
    }
    if (d.HasError() || room_len == 0 || room_len > 128 || room_len > d.Remaining()) return false;
    PlayerSaveData staged;
    staged.room.resize(room_len);
    d.ReadBytes(staged.room.data(), room_len);
    auto& loco = staged.locomotion;
    loco.position.x = d.ReadF32(); loco.position.y = d.ReadF32(); loco.position.z = d.ReadF32();
    loco.velocity.x = d.ReadF32(); loco.velocity.y = d.ReadF32(); loco.velocity.z = d.ReadF32();
    loco.yaw = d.ReadF32(); loco.pitch = d.ReadF32();
    const uint8_t posture = d.ReadU8();
    const uint8_t traversal = d.ReadU8();
    const uint8_t lean = d.ReadU8();
    const uint8_t grounded = d.ReadU8();
    const uint8_t ladder = d.ReadU8();
    const uint8_t climbable = d.ReadU8();
    DeserializeCombatState(d, staged.combat);
    // Only the complete previous candidate layout is supported. Accepting an
    // absent health/jump tail cannot distinguish an old layout from truncation.
    staged.health = d.ReadU16();
    const uint8_t dead = d.ReadU8();
    staged.dead = dead != 0;
    loco.jump_cooldown_frames = d.ReadU16();
    if (d.HasError() || !d.AtEnd() || grounded > 1 || ladder > 1 || climbable > 1 || dead > 1 ||
        posture >= static_cast<uint8_t>(Posture::Count) ||
        traversal >= static_cast<uint8_t>(Traversal::Count) ||
        lean >= static_cast<uint8_t>(Lean::Count) ||
        !std::isfinite(loco.position.x) || !std::isfinite(loco.position.y) || !std::isfinite(loco.position.z) ||
        !std::isfinite(loco.velocity.x) || !std::isfinite(loco.velocity.y) || !std::isfinite(loco.velocity.z) ||
        !std::isfinite(loco.yaw) || !std::isfinite(loco.pitch) ||
        loco.jump_cooldown_frames > kJumpCooldownFrames || staged.health > 100 ||
        staged.dead != (staged.health == 0)) return false;
    if (versioned && staged.combat.last_shot_frame != 0) return false;
    loco.posture = static_cast<Posture>(posture);
    loco.traversal = static_cast<Traversal>(traversal);
    loco.lean = static_cast<Lean>(lean);
    loco.contact = {grounded != 0, ladder != 0, climbable != 0};
    // Legacy saves have no saved clock. Preserve at most one authored cooldown
    // instead of treating their absolute scheduler frame as a new-process deadline.
    const uint32_t remaining = versioned ? staged.combat.next_fire_frame :
        staged.combat.next_fire_frame - staged.combat.last_shot_frame;
    if (remaining > LongestPlayerFireCooldown()) return false;
    staged.combat.last_shot_frame = frame;
    staged.combat.next_fire_frame = frame + remaining;
    out = std::move(staged);
    return true;
}

} // namespace writeover
