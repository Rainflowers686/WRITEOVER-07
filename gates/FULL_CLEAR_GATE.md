# FULL_CLEAR_GATE — 首次完整通关（9/11 18:00）

入口：`scripts/test.ps1 -Gate FullClear`。

## 门槛（正常首次通关 25–30 分钟；Judge Mode 8–10 分钟）

- [ ] 全 6 Room 可顺序通关（三支柱任一不空转：射击爽快/旁白翻脸/因果可见）
- [ ] 2 Full + 2 Medium NPC 触发至少 1 次旁白反制链路
- [ ] ≥1 结局达成；因果 ledger 在 F3 可查
- [ ] 真检查点存档：死亡重开/读档/New Game 均回到合法状态
- [ ] 确定性：同一 Judge seed 两遍 playthrough 结局一致（snapshot 对比）
- [ ] smoke + 全量 CTest 保持绿

## 可执行现状

基础路径组件已绿：sim 循环/存档原子写/raycast/控制器/combat/storylet typed
（50 tests）；**完整关卡内容与 playthrough 集成属 M4/M6/M1 施工成果**，
本 gate 当前 NOT_READY（如实，不 hardcode）。

## 失败处置（T 节 #26）

未通关 → 砍次要路线，每 Room 只留 1 条（内容削减不改架构）。

## 判定

代码绿 + 人工通关记录（含时间）双条件成立才 PASS。