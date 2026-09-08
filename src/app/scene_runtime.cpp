#include "src/app/scene_runtime.h"

#include "writeover/common/io.h"
#include "writeover/common/serialize.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace writeover {

namespace {

constexpr uint32_t kSceneMagic = 0x57534331; // "WSC1"
constexpr uint32_t kSceneVersion = 1;
constexpr uint32_t kMaxSceneEntities = 256;
constexpr uint32_t kMaxSceneTransitions = 128;
constexpr uint32_t kMaxPatrolRoutes = 64;
constexpr uint32_t kMaxPatrolPoints = 32;
constexpr uint32_t kMaxStringBytes = 512;

bool Finite(const Vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

std::string ReadBoundedString(Deserializer& d) {
    const uint32_t length = d.ReadU32();
    if (d.HasError() || length > kMaxStringBytes || length > d.Remaining()) {
        d.MarkError();
        return {};
    }
    std::string value(length, '\0');
    if (length != 0) d.ReadBytes(value.data(), length);
    return value;
}

bool ReadVec3(Deserializer& d, Vec3& value) {
    value.x = d.ReadF32();
    value.y = d.ReadF32();
    value.z = d.ReadF32();
    return !d.HasError() && Finite(value);
}

bool ValidEntityKind(uint8_t value) {
    return value <= static_cast<uint8_t>(SceneEntityKind::Crate);
}

bool ValidVisual(uint8_t value) {
    return value <= static_cast<uint8_t>(CharacterSpriteKind::BodyDead);
}

} // namespace

bool SceneRuntime::Load(const std::string& path) {
    const auto bytes = ReadFileBinary(path);
    if (bytes.IsError() || bytes.Value().size() > 4u * 1024u * 1024u) {
        return false;
    }
    Deserializer d(bytes.Value().data(), bytes.Value().size());
    if (d.ReadU32() != kSceneMagic || d.ReadU32() != kSceneVersion) {
        return false;
    }

    std::vector<SceneEntity> entities;
    const uint32_t entity_count = d.ReadU32();
    if (d.HasError() || entity_count > kMaxSceneEntities) return false;
    for (uint32_t i = 0; i < entity_count; ++i) {
        SceneEntity entity;
        entity.stable_id = d.ReadU64();
        const uint8_t kind = d.ReadU8();
        const uint8_t visual = d.ReadU8();
        entity.id = ReadBoundedString(d);
        entity.room = ReadBoundedString(d);
        entity.systemic_id = d.ReadU64();
        entity.link_id = d.ReadU64();
        if (!ReadVec3(d, entity.position)) return false;
        entity.yaw = d.ReadF32();
        entity.radius = d.ReadF32();
        entity.height = d.ReadF32();
        if (d.HasError() || entity.stable_id == 0 || entity.id.empty() ||
            entity.room.empty() || !ValidEntityKind(kind) || !ValidVisual(visual) ||
            !std::isfinite(entity.yaw) || !std::isfinite(entity.radius) ||
            !std::isfinite(entity.height) || entity.radius <= 0.0f ||
            entity.height <= 0.0f) {
            return false;
        }
        entity.kind = static_cast<SceneEntityKind>(kind);
        entity.visual = static_cast<CharacterSpriteKind>(visual);
        if (std::any_of(entities.begin(), entities.end(), [&](const SceneEntity& prior) {
                return prior.stable_id == entity.stable_id || prior.id == entity.id;
            })) {
            return false;
        }
        entities.push_back(std::move(entity));
    }

    std::vector<SceneTransition> transitions;
    const uint32_t transition_count = d.ReadU32();
    if (d.HasError() || transition_count > kMaxSceneTransitions) return false;
    for (uint32_t i = 0; i < transition_count; ++i) {
        SceneTransition transition;
        transition.stable_id = d.ReadU64();
        transition.id = ReadBoundedString(d);
        transition.source_room = ReadBoundedString(d);
        transition.min_x = d.ReadF32();
        transition.max_x = d.ReadF32();
        transition.min_y = d.ReadF32();
        transition.max_y = d.ReadF32();
        transition.destination_room = ReadBoundedString(d);
        if (!ReadVec3(d, transition.destination_spawn)) return false;
        transition.destination_yaw = d.ReadF32();
        transition.unavailable_message = ReadBoundedString(d);
        if (d.HasError() || transition.stable_id == 0 || transition.id.empty() ||
            transition.source_room.empty() || transition.destination_room.empty() ||
            !std::isfinite(transition.min_x) || !std::isfinite(transition.max_x) ||
            !std::isfinite(transition.min_y) || !std::isfinite(transition.max_y) ||
            transition.min_x > transition.max_x || transition.min_y > transition.max_y ||
            !std::isfinite(transition.destination_yaw)) {
            return false;
        }
        if (std::any_of(transitions.begin(), transitions.end(),
                        [&](const SceneTransition& prior) {
                            return prior.stable_id == transition.stable_id ||
                                   prior.id == transition.id;
                        })) {
            return false;
        }
        transitions.push_back(std::move(transition));
    }

    std::vector<ScenePatrolRoute> routes;
    const uint32_t route_count = d.ReadU32();
    if (d.HasError() || route_count > kMaxPatrolRoutes) return false;
    for (uint32_t i = 0; i < route_count; ++i) {
        ScenePatrolRoute route;
        route.npc = NpcId::New(d.ReadU64());
        route.room = ReadBoundedString(d);
        const uint32_t point_count = d.ReadU32();
        if (d.HasError() || !route.npc.IsValid() || route.room.empty() ||
            point_count == 0 || point_count > kMaxPatrolPoints) return false;
        route.points.reserve(point_count);
        for (uint32_t p = 0; p < point_count; ++p) {
            Vec3 point;
            if (!ReadVec3(d, point)) return false;
            route.points.push_back(point);
        }
        if (std::any_of(routes.begin(), routes.end(), [&](const ScenePatrolRoute& prior) {
                return prior.npc == route.npc;
            })) {
            return false;
        }
        routes.push_back(std::move(route));
    }
    if (d.HasError() || !d.AtEnd()) return false;

    const auto room_exists = [&](const std::string& room) {
        return std::any_of(entities.begin(), entities.end(), [&](const SceneEntity& entity) {
                   return entity.room == room;
               }) || std::any_of(transitions.begin(), transitions.end(),
                                  [&](const SceneTransition& link) {
                                      return link.source_room == room ||
                                             link.destination_room == room;
                                  });
    };
    for (const auto& entity : entities) if (!room_exists(entity.room)) return false;
    for (const auto& transition : transitions) {
        if (!room_exists(transition.source_room) ||
            !room_exists(transition.destination_room)) return false;
    }

    entities_ = std::move(entities);
    transitions_ = std::move(transitions);
    patrol_routes_ = std::move(routes);
    return true;
}

const SceneEntity* SceneRuntime::FindEntity(const std::string& id) const {
    const auto it = std::find_if(entities_.begin(), entities_.end(),
                                 [&](const SceneEntity& entity) {
                                     return entity.id == id;
                                 });
    return it == entities_.end() ? nullptr : &*it;
}

const SceneEntity* SceneRuntime::FindEntity(SceneEntityKind kind,
                                            const std::string& room) const {
    const auto it = std::find_if(entities_.begin(), entities_.end(),
                                 [&](const SceneEntity& entity) {
                                     return entity.kind == kind && entity.room == room;
                                 });
    return it == entities_.end() ? nullptr : &*it;
}

const SceneTransition* SceneRuntime::FindTransition(const std::string& id) const {
    const auto it = std::find_if(transitions_.begin(), transitions_.end(),
                                 [&](const SceneTransition& transition) {
                                     return transition.id == id;
                                 });
    return it == transitions_.end() ? nullptr : &*it;
}

} // namespace writeover
