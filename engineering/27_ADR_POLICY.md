# 27_ADR_POLICY

## When an ADR Is Required

An Architecture Decision Record must be written BEFORE making any of these changes:

1. Any change to a public type or header in `include/writeover/`
2. Any change to save schema / file format
3. Any change to simulation timing (120Hz fixed)
4. Any change to module dependency graph
5. Adding any dependency (third-party or new module)
6. Adding a thread
7. Replacing renderer paradigm (raycaster variant)
8. Changing content schema (room/npc/fact/storylet files)
9. Changing error policy (exceptions, Result, assert)
10. Changing determinism scope (RNG algorithm, iteration order rules)
11. Changing performance budgets/presets
12. New feature beyond P0 scope

## ADR Process

1. **Draft**: create `docs/adr/ADR-XXXX-title.md` with template below
2. **Owner approval**: the human module owner(s) of affected modules approve
3. **Opus review**: for items Opus designated as key (interface signatures, module deps, renderer, save format, performance baseline) — keep a record
4. **Notification**: notify all affected modules (they may need to update code)
5. **Merge**: ADR merged first, code change follows

## ADR Template

```markdown
# ADR-0001: Title

- 状态: 提议 / 接受 / 已实现 / 废弃
- 日期: YYYY-MM-DD
- 决策者: [owner IDs]
- 受影响模块: [M1..M6]

## 背景
(为什么需要决策)

## 决策
(具体决定，含代码/格式)

## 备选方案
(至少一个备选及其被拒绝原因)

## 影响
(对接口、存档、确定性、性能、工期的影响)

## 回滚/回退
(如何回退)
```

## Ownership

- A (core) owns ADR numbers 1–50
- Module owners draft and own ADRs for their modules' interfaces
- Merge requires owner approval of ALL affected modules listed

## Why Bother

The Opus freeze (9/3) depends on public contracts staying put. This ADR process is how exceptions are handled safely — and how Codex is blocked from silent drift.
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
