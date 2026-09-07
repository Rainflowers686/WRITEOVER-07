# ADR-0010: Truthful B1 player-loop seams

- 状态: 已实现，待 Rain 验收
- 日期: 2026-09-08
- 决策者: Rainflowers686
- 范围: B1 recovery slice 的最小跨模块语义

## 决策

- `ReaderAcceptsItem` 只判断 credential 是否有效；`ItemHeldBy` 是上层
  将 credential 作为某个 actor 出示前必须调用的 possession seam。物品的
  `current_holder` 与 `location` 仍由 systemic kernel 作为唯一事实源维护。
- `RefreshMemory` 只更新已有 `MemoryRecord` 的 frame/confidence/salience。
  AI 负责选择 semantic key 与 refresh window，不能用逐 tick 新 ID 制造
  重复记忆；真正不同的事件仍然可以创建新记录。
- B1 的门、终端、身体和 cleaner 行为由既有 `InfrastructureSystem`、
  `SystemicWorld`、`EventBus` 和 AI runtime 组合，禁止创建第二套门、凭据、
  记忆或世界状态系统。
- B1 的 `F` 交互先在玩家位置、朝向、距离和视线约束下解析一个确定目标；
  门/reader 是唯一不要求穿过墙体视线的例外，因为 reader 就是门的交互面。
  目标解析失败时不得猜测另一个同半径对象。
- reader 成功链固定为：玩家持有 credential、credential 有效、clearance 足够，
  然后由既有 infrastructure door 改变 locked/open 状态并更新碰撞几何；
  terminal session 与 audit 只有在同一持有校验成功后才创建。
- cleaner 发现链固定为：在同一 room 的可达位置移动到 cart、到达半径内、
  完成 bounded inspection，再写入 body-discovery 事实；discovery 与
  `Report`/`MedicalCall`/`HelpCoverUp` response 是两个阶段。Stunned/Dead
  cleaner 不具备 DirectWitness 能力。
- `BodyHidden`、`BodyDiscovered` 与 cleaner response 在 B1 replay 中是由
  systemic event ledger 重建的里程碑事实；它们不能被读档时的当前 body
  disposition 反向抹掉。runtime non-lethal hit 会创建独立 body record，
  并把 badge 的 holder 转移到 body 后才允许 search/theft 链继续。
- `EventPlayerDamage` 是本轮唯一的 NPC 到玩家伤害 seam；玩家 health/dead
  状态进入 player save section，F9 只恢复通过 bounded deserialize 校验的
  checkpoint 状态。
- 本轮 guard line-of-sight `EventPlayerDamage` 只证明 bounded player-health /
  death seam。它不是 complete enemy combat：不覆盖敌方攻击选择、战术决策、
  敌方武器行为、完整 combat AI 或一般敌人战斗范围；这些仍不在本轮。
- Windows ANSI backend 在提交 character cells 前显式启用 VT processing 和
  UTF-8 output code page；运行时仍需用真实终端截图确认最终观感。

## 约束

本 ADR 不实现通用 interaction/ECS/negotiation framework，也不改变
Character-Art renderer 的方向。每个新 seam 必须有一个旧逻辑会失败的
反事实测试或运行证据。deterministic replay 的进程退出码不是 gameplay
断言；必须同时检查输入消费、预期状态和章节 checkpoint。视觉验收仍为
`PENDING`，本 ADR 不构成 Gold 或 public-release 声明。
