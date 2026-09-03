# 07_WORLD_COMMAND_EVENT_QUERY_MODEL (REPAIR PASS v2)

> 修订（M-003 / M-004 / M-015 closure）：WorldEvent **不再含 Command/Query
> category**；四类彻底分开；固定 tick phases；世界 mutation 单写者；
> 事件 fan-out 语义明确；**删除 kMaxCascadeDepth 伪安全阀**（反应事件天然
> 延迟到下一 tick，失控反应器由测试约束，不靠假计数器）；原因链以 parent id
> 表达；presentation 不占事件总线。对应实现：`common/command.h`、
> `common/world_event.h`。

## 四类通信（冻结）

| Kind | 语义 | 例子 |
|------|------|------|
| WorldCommand | 请求改变世界的 typed request（点对点，不进总线） | `CommandSetDoor` `CommandSetPower` `CommandFire` |
| WorldEvent | 已发生的事实（fan-out 广播，总线） | `EventDoorChange` `EventPowerToggle` `EventWeaponFire` |
| Query/View | 只读访问（IWorldQuery/snapshot），永不 mutate | `AabbBlocked` `LineOfSight` `GetCell` |
| Presentation | 玩家所见（字幕/HUD），叙事侧队列 | `SubtitleLine` `HudFrame` |

## EventBus 语义（实现即合同）

- `Post(payload, kind, source, target, parent, sim_frame)`：写入 next 队列。
- `Dispatch()` 每 tick 结尾执行一次：把 pending 按序发给全部消费者（**没有
  消费者会"抢走"事件**）；消费期间新发的进入下一 tick；随后清空 pending。
- Journal 为只读环形 500 条（`JournalSnapshot`），供测试与审计；读取不消费。
- 无级联深度上限：反应事件天然下移 tick；若某反应器每 tick 都自产事件，由
  单元测试与内容预算约束。
- Save/Load：EventId 计数 + pending + next + journal 全量序列化（确定性）。

## Tick Phases（120Hz，固定顺序）

```
1. INPUT     poll → InputMapper → InputState（动作语义）
2. COMMANDS  player/ai/narrative 产生 typed WorldCommand
3. MUTATION  world 权威系统校验并应用命令（单写者），写 FactStore/基础设施
4. EMIT      mutation 产生 WorldEvent（记录 sim_frame + parent）
5. OBSERVE   ai/叙事 在事件后更新 belief/state（确定性顺序）
6. DISPATCH  EventBus::Dispatch（fan-out；反应命令进入下一 tick 的 phase 2）
7. FINALIZE  渲染快照（只读）就绪
8. RENDER    render 读快照画帧（alpha 插值仅视觉）
```

## 单写者规则

- 世界状态（grid/基础设施/FactStore）只有 world 权威系统可写。
- 玩家/NPC 写自身状态；narrative 只发 typed command 或 presentation，
  **永远不直接写 FactStore**（见 15/17 文档与 narrator 测试）。

## Query 规则

- Query 永不 mutate、永不发事件；可被任意模块只读调用。
- `IWorldQuery`（world/grid.h）是 render/ai/narrative/player 读几何的唯一入口。

## 报告友好

**STL**: std::variant（typed payload）、std::vector、std::function。
**Design Pattern**: Observer（EventBus fan-out）+ Command。
**Course Note**: 命令/事件/查询/呈现四类分离 = "渲染不意外改世界"的结构保证。