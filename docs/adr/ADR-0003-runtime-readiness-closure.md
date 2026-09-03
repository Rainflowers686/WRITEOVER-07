# ADR-0003: Runtime Readiness Closure (Fable 5 Foundation Closure Pass)

- 状态: 接受
- 日期: 2026-09-03
- 决策者: Fable 5 (Independent Principal Engineer) + Human Review
- 受影响模块: M1, M2, M3, M4, M6

## 背景

Human Review 确认六模块并行施工前 AGENTS.md 存在以下缺口：
- Input Context 不存在（B.1）
- Mouse Delta 路径不存在（B.4）
- Terminal Encoder 无 full/delta/unchanged 实现（C）
- Room refs 静默丢弃（D.1）
- Content IDs 因插入新内容漂移（D.2）
- Governance 包含已不存在的 dev/PR 流程（I）

## 决策

### 最小 public contract 变更

1. `include/writeover/player/input.h`:
   - 新增 `InputContext` 枚举 (Gameplay/Dialogue/Menu/Developer)
   - `InputMapper` 改为 `[context][action] -> key` 二维数组
   - 新增 `SetBinding(InputContext, ...)`, `MapKey(InputContext, ...)`, `GetBinding(InputContext, ...)`
   - 旧无 context 重载默认 Gameplay，保持向后兼容

2. `include/writeover/common/input_types.h`:
   - 新增 `IInputBackend::ConsumeMouseDelta(Vec2&)` 虚方法（默认返回 false）

3. `include/writeover/common/serialize.h`:
   - 新增 `Serializer::WriteI8`, `Deserializer::ReadI8`, `Deserializer::Skip`

4. `include/writeover/common/command.h`:
   - 新增 `SerializeWorldCommand`, `DeserializeWorldCommand`（带 tag 的 variant 序列化）

5. `include/writeover/render/benchmark.h`:
   - `p99_ms` → `worst_1pct_avg_ms`（字段重命名，非新增）

### 非 public 变更

- 新增 `include/writeover/render/frame_encoder.h` + `src/render/frame_encoder.cpp`
- `AnsiTrueColorBackend::Submit` 使用 AnsiFrameEncoder（full/delta/unchanged）
- `KeyboardOnlyBackend::Poll` 使用 `GetNumberOfConsoleInputEvents` 实现 non-blocking
- `KeyboardOnlyBackend` 增加 FOCUS_EVENT 跟踪焦点
- `CursorDeltaBackend` 实现 `ConsumeMouseDelta` 返回累计 dx/dy
- `LeanClamp` 使用 camera local right vector `(-sin(yaw), cos(yaw))`
- `ReferenceRenderer` marker depth test 考虑垂直重叠
- MiniWorld::Step 只处理本 tick 分发的 events（exactly-once reaction）
- Save parser 增加 `d.Remaining() < 4` 预检（防止 size_t underflow），增加 trailing-byte 政策
- room.cpp 序列化/反序列化 npc_refs + storylet_refs
- contentc.py 使用 FNV-1a64 稳定 ID（INSERT 前内容不漂移），修复 light=0 被 or 255 覆盖的 bug

## 备选方案

- 逐文件 ID 排序（1..N）：拒绝（插入新内容导致旧 ID 漂移）
- 全局 facade 输入框架：拒绝（超出本轮范围）
- Raw Input 大规模实现：拒绝（保持 CONDITIONAL_UNVERIFIED）

## 影响

- 93 tests, 0 failed（+5 新增测试）
- 所有已有测试因 context-overload 向后兼容
- InputMapper 序列化格式不变（contexts count + context 数组）
- 内容编译产出需重新生成（FNV-1a64 ID 更改了二进制）

## 测试

- `input.context_same_key_different_actions` — 同一 key 在不同 context 不同动作
- `input.context_replace_within_context` — 同一 context 内 replace 策略
- `terminal.unchanged_frame_emits_no_payload` — 无变化帧 fast path
- `terminal.delta_smaller_than_full_for_small_change` — delta 小于 full
- `terminal.encoder_deterministic` — 编码器确定性
- `event.reaction_exactly_once` — 单事件 reaction 总数 == 1
- `save.truncated_before_footer_rejected` — 截断 save 拒绝
- `save.huge_section_data_size_rejected` — 恶意 data_size 拒绝
- `render.marker_hidden_behind_full_wall` — 满墙遮挡 marker
- `render.marker_visible_above_low_wall` — 低墙不遮挡 marker
- `controller.lean_respects_yaw_90` — lean 方向随 yaw 旋转

## 回滚

可逐项回滚。撤销 `input.h` 的 context 表需要将 `bindings_` 类型恢复为
`std::array<PhysicalKey, kGameActionCount>`。恢复 `p99_ms` 需同时
更新 `benchmark.cpp`、`tools/bench/main.cpp`、`test_render.cpp`。
恢复 marker-only worldcommand 需还原 `contentc.py` 与 `storylet.cpp` 的 case 2。