# 15_FACT_BELIEF_CLAIM_PRESENTATION (REPAIR PASS v2)

> 修订（M-010 closure）：FactValue 是 **typed variant（bool | int32 |
> EntityId | RoomId | uint8_t）**——可以表达 64 位 EntityId（旧 `uint16_t
> predicateValue` 做不到）；WorldFact 使用 strong `subject_entity`；
> Fact/Belief/Claim/Presentation 四层保持。实现：`world/fact_belief.h/.cpp`。

## 四层模型（冻结）

```
Presentation — 玩家所见（字幕/UI）：SubtitleLine / HudFrame，可说谎
Claim        — speaker→audience 的一次陈述：ClaimId/underlying fact/veracity
Belief       — 某 agent 对 fact 的信念：NpcId + FactId + confidence + source
WorldFact    — 世界真相：FactId + subject_entity + predicate + FactValue（无文本）
```

## Typed FactValue（实现即合同）

```cpp
using FactValue = std::variant<bool, int32_t, EntityId, RoomId, uint8_t>;
enum class PredicateType : uint8_t { State, Relation, Count };
struct WorldFact {
    FactId id;
    EntityId subject_entity;   // strong，永不裸 uint64
    PredicateType predicate;
    FactValue value;
};
```

- 序列化：type index + 载荷（小端显式），round-trip 测试覆盖含 EntityId 的 Relation。
- FactStore 迭代确定性（std::map 按 FactId 有序），`Snapshot()` 供查询。

## 写者规则（单写者）

- FactStore 只能由 world 权威系统写（事件应用）。
- narrator **永不直接 mutate facts**（M-011 closure）：它只产生 typed
  WorldCommand 或 Presentation；"旁白骗人"只能发生在 Claim/Presentation 层。
- 测试锚点：`narrator.cannot_mutate_facts`（能力门控命令）+ `fact.belief_*`。

## 旁白身份

- Narrator = 特殊 NpcId（`NarratorSpeakerId()` = 0xFFFF…FE），不引入原始 enum
  破坏统一 speaker 语义；persona 是 NarratorState 的属性（Guide/Director/Corrupted）。

## 预算（冻结）

| Item | Budget |
|------|--------|
| Facts | ≤120 |
| Full NPC 活跃 beliefs | ≤25 each |
| Claims | narrator + 2 Full NPC only |
| Causality ledger | 500（ring，narrative/causality.h） |

## 报告友好

**Design Pattern**: 分层模型（真相层不可被 UI 层污染）。
**Course Note**: 四层分离 = "旁白骗得了玩家，骗不了世界数据库"。