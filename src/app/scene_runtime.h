#pragma once

// Bounded compiled placement data for the recovery slice.  This is an
// authored scene seam, not an ECS: render, interaction, and room switching
// read the same records and the data set remains intentionally small.

#include "writeover/common/ids.h"
#include "writeover/common/types.h"
#include "writeover/render/character_renderer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

enum class SceneEntityKind : uint8_t {
    Cart = 0,
    Camera = 1,
    Terminal = 2,
    DoorReader = 3,
    Door = 4,
    Crate = 5,
};

struct SceneEntity {
    uint64_t stable_id = 0;
    SceneEntityKind kind = SceneEntityKind::Crate;
    std::string id;
    std::string room;
    Vec3 position;
    float yaw = 0.0f;
    float radius = 1.0f;
    float height = 1.0f;
    CharacterSpriteKind visual = CharacterSpriteKind::Crate;
    uint64_t systemic_id = 0;
    uint64_t link_id = 0;
};

struct SceneTransition {
    uint64_t stable_id = 0;
    std::string id;
    std::string source_room;
    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_y = 0.0f;
    float max_y = 0.0f;
    std::string destination_room;
    Vec3 destination_spawn;
    float destination_yaw = 0.0f;
    std::string unavailable_message;
};

struct ScenePatrolRoute {
    NpcId npc;
    std::string room;
    std::vector<Vec3> points;
};

class SceneRuntime final {
public:
    bool Load(const std::string& path);

    const std::vector<SceneEntity>& Entities() const { return entities_; }
    const std::vector<SceneTransition>& Transitions() const { return transitions_; }
    const std::vector<ScenePatrolRoute>& PatrolRoutes() const { return patrol_routes_; }

    const SceneEntity* FindEntity(const std::string& id) const;
    const SceneEntity* FindEntity(SceneEntityKind kind,
                                  const std::string& room) const;
    const SceneTransition* FindTransition(const std::string& id) const;

private:
    std::vector<SceneEntity> entities_;
    std::vector<SceneTransition> transitions_;
    std::vector<ScenePatrolRoute> patrol_routes_;
};

} // namespace writeover
