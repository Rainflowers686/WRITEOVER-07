# 23_TEST_STRATEGY (REPAIR PASS v2)

> 修订：确定性 replay 测试定义按 M-008 修正（A==C 两路对比，禁止 B==C 错式）；
> save round-trip 无墙钟时间戳所以允许 byte-for-byte（M-007）。测试已实现于
> `repo_seed/tests/`，50 个全过（ctest exit 0）。

## 分层

| 层 | 入口 | 现状 |
|----|------|------|
| Unit | `writeover_tests`（自研 harness） | **50 tests, 0 failed** |
| Golden ray | 同上 + 固定断言 | ray.* 系列 |
| Deterministic replay | 同上 | replay.same_seed_identical_bytes |
| Save round-trip | 同上 | save.deterministic_round_trip / rejects_* |
| Content validation | `mapc` | exit 0（实测） |
| Integration smoke | `writeover_app --smoke` | exit 0（实测） |
| Benchmark | `writeover_bench` | exit 0 + CSV |
| Clean machine | release gate 人工清单 | NOT_RUN（9/16 档期） |

## 每模块 ≥3 测试（DoD）

每模块：数据结构 / 算法 / 边界错误 三类。当前分布：
common 4 · core 6 · world 7 · player 8 · ai 6 · narrative 8 · save-replay 4 · render 7。

## Deterministic replay（修正）

```
path A: seed S + 同输入 → 1000 ticks → 快照 A
path B: seed S + 同输入 → 1000 ticks → 快照 B
reload: 由 B 恢复 → 再 1000 ticks → 快照 C
断言:   A == C（同 tick 同状态）；错误写法 B == C 已废弃
```

## Save round-trip（无时间戳）

save → parse → re-compose → byte-identical；bit flip / truncate / garbage →
`Err`（fail-closed）。profile 独立文件单独 round-trip。

## Golden 变更流程

ray golden 断言改动必须：显式 + diff 审查 + ADR（禁止顺手改期望）。

## 规则

- 无 fake PASS：每条注册测试真实执行并断言（test_harness 无跳过/禁用机制）。
- 未运行 = `NOT_RUN`；Codex 协议块强制。
- CI（ci.yml）：contentc --check → build → ctest → mapc → smoke → 扫描。

## 报告友好

**Course Note**: "测试通过"的唯一证据是 ctest 输出行——不是口头声明。