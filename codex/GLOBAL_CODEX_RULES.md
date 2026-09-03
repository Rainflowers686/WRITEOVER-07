# GLOBAL_CODEX_RULES — 六模块共用宪法（Codex 必须执行）

> 覆盖 root AGENTS.md + 00_ENGINEERING_CONSTITUTION。Codex 施工前先读本文件
> 与对应模块 prompt；任何冲突以本文件 + 冻结头文件为准。

## 1. 谁拥有什么（owned / forbidden）

| Owner | Owned（可改） | Forbidden（不可碰） |
|-------|---------------|---------------------|
| M1 Core | src/core/, src/app/, scripts/, tools/contract_check/ | 各模块 src/ |
| M2 Render | src/render/, src/platform/windows/win_terminal.cpp | world mutation |
| M3 Player | src/player/, platform input 后端 | save 文件 |
| M4 World | src/world/, data/, tools/mapc, tools/contentc | 其它模块接口 |
| M5 AI | src/ai/ | core/render/narrative |
| M6 Narrative | src/narrative/ | FactStore 直写 |

公共头 `include/writeover/**` 冻结：任何人不得无 ADR 修改。

## 2. 绝对禁止（机器可查项）

- 第二个 EntityId/Vec/EventBus；std::rand；gameplay 墙钟；无序迭代影响决策。
- 原始 owning 指针、struct dump 序列化、`system("cls")`、逐字符 cout。
- 静默 catch、magic payload 字节、公共 API 漂移、render/world 共享可变状态。
- 隐藏网络、SDL/DirectX 窗口、runtime LLM、P1/P2 范围、泛型投机框架。
- 新增依赖（第三方=0）、新增线程、改 save schema/120Hz/渲染范式/错误政策。
- **fake 测试/bench 结果**（hardcode PASS）——发现即驳回。

## 3. 强制纪律

- 每个任务 ≤400 行 diff；只碰 owned 文件；不做 drive-by 重构。
- 每次改动必须给出真实 build/test 证据；未运行写 `NOT_RUN`。
- 公共 API 漂移检查：`scripts/contract_check.ps1`（含 public header hash baseline）。
- Codex 永不自我批准；合入需要 ≥1 位非本人 Owner 人工评审。

## 4. 必跑命令（工作副本内）

```powershell
cmake --preset ci && cmake --build --preset ci
ctest --preset ci --output-on-failure
scripts/smoke.ps1
scripts/contract_check.ps1
```

## 5. 停止条件

- 同一假设 build 失败 >2 次 → 停下报告；
- 你的改动导致**其它模块**测试失败 → 停下报告（不许代修）；
- 需要改公共 API → 停下，写 ADR 草稿并找 Owner。

## 6. 每周/每日节奏（与 Opus 冻结对齐）

18:00 feature→dev PR；22:00 dev→main；每日 ≥3 单测/模块；UML/STL/模式报告节。