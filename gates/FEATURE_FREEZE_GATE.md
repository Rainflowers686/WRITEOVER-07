# FEATURE_FREEZE_GATE — 功能冻结（9/15 15:00）

入口：`scripts/test.ps1 -Gate FeatureFreeze`。

## 冻结门槛

- [ ] 玩法/关卡/叙事内容冻结：此后只允许修正确性/手感/性能/文档/发布稳定
- [ ] 全模块 DoD 达成（每模块 ≥3 测试 + 1 UML + STL 清单 + 报告 ≥80%）
- [ ] 性能回归阻断扫描：bench 1% low 降 >15% 或平均降 >10% → 禁止合入
- [ ] release 产物候选：`scripts/package.ps1` 可生成单 exe（/MT）
- [ ] 干净机器清单就绪（9/16 执行）

## 可执行现状

- bench 基线已建立（BUDGET=PASS 机制）+ 回归检查入口
- package.ps1 可执行（dist 目录 + SHA256）
- content/feature 完成度属施工期成果 → NOT_READY 如实标记

## 冻结后规则

- 新增玩法大类 / 渲染范式 / 结局体系 / 设置页 → 一律拒绝（P0 范围）。
- 优化窗口 9/15 关闭：只修 bug；基准为准。

## 判定

清单项全 PASS + 6 Owner sign-off = 冻结生效。