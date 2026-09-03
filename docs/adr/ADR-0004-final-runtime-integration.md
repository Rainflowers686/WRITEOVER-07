# ADR-0004: Final Runtime Integration and Extensibility Contracts

- 状态: 接受
- 日期: 2026-09-03
- 决策者: Fable 5 (独立加固轮)
- 受影响模块: M1 (core/input/settings), M3 (player/controller), M2 (render/terminal), M4 (world/content), M6 (narrative)

## 背景
Human Review 确认 Foundation 在 Fable 加固后仍存在 5 个 OPEN_MAJOR 和若干 OPEN_MEDIUM 问题。本轮闭合这些缺口。

## 决策

### 1. InputContext 移至 common/input_types.h
`InputContext` 枚举已从 `player/input.h` 移至 `common/input_types.h`，使 `core/settings.h` 可以引用它而不依赖 `player/input.h`。`player/input.h` 中的重复定义已移除。

### 2. Settings 上下文感知绑定
`Settings::key_bindings` 从单层 `[action]` 改为 `[context][action]` 二维数组。文本格式从 `bind.<action>=<value>` 改为 `bind.<context>.<action>=<value>`。旧格式被接受并解释为 Gameplay 上下文（向后兼容）。

### 3. Windows 输入后端修复
- `KeyboardOnlyBackend` 使用 `std::deque<InputEvent>` 内部队列，确保同一批 `ReadConsoleInput` 中的事件不被丢弃。
- `ENABLE_WINDOW_INPUT` 启用以接收 FOCUS_EVENT。
- `Shutdown` 恢复旧的 console mode。
- `CursorDeltaBackend::HasFocus` 使用 `GetForegroundWindow() == GetConsoleWindow()`。
- `CursorDeltaBackend::Poll` 每次消费后执行 `SetCursorPos` 中心重捕获。

### 4. Composition Root 输入桥接
新增 `InputModule` 私有类，在每个 sim tick 中轮询键盘和鼠标后端，映射到 `InputState`，并写入 `PlayerModule::InputState`。模块注册顺序确保 InputModule 先于其他模块运行。

### 5. 鼠标视角
`ApplyMouseLook()` 应用鼠标 delta 到 `LocomotionState::yaw`（连续性环绕）和 `pitch`（钳制 ±30°），灵敏度来自 `Settings::mouse_sensitivity`。

### 6. 摄像机相对移动
`CameraRelativeWish()` 将本地 WASD 方向通过当前 yaw 转换为世界轴方向：`forward = (cos(yaw), sin(yaw))`，`right = (-sin(yaw), cos(yaw))`。

### 7. 终端编码器尺寸安全
`AnsiFrameEncoder` 保存 `prev_width_`/`prev_height_`，在尺寸变化时重置前一帧并输出 FULL 帧。

### 8. 内容引用验证
NPC 和 storylet 引用在编译时通过 registry 验证。`contentc.py` 扫描 `data/npcs/npcs.json` 建立 NPC registry。每个 ID domain 在运行时进行碰撞检查（FNV-1a64 哈希→规范字符串，碰撞 → 编译错误）。

### 9. Room 解码器 Fail-Closed
`npc_refs_count > 4096` 或 `storylet_refs_count > 4096` 现在返回 `Result::Err`（不再跳过读取后从错误偏移继续解析）。

### 10. 公共头文件变更
- `common/input_types.h`: 新增 `InputContext` 枚举
- `common/serialize.h`: 新增 `WriteI8()`、`ReadI8()`、`Skip()`
- `common/command.h`: 新增 `SerializeWorldCommand()`、`DeserializeWorldCommand()`
- `core/settings.h`: `key_bindings` 改为上下文感知二维数组
- `player/controller.h`: 新增 `ApplyMouseLook()`、`CameraRelativeWish()`
- `player/input.h`: 移除 `InputContext`（移至 common）
- `render/frame_encoder.h`: 新增 `prev_width_`/`prev_height_` 尺寸安全

## 回滚
所有变更可逐项回滚。恢复旧 `key_bindings` 单层数组需要同时更新 `settings.h`、`settings.cpp`、`input_mapper.cpp`、`composition_root.cpp` 和测试文件。