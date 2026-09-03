# FABLE INDEPENDENT REPRODUCTION

> 执行时间：2026-09-03
> 执行环境：Windows 10.0.26200, MSVC 19.44, CMake 3.29.2, Debug/Release x64
> 项目根：WRITEOVER-07/

## 执行命令记录

### cmake --preset debug
```
命令: cmake --preset debug
退出码: 0
结果: PASS
```

### cmake --build --preset debug --config Debug
```
命令: cmake --build --preset debug --config Debug
退出码: 0
结果: PASS
```

### ctest --preset debug --output-on-failure
```
命令: ctest --preset debug --output-on-failure
退出码: 0
结果: PASS (1/1 test)
```

### cmake --preset release
```
命令: cmake --preset release
退出码: 0
结果: PASS
```

### cmake --build --preset release --config Release
```
命令: cmake --build --preset release --config Release
退出码: 0
结果: PASS
```

### scripts/smoke.ps1
```
命令: pwsh -File scripts/smoke.ps1
退出码: 0
结果: PASS
```

### scripts/bench.ps1
```
命令: pwsh -File scripts/bench.ps1
退出码: 0
结果: PASS (240 columns, avg=0.032ms, p99=0.054ms, BUDGET=PASS)
```

### scripts/contract_check.ps1
```
命令: pwsh -File scripts/contract_check.ps1
退出码: 0
结果: PASS (check_forbidden OK, check_deps OK, check_public_headers OK)
```

### python tools/audit/static_audit.py .
```
命令: python tools/audit/static_audit.py .
退出码: 1
结果: 15 findings detected (expected)
```

### writeover_tests.exe direct output
```
命令: ./out/build/debug/Debug/writeover_tests.exe
退出码: 0
结果: "50 tests, 0 failed" BUT with "ASSERT FAILED" on line 88
```

---

## 独立复现判断

### F-01 — Test Oracle 不 fail-fast
**VERDICT: CONFIRMED**

证据：
- 直接运行测试可执行文件，输出显示：
  ```
  [PASS] player.input_mapper_rebind
    ASSERT FAILED tests/test_player.cpp:88: mapper.MapKey(PhysicalKey::E) == GameAction::Interact
  ```
- 该测试被报告为 `[PASS]`，尽管其中有断言失败
- 进程退出码为 0，表明没有任何失败报告
- 源码分析：`test_harness.h` 中 `WO_CHECK` 宏仅返回 bool，不导致测试函数失败
- `test_player.cpp:88` 的 `WO_CHECK(...)` 返回值被忽略，函数最后一行返回 true

### F-02 — InputMapper 重复绑定冲突
**VERDICT: CONFIRMED**

证据：
- 同上，`mapper.MapKey(PhysicalKey::E)` 返回第一个匹配 `LeanRight`（值=9），而不是重绑后的 `Interact`（值=10）
- 源码 `input_mapper.cpp` 使用线性扫描，第一个匹配就返回
- 没有输入上下文（Gameplay/Dialogue/Menu 共享同一映射）

### F-03 — Windows Keyboard 丢 key-up
**VERDICT: CONFIRMED**

证据：
- 源码 `src/platform/windows/win_input.cpp` 只处理 `KEY_EVENT && bKeyDown`，忽略 `!bKeyDown`
- 随后调用 `FlushConsoleInputBuffer` 丢弃该批次中其他事件
- `HasFocus()` 仅检查 `GetConsoleWindow() != nullptr`，不等于真正 foreground

### F-04 — CursorDeltaBackend 不可通过接口获取
**VERDICT: CONFIRMED**

证据：
- `IInputBackend` 接口中不存在 `ConsumeDx()`/`ConsumeDy()` 方法
- `CursorDeltaBackend` 有自己的 `ConsumeDx()`/`ConsumeDy()` 但不属于接口
- `Poll()` 对鼠标移动返回 key=Unknown, analog=0
- 没有公开的 downcast 路径读取累积 delta

### F-05 — Engine 没有调用 EventBus::Dispatch()
**VERDICT: CONFIRMED**

证据：
- `src/core/engine.cpp` 主循环中：
  - 对每个模块调用 `module->SimTick()`
  - 调用 `clock->Tick()`
  - 调用 `render_->RenderFrame()`
  - **没有调用 `events.Dispatch()`**
- 即使模块在 SimTick 中 Post 事件，也不会被 fan-out 到其他模块

### F-06 — EventBus next_pending 语义
**VERDICT: CONFIRMED — 设计需明确**

证据：
- `Post()` 将事件放入 `next_pending_`
- `Dispatch()` 先广播 `pending_`，然后 `pending_.swap(next_pending_)`
- 这意味着 mutation 阶段产生的事件至少延迟一 tick 才被观察到
- 注释说"end-of-tick fan-out"，但实现是 unconditional next-tick
- 不是错误，但需要明确语义并测试

### F-07 — Narrative 因果账本伪造
**VERDICT: CONFIRMED**

证据：
- `src/app/composition_root.cpp` NarrativeModule::SimTick:
  ```cpp
  ledger_.Push(CausalityEntry{
      EventId::New(frame),
      frame > 0 ? EventId::New(frame - 1) : EventId::Invalid(),
      frame, EventKind::Notification});
  ```
- 因果链是 `frame → frame-1`，与真实 EventBus 无关
- 每个 tick 独立创建新 EventId，不关联真实事件

### F-08 — Storylet compiled content corrupted
**VERDICT: CONFIRMED**

证据：
- `src/narrative/storylet.cpp` 中 `LoadBinary()` 在读取所有 definitions 后继续读取 `fired_count`
- `contentc.py` 生成的 storylets.bin 在最后一个 action 结束后即 EOF
- runtime 加载时由于 `fired_count` 不存在，`d.HasError()` 返回 true
- `NarrativeModule::Init()` 在 LoadBinary 失败后静默重置为空的 StoryletEngine
- 因此 smoke 仍然通过，但叙事内容未加载

### F-09 — WorldCommandAction 丢失 payload
**VERDICT: CONFIRMED**

证据：
- `contentc.py` 中的 worldcommand 只写 marker 0
- C++ 端 `Load()` 的 case command 只读 marker 不生成 action
- `StoryletEngine::Save()` 也只写 marker
- 缺少强类型 action 类型（SetDoor, SetPower, SetFlag, QueueLine, EndGame 等）

### F-10 — contentc --check 未实现
**VERDICT: CONFIRMED**

证据：
- `tools/contentc/contentc.py` 中有 `--check` 参数
- 但后续代码从未使用 `args.check` 做 temp recompile / byte compare
- 目前 `--check` 效果等同于无此参数

### F-11 — RoomId 硬编码 1
**VERDICT: CONFIRMED**

证据：
- `tools/contentc/contentc.py` 中：
  ```python
  body += struct.pack("<Q", 1)
  ```
- 所有 room 编译后 RoomId 都是 1
- 没有跨文件 registry 或建立唯一 ID 的机制

### F-12 — Room refs 为空
**VERDICT: CONFIRMED**

证据：
- `contentc.py`：
  ```python
  # NPC / storylet refs (empty ... foundation)
  body += 0
  body += 0
  ```
- Room 数据不引用 NPC 或 Storylet
- 没有 resolvable reference 机制

### F-13 — Save 7-section 被拒绝
**VERDICT: CONFIRMED**

证据：
- `src/core/save.cpp` 行：
  ```cpp
  if (section_count >= static_cast<uint32_t>(SaveSectionId::Count)) {
      return error("save section count out of range");
  }
  ```
- `SaveSectionId::Count = 7`，因此 7 个合法 section（0-6）被拒绝
- 正确应为 `section_count > Count` 以允许 7 个 section

### F-14 — 没有真正的 Save→Load→Resume 测试
**VERDICT: CONFIRMED**

证据：
- `test_save_replay.cpp` 中的 `replay.same_seed_identical_bytes` 只测试：
  - 两次同 seed 同 tick 的 RNG+EventBus bytes 相同
  - 300 tick 与 600 tick 不相同
- 没有测试：2000 ticks uninterrupted == 1000 ticks → Save → Load → 1000 ticks

### F-15 — Smoke save 只存 RNG+Events
**VERDICT: CONFIRMED**

证据：
- `composition_root.cpp` 中 smoke save 只包含：
  ```cpp
  sections.push_back({SaveSectionId::Rng, std::move(rng_bytes)});
  sections.push_back({SaveSectionId::Events, std::move(events_bytes)});
  ```
- 没有 Player, World, AI, Narrative, Storylet runtime, Infrastructure, pending commands

### F-16 — Controller idle 后 grounded=false
**VERDICT: CONFIRMED**

证据：
- `src/player/controller.cpp` 中 `IntegrateLocomotion` 没有 GroundProbe
- `AabbBlocked` 只判断穿透，没有 ground contact query
- 在 box_z 不 blocked 时直接设置 grounded=false
- 导致 Traversal=Grounded 但 contact.grounded=false 的矛盾

### F-17 — 自动 Step-Up 未实现
**VERDICT: CONFIRMED**

证据：
- `controller.cpp` 中水平移动只检查 AABB 是否被阻塞
- 没有 step candidate 逻辑
- 0.2m 高台也会阻塞移动

### F-18 — Lean 无几何约束
**VERDICT: CONFIRMED**

证据：
- `SetLean()` 只设置 `ls.lean = Left/Right`
- 没有 camera offset, wall clamp, head/shoulder sample
- Lean 改变不产生任何碰撞效果

### F-19 — Vault/Mantle/Ladder 为 stub
**VERDICT: CONFIRMED**

证据：
- `Traversal` 枚举中有 Vault, Mantle, Ladder 等值
- 但没有任何实现代码
- 调用 `TrySetPosture` 或相关函数不会触发任何 vault/mantle 逻辑

### F-20 — Height-Span 只处理 floor rise / ceiling drop
**VERDICT: CONFIRMED**

证据：
- `src/render/raycaster.cpp` 中 `CastColumnRay` 只处理：
  - `far.floor > near.floor` → floor rise segment
  - `far.ceiling < near.ceiling` → ceiling drop segment
- 没有处理 floor drop（从高台看向低地面）
- 没有处理 ceiling rise（从低天花看向高房间）
- 缺少双向边界

### F-21 — DDA corner tie 只走 X
**VERDICT: CONFIRMED**

证据：
- `CastColumnRay` 中：
  ```cpp
  if (t_max_x <= t_max_y) step in X
  else step in Y
  ```
- 精确角点时只先跨 X，可能产生 spurious intermediate cell
- 没有 supercover / two-axis advance 策略

### F-22 — Render smoke 没真正画到屏幕
**VERDICT: CONFIRMED**

证据：
- `RenderModule::RenderFrame` 调用 CastColumnRay 但只设置了 40 列
- `code_point = U' '`（空格字符）
- 没有 `ProjectWall`、wall span rasterization、floor/ceiling、depth
- 没有真正的 3D 画面

### F-23 — Player view 只设置一次
**VERDICT: CONFIRMED**

证据：
- `composition_root.cpp` 中：
  ```cpp
  render->SetPlayerView(spawn, 0.0f);
  ```
- 之后没有从 PlayerModule 更新
- 玩家移动后画面不会跟随

### F-24 — ANSI 后端逐 cell 完整 SGR
**VERDICT: CONFIRMED**

证据：
- `src/platform/windows/win_terminal.cpp` 中每个 cell 输出完整 reset+fg+bg+glyph
- 没有 color-run / delta 编码
- 240×67 = 16080 cells 产生数十万字节/帧

### F-25 — Win32 fallback 不完整
**VERDICT: CONFIRMED**

证据：
- `win_terminal.cpp` 中 fallback 只根据 `bg_r >= 128` 和 `fg_r >= 128` 设置 red bits
- 没有完整的 RGB→16 色量化映射

### F-26 — 最后 fallback 未初始化
**VERDICT: CONFIRMED**

证据：
- `CreateTerminalBackend` 中若所有 probe 失败：
  ```cpp
  return std::make_unique<AnsiTrueColorBackend>();
  ```
- 没有调用 `Init()`

### F-27 — Benchmark 命名混淆
**VERDICT: CONFIRMED**

证据：
- `benchmark.cpp` 中 `p99_ms` 字段实际是 slowest 1% frames 的平均值
- 不是严格 P99 percentile
- 应改名以反映真实含义

### F-28 — Content schema 漂移
**VERDICT: CONFIRMED**

证据：
- `schemas/COMPILED_CONTENT_FORMAT.md` 描述字段与 Python 实际输出不一致
- RoomId 全硬编码 1
- Storylet definitions/runtime 混用

### F-29 — MapValidator 永远不可能触发的检查
**VERDICT: CONFIRMED**

证据：
- `src/world/map_validator.cpp`:
  ```cpp
  if (cell.light > 255) { ... }
  ```
- `light` 是 `uint8_t`，值域 0-255，永远不可能 > 255

### F-30 — Windows 可移植性边界
**VERDICT: CONFIRMED**

证据：
- `localtime_s` 是 Windows-specific
- `test_common.cpp` 缺显式 `<cmath>`
- 但课程不要求 ARM 部署，此问题不影响 R0

---

## 与其他证据对比

| 原始报告 | 本独立复现 | 差异 |
|---------|-----------|------|
| 02_INDEPENDENT_REPRODUCTION_EVIDENCE.md | 全部 CONFIRMED | 一致，且增加了代码级证据 |
| BASELINE_STATUS.md 15 项 | 全部 CONFIRMED | 一致，且扩展了 30 项 |
| static_audit.py 15 项 | 全部 CONFIRMED | 一致 |

**结论：原始 02 报告中的独立复现证据全部真实可靠。**