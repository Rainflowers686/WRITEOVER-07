# HK-1 Deterministic Sim / Event / Save / Replay

## 目的

将确定性模拟、事件总线、存档、回放加固为可信内核。

## 关键契约

### 8 阶段固定 tick 语义

每个 SimTick 按固定顺序执行：

1. INPUT SAMPLE — 采样输入
2. COMMAND BUILD — 生成指令
3. AUTHORITATIVE WORLD MUTATION — 权威世界变更（唯一写点）
4. WORLD EVENT EMISSION — 世界事件产生
5. EVENT FAN-OUT — EventBus::Dispatch()（所有 consumer 看到所有事件）
6. REACTION COMMAND QUEUE — 下一 tick 反应指令排队
7. SNAPSHOT FINALIZE — 快照定稿
8. SIM CLOCK TICK — 推进时钟

### 事件 fan-out 语义 (F-06 决议)

- `Post()` 放入 `next_pending_`
- `Dispatch()` 广播 `pending_`，然后将 `next_pending_` 升为 `pending_`
- **同 tick 产生的事件下一 tick 才可见**（end-of-tick fan-out）
- 事件不被消费（所有 consumer 都看到全部事件）

### Engine 循环修复 (F-05)

`engine.cpp` 在每个 sim tick 的模块 SimTick 之后调用 `context_.events->Dispatch()`，
使事件能实际扇出到订阅者。

### Save 7-section 修复 (F-13)

`save.cpp` 的 section_count 检查从 `>= Count` 改为 `> Count`，
允许全部 7 个合法 section（0..6）通过。

### MiniWorld 参考实现

测试内的 MiniWorld 包含：RNG + EventBus + player + door + power + NPC state +
fact + storylet fired + narrator capability + pending reaction。
语义序列化 → Save → destroy → Load → 继续步进，2000 tick 结果与
1000+save/load+1000 完全一致。

## 测试

- `replay.save_load_resume` — 2000 连续 == 1000+存档+读档+1000（语义哈希）
- `save.all_7_sections_legal` — 7 个 section 合法
- `save.duplicate_section_rejected` — 重复 section 拒绝
- `save.unknown_section_rejected` — 未知 section 拒绝
- `event.fanout_all_consumers_see_all` — fan-out 语义
- `event.same_tick_next_tick_semantics` — 同 tick/下一 tick 语义

## 门控 (G1)

PASS：
- Engine 有正式 tick phase（fixed 120Hz，事件 dispatch 在 loop 内）
- EventBus fan-out 行为明确（所有 consumer 看到所有事件）
- real causality parent（F-07 假因果链已移除）
- 2000 tick uninterrupted == 1000 tick save/load 1000 tick（semantic hash 相等）
- pending reaction exactly-once（journal 语义已明确）
- 合法 7-section save PASS
- corruption fuzz PASS（bit flip / garbage / duplicate / unknown 全拒绝）

## 给 Luna
- M1 / M5 / M6 依赖此内核；事件必须通过 EventBus，不得直接 mutate。
