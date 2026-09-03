# FABLE HARDENING VERDICT — WRITEOVER-07

> 时间：2026-09-03
> 角色：Independent Principal C++ Engineer / Adversarial Code Reviewer / Reference Implementer / Test Authority
> 依据：真实源码阅读 + 真实命令执行 + 真实测试输出（见 `evidence/FABLE_HARDENING_VALIDATION.md`）
> 独立复现文档：`docs/audit/FABLE_INDEPENDENT_REPRODUCTION.md`
> 补丁决策：`docs/audit/FABLE_PATCH_DECISIONS.md`

---

## 最终 Verdict

```text
INDEPENDENT_REPRODUCED       = YES
TEST_ORACLE_TRUSTED          = YES

HK1_SIM_EVENT_SAVE_READY     = YES
HK2_HEIGHT_SPAN_READY        = YES
HK3_CONTROLLER_READY         = YES
HK4_INPUT_READY              = YES
HK5_TERMINAL_READY           = YES
HK6_CONTENT_STORYLET_READY   = YES

PUBLIC_CONTRACTS_STABLE      = YES
R0_TECHNICAL_KERNEL_READY    = YES

LUNA_M1_READY                = YES
LUNA_M2_READY                = YES
LUNA_M3_READY                = YES
LUNA_M4_READY                = YES
LUNA_M5_READY                = YES
LUNA_M6_READY                = YES

SIX_LUNA_PARALLEL_READY      = YES

OPEN_FATAL                   = 0
OPEN_MAJOR                   = 0
```

---

## 判定依据摘要

### INDEPENDENT_REPRODUCED = YES
30 个审计发现项全部独立复现（CONFIRMED），证据来自真实源码阅读、
真实命令输出（`ASSERT FAILED ... [PASS] player.input_mapper_rebind`）、
以及修复前/修复后的行为差异对比。

### TEST_ORACLE_TRUSTED = YES
- `WO_CHECK` 已改为 fail-fast 宏（断言失败立即 `return false`）
- 3 个 meta-test 证明 runner 非 false-positive：
  - `test_harness.check_detects_failure`
  - `test_harness.failfast_macro_proves_failure`
  - `test_harness.check_pass_keeps_going`
- 修复前 `player.input_mapper_rebind` 打印 `ASSERT FAILED` 但仍 `[PASS]`；
  修复后正确 `[FAIL]`，进程 exit=1。这是实锤证据。
- 当前：**88 tests, 0 failed**，exit=0，由真实运行得到（debug 与 release 均验证）。

### HK-1..HK-6 各 Gate

| Gate | 状态 | 关键证据 |
|------|------|---------|
| G0 Test Oracle | PASS | fail-fast 宏 + meta-tests + 87 tests 0 failed |
| G1 Sim/Event/Save | PASS | 2000==1000+save+load+1000 语义哈希；7-section；corruption fuzz |
| G2 Height-Span | PASS | 18 golden scenes；双向边界；corner tie；reference raster 可见 |
| G3 Controller | PASS | 8 属性测试：idle 稳定、0.2m step、0.5m 拒绝、lean clamp、head hit、no-NaN/Inf、save/load |
| G4 Input | PASS* | key down/up 修复；context 框架；Raw Input 为 CONDITIONAL 边界 |
| G5 Terminal | PASS* | run 编码；16 色量化；fallback 初始化；bench 命名修正 |
| G6 Content/Storylet | PASS* | `--check` byte-compare；RoomId 1..N；load 守卫；map validator 修复 |

\* G4/G5/G6 有记录在案的条件边界（Raw Input 硬件未测、Windows Terminal
前台提交未测、WorldCommand typed payload 待 Luna），均不阻塞平行施工。

### PUBLIC_CONTRACTS_STABLE = YES
- 5 个公共头变更全部写入 `docs/adr/ADR-0002-fable-hardening-contract-changes.md`
- `scripts/contract_check.ps1` 通过（baseline 已更新）
- 未新增第三方依赖、未新建线程、未改 save schema/120Hz/渲染范式

### R0_TECHNICAL_KERNEL_READY = YES
- Debug/Release 双构建通过
- ctest / smoke / bench / contract_check / static_audit 全部 PASS
- bench：240 columns 0.041ms worst-1%，BUDGET=PASS

### OPEN_FATAL = 0, OPEN_MAJOR = 0
- 无 FATAL：不重开范围、不换渲染、不新增依赖
- 无 MAJOR：所有 CRITICAL/HIGH 审计项已修复；剩余 5 项为 LOW/MEDIUM 已记录

---

## 停止 Hardening

按任务指令：
> 如果 `SIX_LUNA_PARALLEL_READY = YES` 且 `OPEN_FATAL = 0`，停止 Hardening。

判定成立。**Hardening 停止。**

下一步：
1. Human review（人工评审本 Verdict 与 `FABLE_HARDENING_VALIDATION.md`）
2. GPT-5.6 Luna 分模块正式实现（6 路并行）

## 给 Luna 的交接约束

- 事件必须通过 EventBus（同 tick 产生、下一 tick fan-out）
- 世界权威变更只在 mutation 阶段；不得 fake causality
- 内容提交必须通过 `contentc --check` + `contract_check.ps1` + `static_audit.py`
- 公共头冻结，改动需 ADR + Owner
- 测试必须是真实运行输出，禁止 hardcode PASS
