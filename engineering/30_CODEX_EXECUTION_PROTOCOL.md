# 30_CODEX_EXECUTION_PROTOCOL

## Purpose

This document defines how Codex agents work inside WRITEOVER-07 without breaking the foundation. Read together with root `AGENTS.md`, module-specific `codex/M*_*_CODEX.md`, and the engineering constitution.

## Role Boundaries (Frozen)

**Codex MAY implement:**
- DDA / span traversal
- Collision details
- FSM transitions
- Serialization functions
- Tests
- UI render code
- Tools
- Logs
- Standard algorithms

**Codex may NOT:**
- Change public types
- Add a second ID/vector/event bus implementation
- Change save schema
- Change simulation timing
- Change module dependency graph
- Add third-party dependency
- Add new threads
- Change renderer paradigm
- Change content schema
- Change error policy

## Before Work (Checklist)

1. Read this file + `AGENTS.md` + `00_ENGINEERING_CONSTITUTION.md`
2. Read the module's public contract headers
3. Read module-specific `codex/M*_*_CODEX.md`
4. Inspect current code in the owned file list
5. List the invariants you will preserve
6. Plan a minimal diff (≤400 lines)

## During Work (Hard Rules)

- Only touch files you own (see owned-files boundary in module prompt)
- Never touch public API headers
- Never add includes outside your dependency allow-list
- Never "顺便重构" (drive-by refactor)
- Never expand scope
- Never add `std::rand`, wall-clock, unordered iteration, raw owning pointers

## Task Output Protocol

Every Codex task output MUST end with this block (fill truthfully):

```text
CURRENT_FACTS
OWNED_FILES
INVARIANTS
PLAN
IMPLEMENTED_CHANGES
BUILD_COMMAND
BUILD_RESULT
TEST_COMMAND
TEST_RESULT
INTEGRATION_RESULT
BENCHMARK_RESULT (if relevant)
DIFF_SUMMARY
PUBLIC_CONTRACT_CHANGED = NO
KNOWN_LIMITATIONS
STOP_REASON
```

Rules:
- If build/test did not actually run → write `NOT_RUN` (never fabricate).
- `PUBLIC_CONTRACT_CHANGED` must be `NO` unless an ADR + owner approval exists (then the ADR number goes here).
- `DIFF_SUMMARY` lists every file touched with the reason.

## Sequence (per task)

1. Read inputs
2. Write plan in task notes
3. Implement in owned files
4. `scripts/build.ps1 -Preset ci`
5. `scripts/test.ps1`
6. (if module) run module integration + relevant bench
7. `git diff --stat` review
8. Run `tools/contract_check/check_forbidden.ps1`
9. Fill protocol block above
10. Hand to human owner for review

## Stop Conditions

- Build fails >2 attempts without new hypothesis → stop, report
- Test failures in OTHER modules from your change → stop, report (never "fix" others' code)
- Public API change needed → stop, write ADR draft, get owner
- Any instruction conflicts with constitution → constitution wins, stop, report

## Success Criteria

Documented in each module prompt: the module's DoD (≥3 tests, 0 warnings, integration, bench if relevant). A task is done only when its DoD is met with real evidence.

## Handoff to Humans

Every Codex merge is reviewed by the module's human owner before it counts. Review checklist in `28_PR_REVIEW_POLICY.md`.
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
