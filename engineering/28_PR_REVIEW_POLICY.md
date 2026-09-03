# 28_PR_REVIEW_POLICY

## Branch Policy

```
main  ←  merge only CI-green PRs, auto daily 22:00
 ├── dev   ← daily integration, auto merge 18:00
 │    ├── m1-core
 │    ├── m2-render
 │    ├── m3-player
 │    ├── m4-world
 │    ├── m5-npc
 │    └── m6-narrative
```

## PR Limits

- **≤400 lines changed** per PR (split larger work)
- Must include smoke test result (build + tests run)
- Must pass CI before merge
- Codex-generated code must be human-reviewed before commit

## Review Checklist (per PR)

- [ ] Builds Debug + Release, 0 warnings
- [ ] New/changed behavior covered by unit test
- [ ] Golden tests updated only with explicit regeneration + diff review
- [ ] No public contract change without ADR
- [ ] No forbidden patterns (grep check passed)
- [ ] Dependency direction respected
- [ ] Owned-files boundary respected (module owner touched only own dirs)
- [ ] No unrelated refactor/scope creep
- [ ] No dead code/commented-out code
- [ ] Logging follows Logger, no printf
- [ ] Determinism preserved (no wall clock, no rand, no unordered iteration)
- [ ] Evidence: exact build/test commands + results included in PR description
- [ ] If save/schema touched: round-trip test run
- [ ] If raycaster touched: golden tests run
- [ ] If bench touched: benchmark baseline comparison

## Review Rules

- Every PR needs ≥1 human approve from a non-author module owner
- Ring code review (course requirement): rotate reviewers
- Bot/Codex output is never self-approved
- Merge window: feature→dev 18:00; dev→main 22:00
- Integration failure → revert to previous version, fix next day

## 报告友好

**Course Note**: 分支与评审纪律确保证明"六人并行不互相破坏"。
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
