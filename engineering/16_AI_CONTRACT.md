# 16_AI_CONTRACT

## NPC Classes (Frozen)

| Class | Count | Capability |
|-------|-------|-----------|
| Full | 2 | Perception + long-term memory + relationships + Fact/Belief + Utility + GOAP-lite + Storylet |
| Medium | 2 | Perception + FSM + limited memory |
| Light | 3 | FSM patrol/alert/combat/flee |
| Guard | 6–10 | Simplified light FSM, high volume |
| On-screen limit | 8 | — |

## NPC Data

```cpp
enum class NPCClass : uint8_t { Full = 0, Medium = 1, Light = 2, Guard = 3 };

enum class NPCState : uint8_t {
    Idle, Patrol, Alert, Investigate, Combat, Stunned, Flee, Dead,
    Dialogue, Busy, Sleeping, Count
};

struct NPCInstance {
    NpcId id;
    NPCClass npc_class;
    ResourceId data_key;        // content-defined stable id (strong, not const char*)
    Vec3 position;
    float yaw;
    uint16_t health;
    uint8_t alertness;          // 0-100
    uint8_t faction;            // 0=guard, 1=staff, 2=civilian, 3=player
    NPCState state = NPCState::Idle;
    uint32_t state_timer_frames = 0;
    bool is_critical = false;   // main-quest protected
    uint32_t plan_step = 0;
};
```

## Perception

```cpp
struct PerceptionResult {
    bool seesPlayer = false;
    float sightConfidence = 0.0f;
    bool hearsNoise = false;
    Vec3 noisePosition;
    float noiseLoudness = 0.0f;
    uint64_t nearestSoundFrame = 0;
    std::vector<NpcId> visibleTargets;   // other NPCs
};

class PerceptionSystem {
public:
    // Runs at AI subrate (30Hz Medium, 15Hz Light)
    PerceptionResult Update(NPCInstance& npc,
                            const IVisibilityQuery& world, // LineOfSight etc.
                            const PlayerState& player,
                            const std::vector<NoiseSource>& noises,
                            uint32_t simFrame);
};
```

Rules:
- Sight: cone FOV (yaw 120°), range by class (Full 15m, Medium 12m, Light 10m), blocked by solid cells (LineOfSight) and height (eye height vs postures).
- Hearing: `EventWeaponFire` creates a noise source with loudness; NPC within hearingRange and not blocked (distance-based) gets alerted.
- Off-screen NPCs: simulated at 1–2Hz, lightweight (patrol waypoint advance + alert decay), never kill critical NPCs, never trigger irreversible main story events.

## FSM (Medium / Light)

```
Idle ──(timer/player seen)──► Patrol ──(noise/sight)──► Investigate
  ▲                           │  ▲                        │
  └───────────────────────────┘  └──(loses target)────────┘
            Investigate ──(confirms threat)──► Alert ──(fires/moves)──► Combat
            Combat ──(low HP / outgunned)──► Flee
            Any ──(stunner)──► Stunned (timer)
            Any ──(HP 0)──► Dead
```

Transition conditions are simple predicates over PerceptionResult + timers. Deterministic: no rand() in transitions except via sim RNG for investigate destination pick.

## Utility AI (Full NPC)

```cpp
// Two Full NPCs pick a goal from ≤3 candidates by scored utility:
struct UtilityOption {
    const char* goalName;   // e.g. "investigate_noise", "report_to_duty", "engage_player"
    float score;
};

// score components: perceived threat, relationship to player, duty weight, surprise
float ScoreGoal(const char* goal, const PerceptionResult& p, const BeliefSet& b, const NarrativeContext& n);
```

Goals feed into GOAP-lite.

## GOAP-lite (Full NPC)

```cpp
// Constraints (frozen): candidates ≤3, actions ≤10, plan depth ≤5, replan ≤5Hz / on event.

enum class ActionType : uint8_t {
    MoveTo, Investigate, Report, Attack, Reload, TakeCover, OpenDoor,
    UseTerminal, GuardPoint, Communicate, Count
};

struct GOAPAction {
    ActionType type;
    // preconditions as FactPredicate[]  (state checks)
    // effects as FactPredicate[]       (state sets)
    float cost;
    float durationSec;
};

struct GOAPPlanStep {
    GOAPAction action;
    Vec3 targetPos;
};

std::vector<GOAPPlanStep> Plan(const std::vector<FactPredicate>& goals,
                               const FactStore& facts,
                               const std::array<GOAPAction, 10>& actions,
                               int maxDepth = 5);
```

Implementation: simple best-first search over state predicates (NOT a full STRIPS parser — students need to explain it). Replans when: goal changes, perception delta > threshold, plan invalidated by world change, or 5Hz timer.

## Main-Quest Invariant (Frozen)

- NPCs marked `isCritical` cannot die from off-screen simulation, environmental damage, or friendly fire during their scripted window.
- No irreversible main-plot event (npc death, lock-out of a room, ending trigger) can happen off-screen without player intervention. Verified by tests.

## AI Deterministic Scheduling

AI runs at subrates tied to sim frame (frame % N == 0). Order of NPC updates is by `NpcId` ascending — deterministic. All AI random choices use sim RNG.

## AI Tests

1. Perception: sees player around corner? blocked by low wall when crouched? noise heard at range?
2. FSM transition correctness (patrol→alert→combat→flee).
3. GOAP plan validity for 3 sample goals; depth bound.
4. Main-quest invariant: off-screen sim can't kill critical NPC.
5. Belief decay and fact synchronization.

## 报告友好

**STL**: std::vector, std::map (fact predicates), std::optional.
**Design Pattern**: State (FSM), Strategy (utility scoring), Command? (GOAP actions as data).
**Core Algorithm**: Best-first search (GOAP-lite), sensory cone + LOS.
**Course Note**: Full/Medium/Light 是能力层级，不是三个独立系统——它们是同一套感知+FSM 的不同配置。
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
