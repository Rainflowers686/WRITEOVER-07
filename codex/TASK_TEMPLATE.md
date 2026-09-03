# TASK_TEMPLATE — 每个 Codex 任务的标准执行与输出协议

> 任务负责人（人类）用本模板写任务；Codex 输出必须以协议块结尾。真实性优先。

## 任务定义

```text
TASK_ID        = <如 M3-INPUT-001>
MODULE_OWNER   = M3 Player
OWNED_FILES    = 允许触碰的文件/目录
FORBIDDEN_FILES= 不可触碰
PUBLIC_CONTRACT= 涉及哪些冻结头（缺省 = 无涉及）
INVARIANTS     = 必须保持的确定性/接口不变量
ACCEPTANCE     = DoD：build exit 0 / 新测试名 / 相关 bench / gate 名
STOP_WHEN      = 明确的停止条件
```

## Codex 执行顺序

1. 读：本模板 + GLOBAL_CODEX_RULES + 模块 prompt + 相关冻结头文件；
2. 写 PLAN（≤5 行）并列出将保持的不变量；
3. 实现（只动 owned 文件）；
4. `cmake --build --preset ci` → `ctest --preset ci`；
5. `git diff --stat` 自查 + `scripts/contract_check.ps1`；
6. 填输出协议块，交给人工 Owner。

## 输出协议块（每个任务必须原样出现）

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
BENCHMARK_RESULT          (若相关)
DIFF_SUMMARY
PUBLIC_CONTRACT_CHANGED = NO
KNOWN_LIMITATIONS
STOP_REASON
```

规则：没跑过的命令写 `NOT_RUN` + 原因；`PUBLIC_CONTRACT_CHANGED` 只能是 NO
或 ADR 编号；DIFF_SUMMARY 逐文件说明理由。

## DoD（模块通用）

```
- 编译 0 警告（/W4 /WX）
- ≥3 单元测试：1 数据结构 + 1 核心算法 + 1 边界/错误
- 集成路径（smoke 或模块集成测试）通过
- 有 debug 面板/命令入口（M1/M2 相关）
- 无公共 API 漂移
```