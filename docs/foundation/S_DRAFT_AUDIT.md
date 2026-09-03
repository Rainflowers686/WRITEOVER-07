# S_DRAFT_AUDIT: Opus S Section Engineering Sketch Audit

> This document audits every item in Opus 5 S.1–S.14 and the 21 issues in `06_Opus_S节草图_必须审计的问题.md`.
> Each item receives: KEEP / MODIFY / REJECT + final replacement contract.
> Priority hierarchy: Course requirements > Opus freeze > P0-v1.0 > Round-3 corrections > Opus S sketches > Fable design.

---

## A. Repository Structure (S.1)

### Verdict: MODIFY

**Issue**: Opus S.1 places `include/` and `src/` as sibling directories. This makes it hard to enforce module boundary at include level.

**Final Contract**:

```
WRITEOVER-07/
├── CMakeLists.txt              # Root CMake
├── CMakePresets.json           # Build presets
├── .editorconfig
├── .clang-format
├── .gitignore
├── .github/workflows/ci.yml
├── cmake/                      # CMake helpers
├── include/writeover/          # PUBLIC headers only
│   ├── common/                 #   types.h, ids.h, world_event.h, rng.h, math.h, debug.h
│   ├── core/                   #   engine.h, save.h, settings.h, profile.h, console.h
│   ├── render/                 #   raycaster.h, terminal_backend.h, hud.h, benchmark.h
│   ├── player/                 #   controller.h, input.h, weapon.h, combat.h
│   ├── world/                  #   grid.h, room.h, puzzle.h, infrastructure.h, map_validator.h
│   ├── ai/                     #   npc.h, goap.h, perception.h, fact_belief.h, memory.h
│   └── narrative/              #   storylet.h, narrator.h, causality.h, judge.h, dialog.h
├── src/                        # Implementation (private)
│   ├── core/
│   ├── render/
│   ├── player/
│   ├── world/
│   ├── ai/
│   ├── narrative/
│   └── platform/windows/       # Win32-specific impl
├── tests/
│   ├── test_core.cpp
│   ├── test_render.cpp
│   ├── test_player.cpp
│   ├── test_world.cpp
│   ├── test_ai.cpp
│   └── test_narrative.cpp
├── tools/
│   ├── mapc/                   # Map validator
│   └── bench/                  # Benchmark runner
├── data/
│   ├── rooms/
│   ├── npcs/
│   ├── facts/
│   └── storylets/
├── scripts/
│   ├── bootstrap.ps1
│   ├── build.ps1
│   ├── test.ps1
│   ├── bench.ps1
│   ├── smoke.ps1
│   └── package.ps1
└── docs/adr/
```

**Rationale**: `include/writeover/` is the single public include directory. Modules include `<writeover/common/types.h>`. No module includes another module's private src. Platform-specific code is isolated in `src/platform/windows/`. Core/world/AI/narrative have zero Win32 includes.

---

## B. Posture Enum (S.2 / Audit Issue #1)

### Verdict: MODIFY

**Issue**: `Posture` mixes orthogonal states: Stand/Crouch/Prone (posture), Jump (traversal), LeanLeft/LeanRight (lean axis).

**Final Contract**:

```cpp
// --- Posture (body height, mutually exclusive) ---
enum class Posture : uint8_t {
    Stand,    // 1.80m collider
    Crouch,   // 1.20m collider
    Prone     // 0.55m collider
};

// --- Traversal (locomotion, transient state) ---
enum class Traversal : uint8_t {
    Grounded,     // On ground, normal movement
    Jump,         // Ascending/descending from jump
    Fall,         // Falling (drop > safe height)
    Vault,        // Vaulting over obstacle
    Climb,        // Climbing ladder
    Mantle        // Pulling up onto platform
};

// --- Lean (independent lateral axis) ---
enum class Lean : uint8_t {
    Center,       // No lean
    Left,         // Leaning left
    Right         // Leaning right
};

// --- Combined player locomotion state ---
struct LocomotionState {
    Posture posture = Posture::Stand;
    Traversal traversal = Traversal::Grounded;
    Lean lean = Lean::Center;

    bool IsGrounded() const { return traversal == Traversal::Grounded; }
    bool CanShoot() const {
        return IsGrounded() && lean == Lean::Center; // simplified
    }
    float ColliderHeight() const;   // Returns height based on posture
    float EyeHeight() const;        // Returns eye height based on posture
};
```

**Rationale**: Orthogonal states prevent combination explosion. "Crouch+Jump" is impossible because Jump is a traversal, not a posture. "Prone+LeanLeft" is expressible. Each axis has its own valid transitions.

---

## C. WorldEvent / Payload (S.2 / Audit Issue #2)

### Verdict: MODIFY

**Issue**: `uint8_t data[32]` is untyped, fragile, unsearchable, unsaveable without magic offsets.

**Final Contract**:

```cpp
// --- Event payload types (variant-based) ---
struct EventWeaponFire {
    uint64_t shooterId;
    uint64_t weaponType;   // WeaponId
    float originX, originY, originZ;
    float yaw, pitch;
};

struct EventDamage {
    uint64_t targetId;
    uint64_t sourceId;
    uint16_t amount;
    uint8_t damageType;    // 0=kinetic, 1=electrical, 2=environmental
};

struct EventDoorChange {
    uint64_t doorId;
    bool open;
};

struct EventPowerToggle {
    uint64_t systemId;
    bool powered;
};

struct EventNPCStateChange {
    uint64_t npcId;
    uint8_t newState;      // NPCState enum value
};

struct EventStoryletTrigger {
    uint16_t storyletId;
    uint64_t triggerEntityId;
};

struct EventFactLearned {
    uint16_t factId;
    uint64_t learnerNpcId;
};

struct EventPlayerDamage {
    uint16_t amount;
    uint8_t damageType;
    uint64_t sourceId;
};

struct EventGameOver {
    uint8_t endingIndex;   // 0-2
};

// --- Typed event union ---
using EventPayload = std::variant<
    EventWeaponFire,
    EventDamage,
    EventDoorChange,
    EventPowerToggle,
    EventNPCStateChange,
    EventStoryletTrigger,
    EventFactLearned,
    EventPlayerDamage,
    EventGameOver
>;

// --- WorldEvent with typed payload ---
struct WorldEvent {
    EventId id;                         // Strong typed ID
    uint64_t simFrame;                  // Sim frame when posted
    EventCategory category;             // Command / Mutation / Notification
    uint64_t sourceEntityId;            // Who originated
    uint64_t targetEntityId;            // Who is affected (0 = none)
    EventPayload payload;               // Typed payload
};

enum class EventCategory : uint8_t {
    Command,        // Request to change world (player action)
    Mutation,       // World state change that happened
    Notification,   // Info that doesn't affect world state
    Query           // Request for read-only information
};
```

**Rationale**:
- `std::variant` provides compile-time type safety, visitor pattern
- Serialization uses variant index + type-specific serialize
- Debug/printable via `std::visit`
- No magic offsets, no memcpy, no alignment bugs
- Save/load is straightforward (index + bytes)

---

## D. Event Dispatch / Fan-out (S.11 / Audit Issue #3, #4)

### Verdict: MODIFY

**Issue**: `EventBus::Poll()` allows consumer to steal events. All modules must use events for everything including queries.

**Final Contract**:

```cpp
// --- EventBus with deterministic fan-out ---
class EventBus {
public:
    // Post an event to the current tick's pending queue
    void Post(WorldEvent&& evt);

    // Process all events for this tick in deterministic order.
    // Each registered consumer sees every event.
    void Dispatch();

    // Register a consumer (module handler) for event processing
    // Consumers are called in registration order.
    using ConsumerId = uint32_t;
    ConsumerId Register(std::function<void(const WorldEvent&)> handler);

    // Unregister
    void Unregister(ConsumerId id);

    // Query journal (read-only, does not consume)
    const CausalityLedger& GetJournal() const;

    // Save/load pending events
    void Save(Serializer& s) const;
    void Load(Deserializer& s);

    // Max event cascade depth (prevents infinite loops)
    static constexpr uint32_t kMaxCascadeDepth = 8;

private:
    struct TickEvents {
        std::vector<WorldEvent> posted;     // Events posted this tick
        std::vector<WorldEvent> dispatched; // Events already dispatched
    };
    std::vector<TickEvents> pending_;
    std::vector<std::function<void(const WorldEvent&)>> consumers_;
    CausalityLedger journal_;
    uint32_t cascadeDepth_ = 0;
};
```

**Semantics**:
- `Post()` adds to current tick's queue
- `Dispatch()` iterates all pending events, sends each to ALL consumers in order
- During dispatch, new `Post()` calls go to next tick's queue (not re-entrant)
- `kMaxCascadeDepth` prevents infinite event loops
- Journal is append-only, never consumed
- Events have parent ID for causality chain
- Queries use explicit read-only interfaces, NOT events

**Query vs Event**:
```cpp
// --- Read-only interfaces (NOT events) ---
class IWorldQuery {
    virtual bool LineOfSight(float x1, float y1, float z1,
                             float x2, float y2, float z2) const = 0;
    virtual GridCell GetCell(int x, int y) const = 0;
    virtual AABB GetPostureBox(Posture p, float x, float y, float z) const = 0;
};

class IRenderSnapshot {
    virtual PlayerState GetPlayerState() const = 0;
    virtual const GridCell* GetGrid(int& w, int& h) const = 0;
};
```

---

## E. Fact/Belief/Claim/Presentation (S.2 / Audit Issue #5)

### Verdict: MODIFY

**Issue**: Opus `Fact { claimId, sourceNpcId, text, isTruth }` conflates world facts, beliefs, claims, and presentation text.

**Final Contract**:

```cpp
// --- WorldFact: objective truth about the world state ---
// Semantic data without text binding.  E.g., "Door#7 is open"
struct WorldFact {
    FactId id;
    uint64_t subjectEntityId;       // What entity this fact is about
    uint8_t predicateType;          // 0=state, 1=relation, 2=event
    uint16_t predicateValue;        // Type-specific value
    // No text field — text belongs to narrative layer
};

// --- Belief: an agent's belief about a fact ---
struct Belief {
    FactId factId;
    uint64_t npcId;                 // Who holds this belief
    float confidence;               // 0.0–1.0
    uint32_t lastUpdatedFrame;      // Sim frame of last update
    uint8_t sourceType;             // 0=direct observation, 1=comm, 2=inference
};

// --- Claim: a statement made by one entity to another ---
// Can be true, false, misleading, or distorted
struct Claim {
    ClaimId id;
    FactId underlyingFactId;        // The fact this claim refers to
    uint64_t speakerId;             // Who made the claim
    uint64_t audienceId;            // Intended recipient (0 = all)
    uint8_t veracity;               // 0=true, 1=false, 2=misleading, 3=distorted
    // No text field — text is presentation layer
};

// --- Presentation: what the UI/narrator shows to the player ---
struct Presentation {
    PresentationId id;
    FactId factId;                  // Optional: underlying fact
    ClaimId claimId;                // Optional: source claim
    std::string displayedText;      // What the player sees (UTF-8)
    uint8_t narratorFilter;         // 0=raw, 1=guide, 2=director, 3=corrupted
    uint8_t reliability;            // 0=truth, 1=biased, 2=fabricated
};
```

**Rationale**:
- Four-layer separation: WorldFact (semantic truth) → Belief (agent's internal) → Claim (communication) → Presentation (UI text)
- Text is content/narrative layer, not semantic data
- Enables "narrator shows false text but underlying fact is true"
- Enables "NPC believes false claim because they were deceived"
- Enables "player sees distorted text due to corrupted narrator"
- Use `std::string` instead of `char[128]` — text is not fixed-size; serialization handles variable length

---

## F. Update(float dt) / Simulation Clock (S.3, S.5 / Audit Issue #6, #7)

### Verdict: MODIFY

**Issue**: `Update(float dt)` on every module implies variable timestep. Fixed 120Hz simulation must be explicit.

**Final Contract**:

```cpp
// --- Simulation clock (fixed 120Hz) ---
class SimClock {
public:
    static constexpr float kFixedDeltaTime = 1.0f / 120.0f;
    static constexpr uint64_t kSimHz = 120;

    SimClock() = default;

    uint64_t frameCount() const { return frameCount_; }
    float currentTime() const { return frameCount_ * kFixedDeltaTime; }
    float deltaTime() const { return kFixedDeltaTime; }

    void Tick() { ++frameCount_; }

    // Save/load
    void Save(Serializer& s) const { s.Write(frameCount_); }
    void Load(Deserializer& s) { frameCount_ = s.Read<uint64_t>(); }

private:
    uint64_t frameCount_ = 0;
};

// --- Module interface (fixed-step, not float dt) ---
class IModule {
public:
    virtual ~IModule() = default;
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    virtual void SimTick(const SimClock& clock) = 0;  // Called at 120Hz
    virtual const char* Name() const = 0;
};

// --- AI sub-rate scheduling ---
// AI systems run at reduced rate:
//   Full NPC: 60Hz (every 2nd sim tick)
//   Medium NPC: 30Hz (every 4th sim tick)
//   Light NPC: 15Hz (every 8th sim tick)
//   Off-screen: 1-2Hz
// This is enforced by the AI module, not by the clock.
```

**Rationale**:
- `SimTick()` with no `dt` parameter — rate is always 120Hz
- AI sub-rate is the AI module's responsibility, not the engine's
- Render interpolation reads `SimClock.frameCount()` to interpolate between snapshots
- Wall clock is only used for performance measurement and non-deterministic UI

---

## G. Deterministic RNG (S.5 / Audit Issue #7)

### Verdict: MODIFY

**Issue**: Opus sketch saves only seed. Full state must be saved for full determinism.

**Final Contract**:

```cpp
// --- XorShift128+ RNG (full state) ---
class DeterministicRNG {
public:
    DeterministicRNG() = default;
    explicit DeterministicRNG(uint64_t seed) { Seed(seed); }

    // Core: generate next uint64
    uint64_t Next();

    // Convenience wrappers
    uint32_t NextU32() { return static_cast<uint32_t>(Next()); }
    float NextFloat() { return (Next() >> 11) * 0x1.0p-53f; }
    int NextInt(int min, int max);  // Inclusive

    // Full state save/load
    void Save(Serializer& s) const;
    void Load(Deserializer& s);

    // Seed reset (also resets full state)
    void Seed(uint64_t seed);

    // Access current state for debug
    uint64_t GetState0() const { return state0_; }
    uint64_t GetState1() const { return state1_; }

private:
    uint64_t state0_ = 0;
    uint64_t state1_ = 0;
};

// --- RNG ownership ---
// WorldSim owns one "simulation RNG" that drives all deterministic decisions.
// Render module owns a separate "visual RNG" for non-deterministic effects.
// Visual RNG is NOT saved/loaded.
```

**Rationale**:
- XorShift128+ is fast, well-understood, deterministic
- Full 128-bit state saved, not just seed
- Two RNGs: sim (deterministic, saved) + visual (non-deterministic, not saved)
- Determinism guarantee: same build + same seed + same input sequence = same output

---

## H. Save Serialization (S.6 / Audit Issue #8)

### Verdict: REJECT

**Issue**: Opus sketch `SaveHeader { magic(4), version(4), timestamp(8), checksum(4), seed(8) }` sums to 28 bytes, not 32. Padding is compiler-dependent. Raw struct dump is forbidden.

**Final Contract**:

```cpp
// --- Explicit binary save format ---
// File layout:
//   [FileHeader]    - 32 bytes, fixed layout
//   [SectionHeader] - 12 bytes per section
//   [SectionData]   - section payload
//   [Footer]        - 8 bytes checksum

#pragma pack(push, 1)
struct FileHeader {
    uint32_t magic = 0x574F3037;          // "WO07"
    uint32_t version = 1;                 // Schema version
    uint64_t timestamp;                    // Unix timestamp
    uint32_t sectionCount;                 // Number of sections
    uint32_t reserved1 = 0;               // Reserved for future use
    uint64_t reserved2 = 0;               // Reserved for future use
    // Total: 32 bytes
};
static_assert(sizeof(FileHeader) == 32, "FileHeader must be 32 bytes");

struct SectionHeader {
    uint32_t sectionId;                   // 0=player, 1=world, 2=rng, 3=profile, 4=events, 5=ai, 6=narrative
    uint32_t dataSize;                    // Bytes of section data
    uint32_t checksum;                    // CRC32 of section data
    // Total: 12 bytes
};
#pragma pack(pop)
```

**Serialization policy**:
- No `reinterpret_cast` for struct dumps
- Every type has explicit `Serialize(Serializer&)` / `Deserialize(Deserializer&)`
- Serializer handles endianness (little-endian on x64, but explicit)
- `CRC32` checksum per section + full file checksum
- Atomic write: write to `.tmp`, then `MoveFileEx` replace
- Corrupted files: checksum mismatch → reject with error message
- Old version: version mismatch → reject with upgrade message
- Profile and world save are **combined** in one file (simpler for course project)

---

## I. Span / Raycaster Contract (S.7 / Audit Issue #10, #11)

### Verdict: MODIFY

**Issue**: `Span spans[16]` is arbitrary fixed capacity. `startY/endY` mixes world-space and screen-space.

**Final Contract**:

```cpp
// --- Height-span raycaster ---

// World-space vertical interval (before projection)
struct VerticalInterval {
    float bottomZ;          // World Z (up) of bottom
    float topZ;             // World Z (up) of top
    float distance;         // Distance from camera along ray
    uint8_t material;       // Material ID
    uint8_t light;          // Light level 0-255
};

// Ray configuration
struct RayConfig {
    float originX, originY, originZ;  // World position
    float yaw, pitch;                  // Direction (radians)
    float maxDistance = 50.0f;         // Max ray distance
    uint8_t maxIntervals = 32;         // Max vertical intervals (was 16)
};

// Ray result
struct RayResult {
    VerticalInterval intervals[32];    // World-space intervals
    uint8_t intervalCount = 0;         // Actual count
    bool hitSomething = false;         // Did we hit anything?
    float closestHitDistance = 0.0f;   // Nearest hit distance
};

// Public API
RayResult CastRay(const RayConfig& config,
                  const GridCell* grid, int gridW, int gridH);

// --- Projection control (separate from raycast) ---
// The raycaster returns world-space intervals.
// The renderer projects them to screen-space based on FOV and resolution.
// This makes the raycaster unit-testable independent of display.
```

**Rationale**:
- `maxIntervals = 32` based on worst-case analysis: 1 wall span + 1 floor span + 1 ceiling span per cell, ~10 cells deep = 30 intervals max
- Overflow: if exceeded, ray is terminated and intervalCount is set to 32 with a warning logged
- World-space intervals: testable without FOV/resolution dependency
- Projection is a separate concern in the renderer

---

## J. Input Contract (S.9 / Audit Issue #12)

### Verdict: MODIFY

**Issue**: `bool keys[256]` is too low-level. Public gameplay API should not know Win32 scancodes.

**Final Contract**:

```cpp
// --- Physical input events (platform-agnostic) ---
enum class PhysicalKey : uint16_t {
    // Keyboard
    W, A, S, D, Q, E, R, F, Z, X, C, V, B, N, M,
    Space, Shift, Ctrl, Tab, Escape,
    Up, Down, Left, Right,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    // Mouse
    MouseLeft, MouseRight, MouseMiddle,
    MouseX1, MouseX2,
    // Gamepad
    GamepadDPadUp, GamepadDPadDown, GamepadDPadLeft, GamepadDPadRight,
    GamepadA, GamepadB, GamepadX, GamepadY,
    GamepadLB, GamepadRB, GamepadLT, GamepadRT,
    GamepadStart, GamepadBack,
    GamepadLStick, GamepadRStick,
    // Special
    Count,  // NOT a key, used for array sizing
    Unknown = 0xFFFF
};

// --- Game actions (rebindable, what gameplay code uses) ---
enum class GameAction : uint8_t {
    MoveForward, MoveBackward, MoveLeft, MoveRight,
    Sprint, Jump, Crouch, Prone,
    LeanLeft, LeanRight, Interact, Reload,
    Fire, AimDownSights, Melee,
    WeaponSlot1, WeaponSlot2, WeaponSlot3,
    Pause, DevPanel, Help,
    DialogOption1, DialogOption2, DialogOption3, DialogOption4,
    Count
};

// --- Input event (raw, per-frame) ---
struct InputEvent {
    PhysicalKey key;
    bool pressed;           // true = down this frame, false = up this frame
    float analogValue;      // 0.0-1.0 for analog inputs
};

// --- Input state (what gameplay code reads) ---
struct InputState {
    // Mouse delta (raw, unbounded)
    float mouseDX = 0.0f;
    float mouseDY = 0.0f;

    // Gamepad analog
    float leftStickX = 0.0f, leftStickY = 0.0f;
    float rightStickX = 0.0f, rightStickY = 0.0f;
    float leftTrigger = 0.0f, rightTrigger = 0.0f;

    // Action states (resolved from physical keys via rebind map)
    bool actions[static_cast<size_t>(GameAction::Count)] = {};
    bool actionsPressed[static_cast<size_t>(GameAction::Count)] = {};
    bool actionsReleased[static_cast<size_t>(GameAction::Count)] = {};

    // Focus state
    bool hasFocus = true;
    bool imeComposing = false;
};

// --- Input backend interface ---
class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    virtual bool Init() = 0;
    virtual void Shutdown() = 0;
    virtual bool Poll(InputEvent& outEvent) = 0;  // Non-blocking
    virtual const char* Name() const = 0;
    virtual bool HasFocus() const = 0;
};

// --- Input mapper (PhysicalKey → GameAction, handles rebinding) ---
class InputMapper {
public:
    void SetBinding(GameAction action, PhysicalKey key);
    PhysicalKey GetBinding(GameAction action) const;
    GameAction MapKey(PhysicalKey key) const;
    void ResetToDefaults();
    void Save(Serializer& s) const;
    void Load(Deserializer& s);
};
```

**Rationale**:
- Gameplay code uses `GameAction`, not `PhysicalKey`
- Rebinding is a mapping layer between them
- InputEvent is the raw stream; InputState is the resolved per-frame state
- Fallback: RawInput → CursorDelta → KeyboardOnly (all implement IInputBackend)
- Focus lost clears all action states

---

## K. Terminal / CharCell (S.10 / Audit Issue #9)

### Verdict: MODIFY

**Issue**: `wchar_t` is 16-bit on Windows. Unicode policy must be explicit.

**Final Contract**:

```cpp
// --- Character cell ---
struct CharCell {
    char32_t codePoint;     // Unicode code point (UTF-32)
    uint8_t fgR, fgG, fgB; // Foreground color (0-255)
    uint8_t bgR, bgG, bgB; // Background color (0-255)
    uint8_t flags;          // 0x01=bold, 0x02=blink, 0x04=underline
};

// --- Terminal backend interface ---
struct TerminalCaps {
    bool trueColor;         // 24-bit color support
    bool ansiSequence;      // ANSI escape sequence support
    bool win32Native;       // WriteConsoleOutputW support
    int maxWidth, maxHeight;
    float charAspectRatio;  // Width/height of character cell
};

class ITerminalBackend {
public:
    virtual ~ITerminalBackend() = default;
    virtual bool Init(int width, int height) = 0;
    virtual void Shutdown() = 0;
    virtual bool Submit(const CharCell* buffer, int width, int height) = 0;
    virtual void Restore() = 0;  // Restore terminal on crash/exit
    virtual TerminalCaps GetCaps() const = 0;
    virtual const char* Name() const = 0;
};

// --- Unicode policy ---
// 3D layer: ONLY single-width characters (U+0020-U+00FF, U+2580-U+259F block chars)
// CJK layer: HUD, subtitles, menu — uses Unicode-aware width detection
// Internal: char32_t for code points, convert to UTF-8 for storage, UTF-16 for Win32
```

**Rationale**:
- `char32_t` avoids UTF-16 surrogate pair issues
- 3D layer restricted to single-width characters for deterministic character grid
- CJK rendered in separate compositor layer with width detection
- Terminal backend handles encoding conversion to ANSI/Win32

---

## L. Test Harness (S.13 / Audit Issue #13)

### Verdict: MODIFY

**Issue**: Opus `TestHarness` is an abstract class with pure virtual methods, then instantiated directly. Not compilable.

**Final Contract**:

```cpp
// --- Minimal concrete test harness ---
// No external dependencies. Single header.
// Integrated with CTest via exit code.

class TestHarness {
public:
    struct TestCase {
        const char* name;
        bool (*fn)();
    };

    void Register(const char* name, bool (*fn)()) {
        tests_.push_back({name, fn});
    }

    int RunAll() {
        int passed = 0, failed = 0;
        for (auto& t : tests_) {
            printf("  TEST %s ... ", t.name);
            bool ok = t.fn();
            if (ok) {
                printf("PASS\n");
                ++passed;
            } else {
                printf("FAIL\n");
                ++failed;
            }
        }
        printf("\n%d/%d passed, %d failed\n", passed, passed + failed, failed);
        return failed;
    }

    // Assertions
    static void Assert(bool condition, const char* msg) {
        if (!condition) {
            printf("    ASSERTION FAILED: %s\n", msg);
        }
    }

    static void AssertEq(int a, int b, const char* msg) {
        if (a != b) {
            printf("    ASSERTION FAILED: %s (expected %d, got %d)\n", msg, b, a);
        }
    }

    static void AssertEq(float a, float b, float eps, const char* msg) {
        if (fabsf(a - b) > eps) {
            printf("    ASSERTION FAILED: %s (expected %f, got %f, eps %f)\n", msg, b, a, eps);
        }
    }

private:
    std::vector<TestCase> tests_;
};

// Convenience macro
#define REGISTER_TEST(harness, name) harness.Register(#name, name)
```

**Rationale**:
- Concrete, compilable class
- No external dependencies (no Google Test, no Catch2)
- Returns failure count as exit code for CTest integration
- Simple printf output — no need for fancy formatting
- Header-only for easy inclusion

---

## M. Module Boundaries (S.3 / Audit Issue #14, #15)

### Verdict: MODIFY

**Issue**: "No new/delete" is too vague. "No global mutable state" needs an alternative.

**Final Contract**:

```cpp
// --- Ownership policy ---
// 1. Prefer value types (stack/container)
// 2. Use std::unique_ptr for dynamic ownership
// 3. Use std::shared_ptr only when shared ownership is provably necessary
// 4. Raw pointers are ONLY for non-owning observer/reference
// 5. No owning raw pointers (delete is forbidden)
// 6. No global mutable state (use EngineContext for cross-cutting concerns)

// --- EngineContext (the ONE dependency injection point) ---
// This is NOT a service locator.
// It is the explicit dependency graph root.
struct EngineContext {
    SimClock* clock = nullptr;
    EventBus* eventBus = nullptr;
    DeterministicRNG* simRng = nullptr;
    IWorldQuery* world = nullptr;
    Settings* settings = nullptr;
    DebugMetrics* metrics = nullptr;
    IInputBackend* input = nullptr;
    InputMapper* inputMapper = nullptr;
    ITerminalBackend* terminal = nullptr;
};

// Every module Init() receives EngineContext:
//   void Init(const EngineContext& ctx);
// Modules read from ctx, do not mutate it.
```

**Rationale**:
- `EngineContext` is explicit dependency injection, not a service locator
- All cross-cutting concerns (logging, metrics, RNG, clock) are passed explicitly
- No global singletons
- Module `Init()` takes the context; modules don't search for dependencies

---

## N. Build / Toolchain (S.4 / Audit Issue #17, #18)

### Verdict: KEEP

**Final Contract**:

```text
CMake:            3.20+
Toolset:          Visual Studio 2022 (v143)
C++ Standard:     C++17
Warnings:         /W4 /WX (warnings as errors)
Unicode:          /utf-8 (source and execution character set)
Debug:            /MDd (dynamic debug runtime)
Release:          /MT (static runtime), /O2
Precompiled:      No (keep it simple)
Third-party:      Zero runtime dependencies
                  Development-only: none (use self-contained test harness)
CI:               GitHub Actions (windows-latest)
```

---

## O. Error Handling (Audit Issue #20)

### Verdict: ADD (not in Opus S section, but required)

**Final Contract**:

```cpp
// --- Error handling policy ---

// 1. Programmer errors → assert (fail fast, debug)
// 2. Recoverable runtime errors → Result<T> type
// 3. Corrupted data → fail closed (refuse to load)
// 4. User input errors → graceful feedback, no crash
// 5. No exceptions (C++17, no try/catch/throw)
// 6. No silent catch (...) {}

// --- Result<T> type ---
template<typename T>
class Result {
public:
    static Result Ok(T value) { return Result(std::move(value)); }
    static Result Error(const char* msg) { return Result(msg); }

    bool IsOk() const { return ok_; }
    bool IsError() const { return !ok_; }
    const T& Value() const { assert(ok_); return value_; }
    const char* ErrorMessage() const { assert(!ok_); return error_; }

private:
    bool ok_;
    T value_;
    const char* error_;
};

// --- Logging ---
enum class LogLevel : uint8_t {
    Debug, Info, Warning, Error, Fatal
};

// Logger is accessible via EngineContext.metrics (DebugMetrics interface)
// Not a global variable.
```

---

## P. Threading (Audit Issue #16)

### Verdict: KEEP (with clarification)

**Final Contract**:

```text
R0-P0 Threading Model:
- Simulation: single thread, always
- Render: single thread, same as Sim (R0)
- Present: single thread, same as Sim (R0)
- Audio: separate thread for playback only (Windows wasapi/mme)
- File I/O: synchronous, on sim thread (save/load are infrequent)

After Profiler Proof (9/8+):
- If Present blocks >1ms, add Present thread with latest-frame mailbox
- If Raycaster takes >4ms, consider column-parallel render
- World logic NEVER goes multi-threaded

Policy: Profile first, then parallelize.
```

---

## Q. Feature Capability / Fallback (Audit Issue #17, #18)

### Verdict: KEEP

**Final Contract**:

```cpp
// --- Feature flags (compile-time, set in CMake) ---
#define WO_FEATURE_VO             1
#define WO_FEATURE_GOAP           1
#define WO_FEATURE_PRONE          1
#define WO_FEATURE_VAULT          1
#define WO_FEATURE_MANTLE         1
#define WO_FEATURE_GAMEPAD        1
#define WO_FEATURE_PRESENT_THREAD 0  // Enabled after profiler proof

// --- Runtime capability flags ---
struct RuntimeCaps {
    bool rawInput = false;
    bool trueColor = false;
    bool ansiEscape = false;
    bool audioDevice = false;
    int terminalWidth = 0;
    int terminalHeight = 0;
    float charAspect = 0.5f;
};
```

**Rationale**:
- Fallbacks change implementation, not public API
- `InputBackend::Name()` tells which fallback is active
- `TerminalCaps` tells which render backend is used
- Feature flags disable capability at compile time without changing contracts

---

## R. Content Schema (Audit Issue #19, S.2)

### Verdict: MODIFY

**Issue**: `char text[128]` and `char text[512]` are fixed-size. Content should use semantic IDs, not inline text.

**Final Contract**:

```cpp
// --- Content data uses semantic IDs ---
// Storylet has a textId that maps to a localization string table.
// Facts have no text field.
// NPCs have no inline dialogue text.

// Loading:
// 1. Parse all content files (rooms, npcs, storylets, facts, claims)
// 2. Validate all references (no dangling IDs)
// 3. Fail fast on missing references
// 4. Load in deterministic order (sorted by ID)
// 5. Warnings for duplicate IDs (first wins, log warning)
```

---

## S. No-Go Patterns (S.12)

### Verdict: KEEP (with additions)

```text
ADDITIONAL forbidden patterns:
- std::rand() — use DeterministicRNG
- system("cls") — use terminal backend
- std::cout per-character — use double-buffered CharCell
- std::this_thread::sleep_for in main loop — use accumulator
- owning raw pointer — use unique_ptr or value
- global mutable state — use EngineContext
- dynamic_cast — use virtual dispatch or variant
- catch(...){} — no silent errors
- reinterpret_cast for serialization — use explicit serialize
- wchar_t for glyph storage — use char32_t
- Two EntityId implementations — use the one from common/ids.h
- Two Vec libraries — use the one from common/math.h
- Hidden network code — zero network code
- SDL/DirectX window — terminal only
- Runtime LLM — pre-generated content only
```

---

## T. Module Prompts (S.14)

### Verdict: KEEP

**Final Contract**: See `codex/M1_CORE_CODEX.md` through `codex/M6_NARRATIVE_CODEX.md`.

---

## U. Wchar_t / CJK (Audit Issue #9)

### Verdict: MODIFY

**Final Contract**: See Terminal/CharCell section above. Summary:
- `char32_t` for internal glyph representation
- 3D layer: single-width characters only (U+0020–U+00FF, U+2580–U+259F)
- CJK: compositor layer with Unicode width detection
- ANSI output: UTF-8 encoded
- Win32 output: UTF-16 (converted from char32_t)

---

## V. RNG Seed vs State (S.5 / Audit Issue #7)

### Verdict: MODIFY

**Final Contract**: See DeterministicRNG above. Full 128-bit state is saved/loaded.

---

## Summary: All 21 Audit Items

| # | Item | Verdict |
|---|------|---------|
| 1 | Posture/Traversal/Lean mix | **MODIFY** → Orthogonal states |
| 2 | uint8_t data[32] untyped | **MODIFY** → std::variant typed payload |
| 3 | "All events" too absolute | **MODIFY** → Query/View/Snapshot interfaces |
| 4 | EventBus::Poll() steals | **MODIFY** → Fan-out dispatch + journal |
| 5 | Fact/Claim/text mixed | **MODIFY** → 4-layer separation |
| 6 | Update(float dt) | **MODIFY** → SimTick(SimClock) fixed 120Hz |
| 7 | RNG seed ≠ state | **MODIFY** → Full 128-bit state save/load |
| 8 | Struct dump save | **REJECT** → Explicit binary format |
| 9 | wchar_t glyph contract | **MODIFY** → char32_t + Unicode policy |
| 10 | Span[16] arbitrary | **MODIFY** → 32 intervals, worst-case analysis |
| 11 | Span startY/endY mixed | **MODIFY** → World-space VerticalInterval |
| 12 | keys[256] too low-level | **MODIFY** → PhysicalKey / GameAction separation |
| 13 | TestHarness abstract | **MODIFY** → Concrete, compilable harness |
| 14 | "No new/delete" vague | **MODIFY** → Ownership policy + EngineContext |
| 15 | No global state alternative | **MODIFY** → EngineContext DI |
| 16 | sleep_for ban | **KEEP** → With accumulator clarification |
| 17 | /MT architecture coupling | **KEEP** → /MT only Release, /MDd Debug |
| 18 | W4/WX + third-party | **KEEP** → Third-party = 0 |
| 19 | char[128] content | **MODIFY** → Semantic IDs, variable text |
| 20 | Error handling missing | **ADD** → Result<T> + logging policy |
| 21 | Public contract drift | **KEEP** → AGENTS.md + CI check |
---

# REPAIR PASS v2 — AUDIT RE-ENTRY (FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE)

> 本轮不是重做 S 节审计，而是对上一轮“审计结论未落地”的收口：
> 上表 21 项 MODIFY/REJECT 的决定均已**落到真实代码与测试**，否则降级记录如下。

## 落地确认（抽样关键项 → 证据）

| # | 决定 | 落地证据 |
|---|------|----------|
| 1 | Posture/Traversal/Lean 正交 | common/player_types.h + controller 单测 |
| 2 | typed payload | common/world_event.h variant + round-trip 测试 |
| 3 | Command/Event/Query/Presentation 分离 | command.h/world_event.h/IWorldQuery + check_deps |
| 4 | fan-out 无 stealing | EventBus Dispatch 单点 + 事件单测 |
| 5 | Fact/Belief/Claim/Presentation 四层 | fact_belief.h + 四层模型单测 |
| 6 | 无 Update(dt) | IEngineModule::SimTick(const SimClock&) |
| 7 | RNG full state | rng.h Save/Load + state round-trip 测试 |
| 8 | 显式二进制存档 | save.h 逐字段 + corrupt/bit-flip 测试 |
| 10 | 128 段容量不变量 | raycaster.h kMaxSegmentsPerRay=128 + 注释推导 |
| 11 | 世界 Z 区间 | OccludingSegment bottom/top 世界坐标 + 黄金测试 |
| 12 | PhysicalKey/GameAction 分离 | common/input_types.h + input_mapper 测试 |
| 13 | harness 可编译 | tests/ 真实 50 测试 0 failed |
| 14-15 | EngineContext DI | engine.h EngineContext；无 service locator |
| 17 | /MT 只 Release | presets CMAKE_MSVC_RUNTIME_LIBRARY |
| 19 | 文本=StringId | StringId 用于文本键；无 char[128] 内容内嵌 |
| 20 | Result<T> | result.h std::variant<T,ErrorInfo> 支持非默认构造 |
| 21 | 公共契约 drift | check_public_headers hash baseline 生效 |

## 变更管理

9/3 冻结窗口内仅允许：修正确性 / 补测试 / 文档与代码对齐 / 六 Owner sign-off。
需要改上表任何决定 → ADR + Owner + Opus 复审。
