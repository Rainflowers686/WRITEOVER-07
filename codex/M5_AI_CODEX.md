# M5_AI_CODEX — NPC / AI / Belief

Human Owner: E。角色：FSM/perception/GOAP-lite/memory/belief 消费。

## Owned / Forbidden

```
Owned:   src/ai/, include/writeover/ai/**
Forbidden: world 状态写者（只读 IWorldQuery/FactStore）；core/render 实现
```

## 关键契约（读这些头）

- `ai/npc.h`（NPCInstance：ResourceId data_key，NPCState enum）
- `ai/perception.h`（感知：IWorldQuery LOS + 噪声）
- `ai/goap.h`（≤10 actions / depth≤5 / best-first 确定性）
- `ai/memory.h`（有序、上限 64）
- `world/fact_belief.h`（BeliefSet：per-NPC belief，写只允许 Upsert 经事件流）

## 不变量

- 感知/FSM/GOAP 全部确定性：NPC 更新按 NpcId 升序、AI 随机只走 sim RNG。
- NPC 不直接写 FactStore：行为结果通过 WorldCommand/事件 → world 应用。
- is_critical NPC 不可离屏死亡/环境死亡（main-quest 不变量，有测试）。
- AI 频率：全量重规划 ≤5Hz、Full 感知 30–60Hz、Light 15Hz——按 frame 取模。

## 施工建议

1. Full NPC 感知→belief→storylet 联动（与 M6 集成）；
2. 守卫巡逻/警戒/战斗 FSM 按房间数据接入；
3. GOAP 目标来自 BeliefSet 评估（当前单测已覆盖计划器正确性）。

## 模块 DoD

perception 三种用例、FSM 迁移、GOAP 计划/无解、belief 衰减、critical 不变量
（本包 6 项全过 + main-quest invariant 测试待 M5 补全）。

## 报告素材

感知锥+LOS、best-first（GOAP-lite）、FSM。UML：NPC→Perception→GOAP→Belief。