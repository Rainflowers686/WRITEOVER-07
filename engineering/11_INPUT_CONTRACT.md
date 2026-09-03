# 11_INPUT_CONTRACT (REPAIR PASS — 新建)

> 上一轮缺此文档（验收项 M-013 / Artifact 3.13）。本版与实现一致：
> `common/input_types.h`（PhysicalKey/GameAction/InputEvent/IInputBackend）、
> `core/settings.h`（Settings::key_bindings 可序列化绑定表）、
> `player/input.h/.cpp`（InputMapper/InputState/ClearInputState）、
> `src/platform/windows/win_input.cpp`（后端实现）。

## 输入分层

```
PhysicalKey 流（后端 raw）  →  InputMapper（键位映射表）  →  GameAction
                                                              ↓
                                        InputState.action_down/pressed/released
```

- gameplay 只读 `GameAction`，永远不读 Win32 VK。
- 键位映射表是**数据**（`std::array<PhysicalKey, kGameActionCount>`），可序列化
  进 settings（`bind.<actionIndex>=<PhysicalKey>`），**绝不是指针/句柄**
  （M-013 closure：旧版 `uint8_t keyBindings; // pointer` 已删除）。

## 支持矩阵与回退（不改公共 API）

| 层 | 主 | fallback | fallback2 |
|----|----|----------|-----------|
| Mouse | Raw Input（M3 施工注册） | **cursor-delta + center recapture**（已实现） | Q/E + 方向键视角 |
| Keyboard | 控制台键盘事件（已实现） | — | — |
| Gamepad | XInput（R0 gate 后） | — | — |

- RawInput 是否可用只影响后端选择，`GameAction`/`InputMapper` 零改动。
- 失焦：app 检测 `backend->HasFocus()` 变 false → `ClearInputState`（防卡键）。
- IME：中文输入法组合期间的按键不映射 gameplay（ime_composing 标志）。

## PhysicalKey / GameAction 范围

- PhysicalKey：键盘 WASD/数字/F1-F12/Esc/Tab/方向 + 鼠标 5 键 + 手柄按钮；
  `kPhysicalKeyCount` 供数组。
- GameAction：移动/冲刺/跳/蹲/趴/左右探头/互动/换弹/开火/瞄准/近战/武器槽1-3/
  暂停/DevPanel/帮助/对话1-4；`kGameActionCount`。

## 默认键位（Settings::Defaults，与 InputMapper::ResetToDefaults 同源）

WASD 移动；Shift 冲刺；Space 跳；Ctrl 蹲；Z 趴；Q/E 左右探头；F 互动；R 换弹；
鼠标左开火/右瞄准；V 近战；1/2/3 武器；Esc 暂停；F3 DevPanel；F1 帮助；
1-4 对话选项。

## InputState（每帧）

```cpp
struct InputState {
    Vec2 mouse_delta;                 // 原始增量
    Vec2 left_stick, right_stick;     // 手柄（-1..1）
    float left_trigger, right_trigger;
    std::array<bool, kGameActionCount> action_down/pressed/released;
    bool has_focus, ime_composing;
};
```

## 测试锚点

- `player.input_mapper_rebind`：重绑后 MapKey 正确；未映射键 → GameAction::Count。
- `settings.encode_decode` / `settings.key_value_disk`：键位表持久化 round-trip。

## 报告友好

**Design Pattern**: Strategy（后端可换）+ Memento（绑定表持久化）。
**Course Note**: "输入=动作语义，不是键码"——换鼠标方案不碰玩法代码。