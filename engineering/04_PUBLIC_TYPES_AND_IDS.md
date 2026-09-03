# 04_PUBLIC_TYPES_AND_IDS

## ID Policy

All entity types use **strong typed wrappers** around `uint64_t`. No bare `uint64_t` for identity.

```cpp
// --- Strong ID types ---
// Each wraps a uint64_t. The compiler prevents mixing.
#define WO_DEFINE_ID(Name)                                      \
    struct Name {                                               \
        uint64_t value = 0;                                     \
        bool operator==(const Name& o) const { return value == o.value; } \
        bool operator!=(const Name& o) const { return value != o.value; } \
        bool operator<(const Name& o) const { return value < o.value; }   \
        uint64_t GetValue() const { return value; }             \
        bool IsValid() const { return value != 0; }             \
        static Name Invalid() { return Name{}; }                \
        static Name New(uint64_t v) { return Name{v}; }         \
    };

WO_DEFINE_ID(EntityId)       // Any world entity
WO_DEFINE_ID(EventId)        // WorldEvent
WO_DEFINE_ID(FactId)         // WorldFact
WO_DEFINE_ID(ClaimId)        // Claim
WO_DEFINE_ID(StoryletId)     // Storylet
WO_DEFINE_ID(RoomId)         // Room
WO_DEFINE_ID(ResourceId)     // Resource (audio, texture, etc.)
WO_DEFINE_ID(NpcId)          // NPC (EntityId is the general form)
WO_DEFINE_ID(DoorId)         // Door
WO_DEFINE_ID(SystemId)       // Infrastructure system
WO_DEFINE_ID(WeaponId)       // Weapon
WO_DEFINE_ID(AudioId)        // Audio clip

// Serialization
template<typename Serializer, typename T>
void WriteId(Serializer& s, const T& id) { s.Write(id.GetValue()); }
template<typename Deserializer, typename T>
T ReadId(Deserializer& d) { return T::New(d.Read<uint64_t>()); }
```

## ID Generation / Reuse Policy

- **EntityId**: monotonically increasing counter in WorldSim. 0 = invalid.
- **EventId**: monotonically increasing in EventBus. 0 = invalid.
- **FactId / ClaimId / StoryletId**: from content data files, stable across saves.
- **Generation**: No generation/reuse for this course project. IDs are never reused (monotonic counter suffices for ≤10k entities).
- **Invalid**: `value == 0`.
- **Save/Load**: IDs are persisted as their raw uint64_t value. Stable IDs (Fact, Claim, Storylet, Room, Weapon) are defined by content data, not runtime allocation.

## Basic Shared Types

```cpp
// --- Coordinates ---
// Units: meters. Z up. Yaw/pitch in radians (yaw around Z, pitch from horizontal).
struct Vec2 {
    float x = 0.0f, y = 0.0f;
};
struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

// --- Color (8-bit per channel) ---
struct Color {
    uint8_t r = 255, g = 255, b = 255;
};

// --- Epsilon policy ---
// Position comparison epsilon: 0.001m
// Velocity comparison epsilon: 0.0001 m/s
// Float equality in tests: 1e-4 relative

// --- Grid coordinates ---
// Grid is indexed (col, row). World position = (col + 0.5, row + 0.5) * 1.0m at grid z.
struct GridCoord {
    int32_t col = 0;
    int32_t row = 0;
};

// --- GridCell ---
struct GridCell {
    float floorHeight = 0.0f;      // World Z of floor (meters)
    float ceilingHeight = 4.0f;    // World Z of ceiling (meters)
    uint8_t material = 0;          // Material ID (see material table)
    uint8_t light = 255;           // Light level 0-255
    uint8_t flags = 0;             // CellFlags bitmask
};

enum CellFlags : uint8_t {
    CellFlag_Solid      = 1 << 0,  // Not traversable
    CellFlag_Door       = 1 << 1,  // Is a door
    CellFlag_Breakable  = 1 << 2,  // Can be destroyed
    CellFlag_Special    = 1 << 3,  // Special interaction
};

// --- Material table (indices) ---
// 0 = default wall, 1 = metal, 2 = glass, 3 = dirt,
// 4 = concrete, 5 = wood, 6 = grate, 7 = hazard

// --- AABB ---
struct AABB {
    Vec3 min;
    Vec3 max;
    bool Overlaps(const AABB& o) const {
        return min.x < o.max.x && max.x > o.min.x &&
               min.y < o.max.y && max.y > o.min.y &&
               min.z < o.max.z && max.z > o.min.z;
    }
};

// --- Player dimensions (frozen) ---
constexpr float kEyeStand   = 1.60f;
constexpr float kEyeCrouch  = 1.00f;
constexpr float kEyeProne   = 0.42f;
constexpr float kColliderStand  = 1.80f;
constexpr float kColliderCrouch = 1.20f;
constexpr float kColliderProne  = 0.55f;
constexpr float kLeanOffset = 0.35f;
constexpr float kPlayerRadius = 0.35f;
```

## UUIDs vs IDs

No UUIDs. The course project does not need globally unique IDs. Per-session monotonic counters and content-defined stable IDs suffice.

## 报告友好

**STL**: Strong typed IDs are simple structs; no inheritance, no templates beyond the macro.
**Design Pattern**: Value Object (ID types).
**Course Note**: The strong ID types prevent the classic bug of passing a room ID where a NPC ID is expected.
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
