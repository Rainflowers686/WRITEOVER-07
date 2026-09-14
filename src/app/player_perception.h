#pragma once

#include "src/app/player_product.h"
#include "src/app/scene_runtime.h"
#include "writeover/ai/runtime.h"
#include "writeover/player/controller.h"
#include "writeover/world/grid.h"

#include <cmath>
#include <map>
#include <limits>

namespace writeover {

inline const char* PerceivedDirection(const LocomotionState& pose, const Vec3& source) {
    const float dx = source.x - pose.position.x, dy = source.y - pose.position.y;
    if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(pose.yaw)) return "";
    if (dx * dx + dy * dy < 0.0625f) return "near you";
    // Same camera-relative basis as locomotion: right=(-sin(yaw),cos(yaw)).
    const float forward = dx * std::cos(pose.yaw) + dy * std::sin(pose.yaw);
    const float right = -dx * std::sin(pose.yaw) + dy * std::cos(pose.yaw);
    if (std::fabs(forward) >= std::fabs(right)) return forward >= 0 ? "from ahead" : "from behind";
    return right >= 0 ? "from the right" : "from the left";
}

inline const char* PerceivedRole(Role role) {
    switch (role) {
    case Role::Guard: return "Security guard";
    case Role::Cleaner: return "Cleaner";
    case Role::Doctor: return "Medical staff";
    case Role::Technician: return "Technician";
    case Role::Researcher: return "Research staff";
    default: return "Facility staff";
    }
}

inline size_t PlayerKnownEvidence(const SystemicWorld& world, EntityId player) {
    return static_cast<size_t>(std::count_if(world.Knowledge().begin(), world.Knowledge().end(),
        [&](const KnowledgeAssetRecord& item) {
            return std::find(item.known_by.begin(), item.known_by.end(), player) != item.known_by.end();
        }));
}

// Conservative visible-perception gate: same room is checked by the caller;
// distance, horizontal and vertical field of view, and geometry must all agree.
inline bool PlayerCanPerceive(const LocomotionState& pose, const Vec3& target,
                             const IWorldQuery& query, float fov) {
    const Vec3 eye = pose.EyePosition();
    const float dx = target.x - eye.x, dy = target.y - eye.y;
    const float range = std::sqrt(dx * dx + dy * dy);
    if (range > 18.0f) return false;
    const float angle = std::remainder(std::atan2(dy, dx) - pose.yaw, 6.2831853f);
    const float vertical = std::atan2(target.z - eye.z, std::max(range, 0.01f));
    return std::fabs(angle) <= fov * 3.14159265f / 360.0f &&
        std::fabs(vertical - pose.pitch) <= fov * 3.14159265f / 360.0f &&
        query.LineOfSight(eye, target, eye.z);
}

class PlayerPerceptionObserver {
public:
    void Reset() { states_.clear(); cameras_.clear(); last_event_ = 0; initialized_ = false; }
    void Observe(PlayerProductRuntime& product, const std::vector<RuntimeNpc>& npcs,
                 const SceneRuntime& scene, const SystemicWorld& world,
                 const IWorldQuery& query, RoomId room, const std::string& room_name,
                 const LocomotionState& pose, uint64_t frame, const Settings& settings) {
        if (initialized_ && frame == last_frame_) return;
        if (initialized_ && frame < last_frame_) Reset();
        const bool had_baseline = initialized_;
        initialized_ = true;
        last_frame_ = frame;
        const auto visible = [&](Vec3 point) { return PlayerCanPerceive(pose, point, query, settings.fov); };
        const auto publish = [&](PerceptionCategory category, const std::string& text, const std::string& key) {
            product.feed.Publish(category, text, key, room_name, frame, true, true,
                settings.text_duration == 0 ? 300 : settings.text_duration == 2 ? 900 : 540);
        };
        for (const auto& npc : npcs) {
            const auto previous = states_.find(npc.instance.id.GetValue());
            Vec3 head = npc.instance.position; head.z += 1.3f;
            if (previous != states_.end() && previous->second != npc.instance.state &&
                npc.room == room && visible(head)) {
                const char* action = nullptr;
                switch (npc.instance.state) {
                case NPCState::Alert: action = " turns sharply toward a disturbance."; break;
                case NPCState::Investigate: action = " moves to check a disturbance."; break;
                case NPCState::Combat: action = " raises a weapon."; break;
                case NPCState::Stunned: action = " collapses under the discharge."; break;
                case NPCState::Dead: action = " falls motionless."; break;
                case NPCState::Flee: action = " breaks away."; break;
                case NPCState::Patrol: if (previous->second == NPCState::Investigate) action = " resumes the patrol."; break;
                default: break;
                }
                if (action) publish(npc.instance.state == NPCState::Combat ? PerceptionCategory::Threat : PerceptionCategory::Environment,
                    std::string(PerceivedRole(npc.instance.role)) + action + " / " +
                        PerceivedDirection(pose, npc.instance.position),
                    "npc:" + std::to_string(npc.instance.id.GetValue()));
            }
            if (states_.size() < 64 || previous != states_.end()) states_[npc.instance.id.GetValue()] = npc.instance.state;
        }
        for (const auto& entity : scene.Entities()) {
            if (entity.kind != SceneEntityKind::Camera || entity.systemic_id == 0) continue;
            const auto* camera = world.GetObservationSource(ObservationSourceId::New(entity.systemic_id));
            if (!camera) continue;
            const auto previous = cameras_.find(entity.stable_id);
            if (previous != cameras_.end() && previous->second != camera->online &&
                camera->room == room && visible(entity.position)) {
                publish(PerceptionCategory::Environment, camera->online ? "The camera indicator comes on." :
                    "The camera indicator goes dark.", "camera:" + std::to_string(entity.stable_id));
            }
            if (cameras_.size() < 32 || previous != cameras_.end()) cameras_[entity.stable_id] = camera->online;
        }
        for (const auto& event : world.SystemEvents()) {
            if (event.id.GetValue() <= last_event_) continue;
            if (had_baseline && event.type == SystemicEventType::BodyDiscovered && event.location == room) {
                for (const auto& npc : npcs) {
                    Vec3 head = npc.instance.position; head.z += 1.3f;
                    if (npc.instance.id.GetValue() == event.actor.GetValue() && npc.room == room && visible(head)) {
                        publish(PerceptionCategory::Environment, std::string(PerceivedRole(npc.instance.role)) +
                            " stops to examine the body.", "body-discovery");
                    }
                }
            }
            last_event_ = std::max(last_event_, event.id.GetValue());
        }
    }
private:
    std::map<uint64_t, NPCState> states_;
    std::map<uint64_t, bool> cameras_;
    uint64_t last_event_ = 0;
    uint64_t last_frame_ = 0;
    bool initialized_ = false;
};

inline std::vector<std::string> InspectVisibleTarget(
    const std::vector<RuntimeNpc>& npcs, const SceneRuntime& scene,
    const SystemicWorld& world, RoomId room, const std::string& room_name,
    const Vec3& eye,
    const std::function<bool(const Vec3&, float, float)>& focused,
    const std::function<bool(const char*)>& focused_entity) {
    enum class Kind { None, Body, Npc, Prop };
    Kind nearest_kind = Kind::None;
    uint64_t nearest_id = 0;
    float nearest_distance = std::numeric_limits<float>::infinity();
    const auto offer = [&](Kind kind, uint64_t id, const Vec3& position, float radius) {
        const float dx = position.x - eye.x, dy = position.y - eye.y, dz = position.z - eye.z;
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz) - radius;
        if (distance < nearest_distance) { nearest_distance = distance; nearest_kind = kind; nearest_id = id; }
    };
    for (const auto& body : world.Bodies()) if (body.room == room && body.disposition == BodyDisposition::Exposed &&
        focused(body.position, 1.2f, 0.45f)) offer(Kind::Body, body.id.GetValue(), body.position, 1.2f);
    for (const auto& npc : npcs) if (npc.room == room && npc.instance.state != NPCState::Dead &&
        npc.instance.state != NPCState::Stunned && focused(npc.instance.position, 0.52f, 1.85f))
        offer(Kind::Npc, npc.instance.id.GetValue(), npc.instance.position, 0.52f);
    for (const auto& entity : scene.Entities()) if (entity.room == room_name && focused_entity(entity.id.c_str()))
        offer(Kind::Prop, entity.stable_id, entity.position, entity.radius);
    for (const auto& body : world.Bodies()) {
        if (nearest_kind == Kind::Body && body.id.GetValue() == nearest_id) {
            return {"EXAMINE / BODY", body.status == BodyStatus::Dead ? "Motionless. No visible breathing." :
                "Shallow breathing. The person is unconscious.", "Possessions are not visible. Use Search to find out more."};
        }
    }
    for (const auto& npc : npcs) {
        if (nearest_kind != Kind::Npc || npc.instance.id.GetValue() != nearest_id) continue;
        const char* clothing = npc.instance.role == Role::Guard ? "Helmet, dark visor, broad protective vest and duty equipment." :
            npc.instance.role == Role::Cleaner ? "Work cap, heavy overalls and practical cleaning equipment." :
            npc.instance.role == Role::Technician ? "Work clothes, tool pockets and a distracted stance beside the equipment." :
            npc.instance.role == Role::Doctor ? "Clinical coat and medical insignia. Hands kept clear of the equipment." :
            "Facility clothing, a visible identification patch and a reserved stance.";
        return {std::string("EXAMINE / ") + PerceivedRole(npc.instance.role), clothing,
            "This tells you what you can see, not what this person knows or carries."};
    }
    for (const auto& entity : scene.Entities()) {
        if (nearest_kind != Kind::Prop || entity.stable_id != nearest_id) continue;
        switch (entity.kind) {
        case SceneEntityKind::Camera: {
            const auto* source = world.GetObservationSource(ObservationSourceId::New(entity.systemic_id));
            return {"EXAMINE / CAMERA", "A wall-mounted lens housing and status indicator.",
                source && !source->online ? "The indicator is dark." : "The indicator is lit. Its field of view is not marked."};
        }
        case SceneEntityKind::DoorReader: return {"EXAMINE / ACCESS READER", "Credential slot and a small response lamp.", "Present a credential to learn whether this reader accepts it."};
        case SceneEntityKind::Door: return {"EXAMINE / DOOR", "A framed threshold, recessed panel and access mechanism.", "The adjacent interaction prompt describes your available action."};
        case SceneEntityKind::Terminal: return {"EXAMINE / TERMINAL", "An institutional workstation for this room's department.", "Its screen is the direct source for records and access responses. Interact to read it."};
        case SceneEntityKind::Cart: return {"EXAMINE / SERVICE CART", "A wheeled industrial container. Its enclosed compartment obscures the contents."};
        default: return {"EXAMINE / EQUIPMENT", "A fixed equipment housing. Its shape marks work space rather than a passage."};
        }
    }
    return {"EXAMINE", "No clear target in reach. Aim at a person, body, camera, reader, door or workstation."};
}

} // namespace writeover
