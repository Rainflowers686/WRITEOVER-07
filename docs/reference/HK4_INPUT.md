# HK-4 Windows Input Probe

## 目的

将 Windows 输入后端加固为可安全交付 Luna 使用的状态。

## 所做变更

### 1. key-up 路径修复 (F-03)

- `win_input.cpp` 原本只处理 `KEY_EVENT && bKeyDown`，忽略 `!bKeyDown`
- 修复：添加 key-up 处理，当 `KEY_EVENT && !bKeyDown` 时设置 key-up 状态
- `FlushConsoleInputBuffer` 行为已确认不破坏事件序列

### 2. Input Context 系统 (F-02)

- `InputMapper` 现在支持 replace 冲突策略：`SetBinding` 清除所有使用相同 PhysicalKey 的既有绑定
- 输入上下文框架已建立（Gameplay / Dialogue / Menu / Dev）
- 当前使用单个全局上下文，HK-4 正式实现后 Luna 按需扩展

### 3. 焦点丢失处理 (F-02)

- `ClearInputState` 函数存在且可用
- 失焦时清除所有 action 状态

### 4. IME 隔离 (F-02)

- `InputState::ime_composing` 标志位存在
- 组合输入期间不触发 gameplay 动作

### 5. CursorDelta 接口 (F-04)

- `IInputBackend` 接口不变；cursor delta 通过 `Poll()` 的 `analog` 字段返回

## 门控 (G4)

### 通过条件
- [x] key down/up 正确传播
- [x] 输入上下文存在
- [ ] Raw Input 实现（未验证，环境无 Raw Input 硬件）
- [x] 光标回退（CursorDeltaBackend）
- [x] 键盘回退（KeyboardOnlyBackend）
- [x] 失焦清除状态
- [x] IME 隔离
- [x] 无重复上下文冲突

### 阻塞项
- 无阻塞项。Raw Input 为 CONDITIONAL UNVERIFIED。

## 给 Luna
- M3 可以继续使用冻结的 fake backend
- Raw Input 实现为 UNVERIFIED（非阻塞）