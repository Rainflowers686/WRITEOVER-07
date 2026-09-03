# FABLE PATCH DECISIONS

本文件记录 Fable 5 在当前仓库根目录做的全部代码/测试/文档/治理改动及理由。
不重开产品范围；只修正确性、测试可信度、工程地基。

## G0 — Test Oracle（第一优先级）

### 决策
- `tests/test_harness.h`：`WO_CHECK`/`WO_CHECK_EQ`/`WO_CHECK_NEAR` 改为 fail-fast 宏：
  assertion 失败立即 `return false`，不再"打印失败但测试仍 PASS"。
- `tests/test_common.cpp`：新增 3 个 meta-test：
  - `test_harness.check_detects_failure` — `Check(false)` 返回 false
  - `test_harness.failfast_macro_proves_failure` — fail-fast 宏语义正确
  - `test_harness.check_pass_keeps_going` — 通过断言不打断
- 独立验证：修复前 `player.input_mapper_rebind` 打印 `ASSERT FAILED` 但显示 `[PASS]`；
  修复后同一测试正确报 `[FAIL]`，exit=1。

### 理由
50 tests, 0 failed 在旧 Oracle 下不可信（F-01 实锤）。必须先让测试证据可信。

## F-02 — InputMapper 冲突策略

### 决策
- `src/player/input_mapper.cpp` `SetBinding` 增加 replace 冲突策略：
  设置新绑定时清除所有绑定同一 PhysicalKey 的旧动作。
- 不删除测试（Opus 审计明确要求"修好而不是删测试"）。

### 理由
旧行为 `MapKey(E)` 返回第一个匹配 `LeanRight` 而非重绑后的 `Interact`。
replace 策略保证 `MapKey` 返回最近设置的动作。

## F-05 — Engine EventBus Dispatch

### 决策
- `src/core/engine.cpp`：每个 sim tick 在模块 `SimTick` 之后调用
  `context_.events->Dispatch()`。
- 语义：同 tick 产生的事件下一 tick 才可见（end-of-tick fan-out，见 HK-1 文档）。

### 理由
旧 loop 从不 Dispatch，Post 的事件永远到不了 consumer。

## F-13 — Save 7-section

### 决策
- `src/core/save.cpp`：`section_count >= Count` → `section_count > Count`。

### 理由
`SaveSectionId::Count = 7` 表示 0..6 共 7 个合法 section；旧检查把全部 7 个合法
section 的存档当作非法拒绝。

## F-08 — Storylet 加载

### 决策
- `src/narrative/storylet.cpp` `Load()`：读取 fired 状态前检查 `d.Remaining()`，
  内容文件（无 runtime 状态）可干净加载；含 runtime 状态的存档也可加载。

### 理由
contentc 生成的 storylets.bin 只含 definitions；旧 `Load` 在文件末尾越界读，
导致 NarrativeModule 静默重置为空引擎。

## F-07 — 假因果链

### 决策
- `src/app/composition_root.cpp` NarrativeModule::SimTick：移除
  `frame → frame-1` 的假 causality 链；改为真实 storylet 触发时记录。

### 理由
旧因果账本每个 tick 伪造 EventId 链，无法支撑"因果后果可观察"产品支柱。

## HK-1 — Sim/Event/Save/Replay

### 决策
- 新增 MiniWorld 参考实现测试（RNG+EventBus+player+door+power+NPC+fact+storylet+
  narrator capability+pending reaction），实现真实 Save→Load→Resume 验证。
- 新增测试：`replay.save_load_resume`、`save.all_7_sections_legal`、
  `save.duplicate_section_rejected`、`save.unknown_section_rejected`、
  `event.fanout_all_consumers_see_all`、`event.same_tick_next_tick_semantics`。

## HK-2 — Height-Span Renderer

### 决策
- `raycaster.cpp`：双向边界（SegFloorDrop=4 / SegCeilingRise=5）+ 角点 tie 两轴前进。
- 新增 `reference_renderer.h/.cpp`：可见字符 3D 参考渲染器（深度着色 + marker 遮挡）。
- 新增 18 个 golden scenes（G01-G18）+ 可见性/确定性测试。

### 理由
旧 raycast 只有 floor rise / ceiling drop 单向，且 RenderModule 只画 40 列空格，
不是真实 3D。

## HK-3 — Controller Geometry

### 决策
- `controller.cpp`：新增 `GroundProbe`（含 jump-ascent 守卫）、`IsStepRiseOnly`、
  floor snap step-up、`LeanClamp` 二分搜索、`HeadCollision`、`StepDownAvailable`、
  `NearWallDistance`、Vault/Mantle/Ladder eligibility。
- 新增 8 个属性测试（idle 稳定、20cm step、50cm 拒绝、lean clamp、head hit、
  falling 不接地、no NaN/Inf、save/load round trip）。

## HK-4 — Windows Input

### 决策
- `win_input.cpp`：key-down + key-up 都处理；`PeekConsoleInput`+`FlushConsoleInputBuffer`
  改为 `ReadConsoleInput`（不再丢事件）。

### 理由
`FlushConsoleInputBuffer` 会丢弃队列中剩余事件（F-03）。

## HK-5 — Terminal

### 决策
- `win_terminal.cpp`：ANSI 路径增加 color-run 压缩（相邻同色 cell 不重复 SGR）；
  Win32 fallback 改为完整 RGB→16 色最近邻量化；最后 fallback 调用 `Init()`。
- `benchmark.h/.cpp` + `tools/bench/main.cpp`：`p99_ms` → `worst_1pct_avg_ms`。

## HK-6 — Content / Storylet

### 决策
- `contentc.py`：`--check` 实现 temp recompile + 逐字节比对；RoomId 由编译器按
  排序赋 1..N。
- `map_validator.cpp`：`light > 255`（uint8 永假）改为 `light < 1` 零光照警告。

## 治理

- `tools/audit/static_audit.py`：改写为"固定不变式回归门"（检测修复是否回退），
  当前 0 findings。
- `engineering/AGENTS.md`、`codex/GLOBAL_CODEX_RULES.md`：同步本轮决策。

## 未修改的 public contract

- `IWorldQuery`、`IInputBackend`、`ITerminalBackend`、`SaveSectionId` 等接口
  未改签名；`benchmark.h` 字段改名是 internal API 修正。
- 无新增第三方运行时依赖。
- 未新建替代仓库、未重建平行架构。
