# HK-6 Content / Storylet Compiler Runtime Contract

## 目的

将内容编译管线（contentc）与 Storylet 运行时加载合同加固为确定、可验证、可交给 Luna 的状态。

## 修复

### F-08 — Storylet 加载被破坏

**问题**：`storylet.cpp` 的 `Load()` 在定义之后无条件读取 `fired_count` 与 fired id；而 `contentc.py` 生成的 storylets.bin 只含定义（无 runtime 状态）。导致 `LoadBinary` 在文件末尾越界读，`d.HasError()` 触发，NarrativeModule 静默重置为空引擎。

**修复**：`Load()` 现在在读取 fired 状态前检查 `d.Remaining()`：
- 若剩余 ≥4 字节才读 `fired_count`
- 且剩余足够容纳 fired ids 才读取
- 内容文件（无 fired 状态）可干净加载；含 runtime 状态的存档也可加载

### F-10 — contentc `--check` 未实现

**问题**：`--check` 参数存在但从未使用。

**修复**：实现真正的确定性强校验：
1. 编译到临时目录
2. 与现有输出逐字节比较
3. 缺失文件或内容不同 → `CHECK ERROR` + exit 1
4. 全部一致 → `contentc --check: OK (deterministic recompile matches)`

验证：
```
python tools/contentc/contentc.py --data-dir data --out-dir data --check
contentc --check: OK (deterministic recompile matches)
```

### F-11 — RoomId 硬编码 1

**问题**：`body += struct.pack("<Q", 1)` — 每个 room 都是 id 1。

**修复**：RoomId 由编译器按文件排序赋 1..N（与 fact id 策略一致）。`compile_room(json_path, out_dir, room_id)`。

### F-09 — WorldCommand marker-only → typed payload

**问题**：`WorldCommandAction` 只有 marker byte，内容编译无法表达带参命令。

**修复**：
- `command.h` 新增 `SerializeWorldCommand` / `DeserializeWorldCommand`（带 tag 的 variant 序列化）
- `storylet.cpp` Load case 2 读完整 typed command；Save case 2 写完整 typed command
- `contentc.py` worldcommand 分支支持 `setdoor` / `setpower` / `interact` / `usecheckpoint`，tag 与 C++ variant index 对齐
- 新增测试 `storylet.world_command_round_trip`：SetDoor/SetPower 参数 byte-exact round-trip

### F-29 — MapValidator 不可能触发的检查

**问题**：`cell.light > 255` — `light` 是 `uint8_t`，永远不可能为真。

**修复**：改为有意义的 `cell.light < 1`（零光照）警告。

## 仍保留的已知边界

- **Storylet runtime state（fired set）与 definitions 的二进制混合**：
  已做兼容读；Luna 若引入正式 checkpoint 存档，应写独立 runtime section。
- **Two-room synthetic refs 尚未在数据层填充**：M4 内容生产时将建立。

## 测试与验证

- `storylet.selects_highest_priority` PASS
- `storylet.once_fired_skipped` PASS
- `storylet.condition_false` PASS
- `storylet.world_command_round_trip` PASS（typed WorldCommand 参数 round-trip）
- `contentc --check` 通过（确定性 recompile byte-equal）
- smoke 加载 data/storylets/storylets.bin 无错误

## 门控 (G6)

PASS：
- [x] storylets.bin runtime load 成功（不再被越界读破坏）
- [x] runtime state 与 definitions 读取分离（guard + 存档可含 fired 状态）
- [x] typed world commands survive compile/load（Serialize/Deserialize + round-trip 测试）
- [x] global stable IDs（fact id registry + room id 1..N）
- [ ] two-room synthetic refs（数据层尚未生产；Luna M4 内容阶段建立）
- [x] `--check` 检测 drift（temp recompile byte-compare）
- [x] invalid refs fail（stable_id 未注册 → CONTENT ERROR）
- [x] compiled schema == code == docs（--check 保证字节级一致）

## 给 Luna
- M4 / M6 内容生产必须在 `contentc --check` 通过后提交
- WorldCommand typed payload 已实现；新命令类型在 `contentc.py` worldcommand 分支 +
  `command.h` variant 中按 tag 对齐扩展
- Two-room refs 是 M4 数据生产任务
