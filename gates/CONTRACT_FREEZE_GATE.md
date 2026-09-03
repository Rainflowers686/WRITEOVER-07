# CONTRACT_FREEZE_GATE — 公共接口 / 数据 schema 冻结（9/3 23:59）

入口：`scripts/test.ps1 -Gate ContractFreeze`。

## 门槛

- [x] `include/writeover/**` 公共头齐备（common/core/render/player/world/ai/narrative）
- [x] `check_public_headers.ps1` baseline 已记录（此后漂移即失败）
- [x] `check_deps.ps1` 依赖白名单全绿（DAG 无环）
- [x] 存档 schema v1 冻结（08 文档 + SAVE_SCHEMA + round-trip 测试）
- [x] content 编译产物格式冻结（COMPILED_CONTENT_FORMAT + contentc --check）
- [x] `engineering/00..30 + 11` 连续存在（31 篇，本包已核）
- [ ] 六模块 Owner 书面 sign-off（人工）

## 冻结后规则

- 公共类型/ID/schema 变更 = ADR（docs/adr/）+ Owner 批准 + Opus 复审
  （接口签名/模块依赖/renderer/save/性能基线五类，见 27 文档）。
- Codex 若触发 drift gate 即失败回退。

## 当前状态

本包交付时刻：所有代码侧检查 PASS；**人工 sign-off 待六 Owner 执行**
（这是流程性动作，不是代码缺口）。