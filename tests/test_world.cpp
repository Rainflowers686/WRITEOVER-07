#include "tests/test_harness.h"

#include "writeover/world/fact_belief.h"
#include "writeover/world/grid.h"
#include "writeover/world/map_validator.h"
#include "writeover/world/infrastructure.h"
#include "writeover/world/room.h"

#include <cmath>

namespace writeover {

namespace {

Grid MakeTestGrid() {
    Grid grid(8, 6);
    for (int32_t r = 0; r < 6; ++r) {
        for (int32_t c = 0; c < 8; ++c) {
            GridCell cell;
            cell.flags = 0;
            grid.SetCell(c, r, cell);
        }
    }
    // Solid wall around the map: mark a vertical wall at col 7 (single column).
    for (int32_t r = 0; r < 6; ++r) {
        GridCell wall;
        wall.flags = CellFlag_Solid;
        grid.SetCell(7, r, wall);
    }
    return grid;
}

bool GridCollision() {
    const Grid grid = MakeTestGrid();
    GridWorldQuery query(&grid);
    // Standing box inside the open floor at z=0 is not blocked.
    AABB stand;
    stand.min = Vec3{1.35f, 1.35f, 0.0f};
    stand.max = Vec3{1.65f, 1.65f, kColliderStand};
    WO_CHECK(!query.AabbBlocked(stand));
    // Box overlapping the solid wall at col 7 is blocked.
    AABB into_wall;
    into_wall.min = Vec3{6.9f, 1.35f, 0.0f};
    into_wall.max = Vec3{7.1f, 1.65f, kColliderStand};
    return query.AabbBlocked(into_wall);
}

bool FloorCeilingQuery() {
    Grid grid(4, 4);
    GridCell low;
    low.floor_height = 0.0f;
    low.ceiling_height = 1.0f;  // crouch-only
    grid.SetCell(1, 1, low);
    GridWorldQuery query(&grid);
    return std::fabs(query.CeilingHeightAt(1.5f, 1.5f) - 1.0f) < 1e-4f;
}

bool FactBeliefRoundTrip() {
    FactStore store;
    store.Set(WorldFact{FactId::New(1), EntityId::New(10), PredicateType::State, true});
    store.Set(WorldFact{FactId::New(2), EntityId::New(20), PredicateType::Relation, EntityId::New(30)});
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    store.Save(s);
    FactStore out;
    Deserializer d(bytes.data(), bytes.size());
    out.Load(d);
    WorldFact f;
    return out.Get(FactId::New(1), f) && std::get<bool>(f.value) == true &&
           out.Get(FactId::New(2), f) &&
           std::get<EntityId>(f.value) == EntityId::New(30);
}

bool BeliefSetOrdering() {
    BeliefSet set;
    set.Upsert(NpcId::New(5), FactId::New(1), 0.9f, BeliefSource::Observed, 10);
    set.Upsert(NpcId::New(5), FactId::New(2), 0.4f, BeliefSource::Heard, 11);
    const auto beliefs = set.BeliefsOf(NpcId::New(5));
    WO_CHECK_EQ(static_cast<int64_t>(beliefs.size()), 2);
    return set.Confidence(NpcId::New(5), FactId::New(1)) > 0.8f &&
           set.Confidence(NpcId::New(5), FactId::New(2)) < 0.5f;
}

bool InfrastructureApply() {
    InfrastructureSystem infra;
    infra.AddDoor(DoorState{DoorId::New(1), false, false});
    infra.AddSystem(SystemState{SystemId::New(2), 0, true});
    WO_CHECK(infra.SetDoorOpen(DoorId::New(1), true));
    const DoorState* door = infra.GetDoor(DoorId::New(1));
    WO_CHECK(door != nullptr);
    return door != nullptr && door->open;
}

bool RoomCodecRoundTrip() {
    Room room;
    room.id = RoomId::New(1);
    room.grid = MakeTestGrid();
    room.spawn_point = Vec3{1.5f, 1.5f, 0.0f};
    std::vector<uint8_t> bytes;
    Serializer s(bytes);
    SerializeRoom(s, room);
    Deserializer d(bytes.data(), bytes.size());
    const auto out = DeserializeRoom(d);
    WO_CHECK(out.IsOk());
    return out.IsOk() && out.Value().grid.Width() == 8 &&
           out.Value().grid.Height() == 6;
}

bool MapValidatorCatchesSolid() {
    Room room;
    room.id = RoomId::New(2);
    room.grid = MakeTestGrid();
    room.spawn_point = Vec3{1.5f, 1.5f, 0.0f};
    MapValidator validator;
    const auto issues = validator.Validate(room);
    // Spawn fits, so no errors (only possibly warnings on no-solid? we have solids)
    return validator.HasErrors(issues) == false;
}

} // namespace

void RegisterWorldTests(TestHarness& test) {
    test.Add("grid.collision_blocked", &GridCollision);
    test.Add("grid.floor_ceiling_query", &FloorCeilingQuery);
    test.Add("fact.belief_fact_round_trip", &FactBeliefRoundTrip);
    test.Add("fact.belief_set_ordering", &BeliefSetOrdering);
    test.Add("infrastructure.apply_door", &InfrastructureApply);
    test.Add("room.codec_round_trip", &RoomCodecRoundTrip);
    test.Add("map_validator.spawn_fits", &MapValidatorCatchesSolid);
}

} // namespace writeover