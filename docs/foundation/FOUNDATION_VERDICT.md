# FOUNDATION_VERDICT（REPAIR PASS v2 — 由证据决定）

> 上一轮 VERDICT 声称 `REPO_SEED_BUILDS=YES / TEST_HARNESS_RUNS=YES /
> CODEX_READY=YES` 而目录全空（F-001），已废弃。本裁决只依据本轮真实验证
> （evidence/FOUNDATION_VALIDATION.md，本机实跑）。

```text
FOUNDATION_VERDICT      = MODIFY
PUBLIC_CONTRACTS_READY  = YES
REPO_SEED_BUILDS        = YES
TEST_HARNESS_RUNS       = YES
CODEX_READY             = NO
OPEN_FATAL              = 0
OPEN_MAJOR              = 0
NEXT_STEP               = （六 Owner sign-off → 6 Codex Agent 施工）
```

## 为什么是 MODIFY / CODEX_READY=NO（诚实）

代码侧（build/test/bench/smoke/contract/mapc）已全部实测通过，因此
`REPO_SEED_BUILDS/TEST_HARNESS_RUNS = YES` 是**事实**。

`CODEX_READY` 仍为 NO，因为验收 Gate v2（04 文档）要求的是"六个 Codex Agent
正式进场"的完整状态，它包含**流程性人工动作**，这些不能由本包单方面盖章：

| 缺口 | 性质 |
|------|------|
| CONTRACT_FREEZE_GATE 人工 sign-off（六 Owner） | 流程动作 |
| R0 实机项（≥120Hz 实显、Raw Input 注册、碰撞手测） | 需真机/施工 |
| Codex 进场前的 git 仓库初始化 + CI runner | 组织动作 |

若六 Owner 在 9/3 冻结窗口内完成上述签核与 R0 实测，且无新 fatal，
则 `CODEX_READY` 依同一证据标准翻转为 YES——但那个 YES 只能由人基于本包
证据做出，不由本包自我宣告（这正是上一轮被 REJECT 的原因）。

## 验收 Gate v2 逐条状态

### 1. Artifact presence
engineering/00..30 含 11 ✓ · repo_seed/AGENTS.md ✓ · CMakeLists ✓ ·
CMakePresets ✓ · include/src/tests/tools/scripts 非空 ✓ · schemas 非空 ✓ ·
codex ≥8 ✓ · gates ≥8 ✓ · evidence/FOUNDATION_VALIDATION.md ✓

### 2. Build evidence
Debug/Release configure+build exit 0 ✓ · test target exit 0（50 tests）✓ ·
smoke executes ✓ · bench executes ✓ · contract_check executes ✓ ·
不可运行项均标 UNVERIFIED/NOT_RUN（≠PASS）

### 3. Contract closure
组合根无环 ✓ · platform 接口方向 ✓ · Command≠Event≠Query≠Presentation ✓ ·
world 单写者 tick phases ✓ · 2.5D 边界开口 ray ✓ · floor/ceiling 策略 ✓ ·
controller 单一状态源 ✓ · Input contract 真实存在 ✓（11 文档）· typed
storylet ✓ · FactValue 可装 EntityId ✓ · Narrator capability→command ✓ ·
save 时间戳/replay/profile ✓ · content JSON 编译管线 ✓ · terminal probe 不虚构 ✓ ·
strong ID policy ✓

### 4. Codex governance
root AGENTS.md ✓ · owned/forbidden ✓ · 禁 API 漂移（hash baseline）✓ ·
禁依赖/线程 ✓ · 强制 build/test 证据 ✓ · NOT_RUN 政策 ✓ · drift checker ✓ ·
dependency checker ✓

## 最终

`OPEN_FATAL=0, OPEN_MAJOR=0`。下一动作：六 Owner 执行 CONTRACT_FREEZE_GATE
签核 + R0 实机，然后按 codex/TASK_TEMPLATE 开始六模块施工。
**不重开产品红队，不重做规划轮。**