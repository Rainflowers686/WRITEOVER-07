# Fable IDE 工作树说明

这是 WRITEOVER-07 后续的**唯一代码工作树基线**。

使用方法：
1. 解压整个目录。
2. 用 Cursor / IDE 直接打开本目录根。
3. 不要再把 `repo_seed/` 当子工程；其内容已经提升到真正仓库根。
4. Fable 5 应直接在本目录原地审计、修复、实现 Hard Kernels。
5. Fable 的专用主提示词单独提供，不嵌入本包。

## 当前来源

- 基础代码：Opus 5 修复后的 `OUTPUT_FABLE_ENGINEERING_FOUNDATION_REPAIRED.zip`
- 产品基线：`docs/project/P0-v1.0_FINAL_CANDIDATE.md`
- 制作计划：`docs/project/OPUS_PRODUCTION_EXECUTION_PLAN.md`
- Foundation 规范：`engineering/`
- Codex 规则：`codex/`
- 已知独立审计：`docs/audit/`
- 课程原文：`docs/course/`

## 当前状态

Opus 最后声明：

```text
PUBLIC_CONTRACTS_READY = YES
REPO_SEED_BUILDS       = YES
TEST_HARNESS_RUNS      = YES
CODEX_READY             = NO
OPEN_FATAL              = 0
OPEN_MAJOR              = 0
```

但随后独立代码审计又复现了若干真实问题，因此现在的状态应理解为：

```text
PRODUCT_SCOPE            = FROZEN
PRODUCTION_PLAN          = FROZEN
FOUNDATION_CODE          = REAL_BUT_REQUIRES_HARDENING
SIX_LUNA_PARALLEL_READY  = NO
```

Fable 的职责不是重新设计产品，而是把这些工程问题修到可以安全交给 Luna。

## Fable 优先阅读

1. `docs/audit/01_INDEPENDENT_AUDIT_FINDINGS.md`
2. `docs/audit/02_INDEPENDENT_REPRODUCTION_EVIDENCE.md`
3. `docs/audit/04_HARD_KERNEL_GATES.md`
4. `docs/audit/06_SOURCE_HOTSPOTS.md`
5. `engineering/`
6. `AGENTS.md`
7. `codex/`

## 注意

`AGENTS.md` 是给未来 Codex/Luna 的冻结规则。
Fable 本轮作为 Foundation Maintainer 可以在有证据的情况下修 public contracts，
但必须：
- 写 ADR；
- 更新相关 engineering contract；
- 更新 tests；
- 最终重新冻结 AGENTS/contract baseline。
