# 17_STORYLET_NARRATOR_CONTRACT (REPAIR PASS v2)

> 修订（M-012 closure）：Storylet condition/action 全部 **typed**
> （std::variant），无 magic a/b/f 字段；FrameRange 有显式 min/max（+active 语义）；
> narrator 输出**能力门控的 typed WorldCommand**（不再只有文本层）；因果链由
> `CausalityLedger`（500 ring）在叙事模块维护，事件总线 journal 只读。
> 实现：`narrative/storylet.h/.cpp`、`narrator.h/.cpp`、`causality.h/.cpp`。

## Storylet 数据结构（typed）

```cpp
using StoryletCondition = std::variant<
    FactEqualsCondition, RoomVisitedCondition, NpcStateCondition,
    FrameRangeCondition, DifficultyCondition, FlagCondition>;

using StoryletAction = std::variant<
    NarratorLineAction, DialogAction, WorldCommandAction, EndGameCommand>;
```

- 无 `a/b/f` magic 字段；文本一律 StringId/textId（内容层），不内联 char 数组。
- 选择确定性：`priority desc, storylet id asc`；`once` 用 fired 有序集合。
- 序列化 = 类型索引 + 字段（storylet.bin 编译器输出同构）。

## Narrator 能力（capability-gated commands）

```cpp
struct NarratorCapabilitySet { lock_doors, open_doors, toggle_power, control_comms; };
// TryIssueLockDoor / TryIssueOpenDoor / TryIssueTogglePower(state, out cmd)
// 能力不足 → 返回 false；world 权威系统做最终校验并应用。
```

- 旁白影响世界 = 通过 typed `WorldCommand`（如 CommandSetDoor）；事实真相层只
  能被 world mutation 改。**没有任何绕过路径**（编译期类型限制 + 测试锚点）。

## 因果链（causality）

- `CausalityLedger`：`kCausalityLedgerCapacity=500` ring；每 tick 由叙事模块把
  已派发事件（event_id/parent/sim_frame/kind）写入；只读供 F3/Judge。
- parent id 表达"事件 A 由 B 引起"；F3 面板/测试从 ring 读取，不消费总线。

## 选择循环（每 tick，120Hz 内）

```
OBSERVE phase: SelectEligible(facts, visited, npcs, flags, difficulty, frame)
  → 命中且 once 未触发 → MarkFired → 依次执行 actions：
       NarratorLineAction → SubtitleLine(presentation)
       DialogAction       → DialogueQueue
       WorldCommandAction → PushCommand(下一 tick mutation 阶段应用)
       EndGameCommand     → 结局事件
```

- 每 tick 最多触发一条（优先级+id 全序），保证确定性 & 可测试。

## 测试锚点

`storylet.selects_highest_priority` / `once_fired_skipped` / `condition_false` /
`narrator.capability_gates_command` / `narrator.cannot_mutate_facts` /
`causality.ledger_ring`。

## 报告友好

**Course Note**: Storylet 全部 typed + 确定性排序，Codex 不能发明 magic 字段。