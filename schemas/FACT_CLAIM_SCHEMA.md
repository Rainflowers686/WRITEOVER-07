# FACT_CLAIM_SCHEMA — Authoring JSON (schemaVersion 1)

作者 JSON（编译期校验/登记）：

```json
{
  "schemaVersion": 1,
  "facts": [
    {"id": "fact_player_has_gun", "predicate": 0, "initial": false, "subject": 1}
  ]
}
```

| 字段 | 规则 |
|------|------|
| id | 同 kind 唯一；被 storylet/npc 引用必须可解析 |
| predicate | 0=State / 1=Relation / 2=Count（默认 0） |
| initial | 初始布尔值（State） |
| subject | 内容层 subject 序号（编译期→EntityId） |

## 运行时类型（world/fact_belief.h）

```cpp
using FactValue = std::variant<bool, int32_t, EntityId, RoomId, uint8_t>;
struct WorldFact { FactId id; EntityId subject_entity;
                   PredicateType predicate; FactValue value; };
```

- FactValue 支持 64 位 EntityId（Relation 事实可指向实体）。
- Fact 无文本——文本属于 Claim/Presentation 层（四层分离）。
- 写者：world 权威系统唯一；narrator 只发 typed command（15/17 文档）。

## Claim（运行时，不来自作者 JSON）

```cpp
struct Claim { ClaimId id; FactId underlying_fact_id;
               NpcId speaker_id; NpcId audience_id; Veracity veracity; };
enum class Veracity { True, False, Misleading, Distorted };
```

- NarratorSpeakerId() = 0xFFFF…FE 统一 speaker 语义。

## facts.bin（validation artifact）

contentc 把 facts.json 编译为 `data/facts/facts.bin`（校验快照 + 未来运行时
FactStore 播种的输入）；foundation 阶段 FactStore 由运行期事件/测试驱动。

## 预算

Facts ≤120；Full NPC active beliefs ≤25 each；Claims = narrator + 2 Full NPC。