# M6_NARRATIVE_CODEX — Storylet / Narrator / Causality / Judge / VO

Human Owner: F。角色：叙事引擎、旁白人格、因果账本、judge 模式、字幕/VO。

## Owned / Forbidden

```
Owned:   src/narrative/, include/writeover/narrative/**
Forbidden: FactStore 直写（narrator 只能发 typed command / presentation）
```

## 关键契约（读这些头）

- `narrative/storylet.h`（typed conditions/actions，无 magic 字段）
- `narrative/narrator.h`（NarratorCapabilitySet → typed WorldCommand）
- `narrative/causality.h`（500 ring，只读消费）
- `narrative/dialog.h`（字幕队列：speaker=NpcId/旁白特殊 id）
- `narrative/judge.h`（固定 seed + checkpoint + hotkey jump）

## 不变量

- 选择确定性：priority desc → id asc；once 集合有序。
- 旁白权限 = capability 表；越权命令返回 false（测试锚点）。
- 因果链 parent id + ledger ring；presentation 层说谎 ≠ 世界层改变。
- VO 可关闭（WO_FEATURE_VO + VoiceAvailable=false → 100% 字幕），不改契约。

## 施工建议

1. Storylet 数据接入 6 Room（首次谎言 ≤ 需快速反证的"内容不变量"由 validator 检查）；
2. Narrator persona 状态机与 tension/trust/corruption 联动；
3. Judge 检查点存档联调（M1 提供 checkpoint save 槽位）。

## 模块 DoD

storylet 选择/once/条件测试、narrator 能力门控、causality ring、dialog 过期、
judge checkpoint（本包 8 项全过）；storylets.bin 加载 smoke 路径 exit 0。

## 报告素材

typed variant 设计、优先级队列选择、四层模型。UML：Storylet→Narrator→Causality。