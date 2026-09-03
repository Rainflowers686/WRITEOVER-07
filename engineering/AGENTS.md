# AGENTS.md — WRITEOVER-07 Engineering Constitution (Fable-5 hardened)

> 本文件覆盖 root 工作副本规则。所有 Codex/Luna 代理施工前必须阅读。
> 任何冲突以本文件 + 冻结头文件 + GLOBAL_CODEX_RULES 为准。

## 0. 历史边界（Fable 5 独立加固轮已完成）

- G0 Test Oracle 已 fail-fast；`50 tests, 0 failed` 现在是可信证据。
- HK-1..HK-6 参考实现已落地并有测试与文档：
  `docs/reference/HK1_SIM_EVENT_SAVE.md` … `HK6_CONTENT_STORYLET.md`
- 裁决文件：`FABLE_HARDENING_VERDICT.md`；发现清单：`docs/audit/FABLE_FINDINGS_AFTER_REPRO.md`
- 最终闭合：`evidence/FINAL_FOUNDATION_FREEZE.md`；产品基线：`docs/product/PRODUCT_BASELINE_V1_1.md`
- 不再需要重新仲裁产品范围 / Opus 计划 / 六模块分工。

## 1. 谁拥有什么（owned / forbidden）

| Owner | Owned（可改） | Forbidden（不可碰） |
|-------|---------------|---------------------|
| M1 Core | src/core/, src/app/, scripts/, tools/contract_check/ | 各模块 src/ |
| M2 Render | src/render/, src/platform/windows/win_terminal.cpp | world mutation |
| M3 Player | src/player/, platform input 后端 | save 文件 |
| M4 World | src/world/, data/, tools/mapc, tools/contentc | 其它模块接口 |
| M5 AI | src/ai/ | core/render/narrative |
| M6 Narrative | src/narrative/ | FactStore 直写 |

公共头 `include/writeover/**` 冻结：任何人不得无 ADR 修改（当前 ADR-0001/0002）。

## 2. 已固定的契约（Fable 5 冻结）

- **事件语义**：同 tick 产生的事件下一 tick 才 fan-out（end-of-tick）；事件不被消费。
- **Save 7-section**：`section_count > Count` 才拒绝；0..6 全部合法。
- **Step-up**：`kMaxStepHeight = 0.35m` 自动步进；以上拒绝。
- **Lean**：几何 clamp（`LeanClamp`），不穿透墙壁。
- **Raycast 双向边界**：floor rise/drop + ceiling rise/drop；角点两轴同时前进。
- **Benchmark 命名**：`worst_1pct_avg_ms`（不是 `p99_ms`）。
- **Test Oracle**：`WO_CHECK` 失败即 fail-fast；meta-test 证明 runner 可信。

## 3. 绝对禁止（机器可查项）

- 第二个 EntityId/Vec/EventBus；std::rand；gameplay 墙钟；无序迭代影响决策。
- 原始 owning 指针、struct dump 序列化、`system("cls")`、逐字符 cout。
- 静默 catch、magic payload 字节、公共 API 漂移、render/world 共享可变状态。
- 隐藏网络、SDL/DirectX 窗口、runtime LLM。
- **ONLY PRODUCT_BASELINE_V1_1 AUTHORIZED EXPANSIONS ARE ALLOWED（见 `docs/product/PRODUCT_BASELINE_V1_1.md`）**；任何未列入 v1.1 的旧废案仍禁止自行恢复。
- 新增依赖（第三方=0）、新增线程、改 save schema/120Hz/渲染范式/错误政策。
- **fake 测试/bench 结果**（hardcode PASS）——发现即驳回。
- **回退已修复的审计项**（见 `tools/audit/static_audit.py` 回归门）。

## 4. 强制纪律

- 每个任务 ≤400 行 diff；只碰 owned 文件；不做 drive-by 重构。
- 每次改动必须给出真实 build/test 证据；未运行写 `NOT_RUN`。
- 公共 API 漂移检查：`scripts/contract_check.ps1`（含 public header hash baseline）。
- Codex 永不自我批准；合入需要 ≥1 位非本人 Owner 人工评审。
- **Git 工作流**：main-only。无 dev 分支、无 feature 分支、无 PR 流程。禁止六 Agent 同时无约束写同一文件。

## 5. 必跑命令（工作副本内）

```powershell
cmake --preset debug
cmake --build --preset debug --config Debug
ctest --preset debug --output-on-failure
scripts/smoke.ps1
scripts/contract_check.ps1
python tools/audit/static_audit.py .
```

内容变更后额外：`python tools/contentc/contentc.py --data-dir data --out-dir data --check`

## 6. 停止条件

- 同一假设 build 失败 >2 次 → 停下报告；
- 你的改动导致**其它模块**测试失败 → 停下报告（不许代修）；
- 需要改公共 API → 停下，写 ADR 草稿并找 Owner。

## 7. 测试证据要求

- `writeover_tests.exe` 的 `N tests, 0 failed` 必须来自真实运行输出。
- 故意失败的 meta-test 已证明 runner 可信（`test_harness.failfast_macro_proves_failure`）。
- 禁止引用旧版本的测试计数；每次运行记录实际输出。
