# DAILY_INTEGRATION_GATE — 每日集成（18:00 feature→dev；22:00 dev→main）

入口：`pwsh -File scripts/test.ps1 -Gate Integration`。

## 每日可执行检查

- [x] ci preset configure/build exit 0
- [x] `ctest --preset ci` 全绿（单元套件 50 项基线）
- [x] `scripts/smoke.ps1` exit 0（build+tests+mapc+app --smoke）
- [x] `scripts/contract_check.ps1` exit 0（forbidden/deps/headers）
- [x] contentc --check（编译产物与提交一致）

## 纪律

- 每人 ≥3 单测当日新增必须落进 CTest；PR ≤400 行；CI 非绿不合并。
- 集成失败 → 当日回退上一版，第二日修复；不留红 dev 过夜。
- 每模块 debug 命令/面板入口在 Feature Freeze 前就绪（F3 面板来自 DebugMetrics）。

## 状态机

dev 红 → 禁止 feature→dev 合并；main 绿保持；release 分支只在 RELEASE_GATE 打开。

## 人工项

环形 code review 记录；6 人每日 WBS 勾选（Opus F 节）。