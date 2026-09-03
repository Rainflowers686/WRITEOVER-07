# ADR-0002: Fable Hardening Public Contract Changes

- 状态: 接受
- 日期: 2026-09-03
- 决策者: Fable 5 (Independent Principal Engineer)
- 受影响模块: M2 (render), M3 (player), M1 (integration), M0 (common)

## 背景

Fable 5 独立加固轮发现了 30 个审计发现项，其中部分需要修改公共头文件。
本 ADR 记录所有 public contract 变更及其理由。

## 变更清单

### 1. `include/writeover/player/controller.h`

**新增全局常量**：
- `kMaxStepHeight` (0.35f) — 自动步进最大高度
- `kGroundProbeEpsilon` (0.05f) — 地面接触检测 epsilon

**新增函数声明**：
- `bool GroundProbe(...)` — 地面接触探测（含 jump-ascent 守卫）
- `bool TryStepUp(...)` — 自动步进尝试
- `bool StepDownAvailable(...)` — 下台阶检测
- `bool HeadCollision(...)` — 头部碰撞检测
- `float LeanClamp(...)` — Lean 几何约束
- `float NearWallDistance(...)` — 前方墙壁距离
- `bool VaultEligibility(...)` — 翻越资格
- `bool MantleEligibility(...)` — 攀上资格
- `bool LadderEligibility(...)` — 梯子资格

**理由**：F-16/F-17/F-18 — 控制器缺少 GroundProbe、step-up 和 lean 几何约束。

### 2. `include/writeover/common/math.h`

**新增**：`Vec2 Normalize(const Vec2&)` — 原只有 Vec3 版本。

**理由**：控制器代码需要 Vec2 normalize。

### 3. `include/writeover/render/raycaster.h`

**新增 SegmentFlag**：`SegFloorDrop = 4`、`SegCeilingRise = 5`。

**理由**：F-20 — 双向高度边界需要新的 segment 类型。

### 4. `include/writeover/render/benchmark.h`

**重命名**：`p99_ms` → `worst_1pct_avg_ms`
**新增**：`one_pct_low_fps`

**理由**：F-27 — 旧字段名 `p99_ms` 误导（实际是 slowest 1% 的平均值，不是严格 P99）。

### 5. `include/writeover/render/reference_renderer.h`

**新增文件**：显式字符 3D 参考渲染器接口。

**理由**：F-22 — Render smoke 需要真正可见的 3D 画面。

### 6. `include/writeover/common/serialize.h`

**新增方法**：
- `Serializer::WriteI8(int8_t)` — 带符号单字节写
- `Deserializer::ReadI8()` — 带符号单字节读
- `Deserializer::Skip(size_t)` — 跳过字节（错误时置 error）

**理由**：`command.h` 的 WorldCommand 序列化需要 I8 原语。

### 7. `include/writeover/common/command.h`

**新增**：
- `SerializeWorldCommand(Serializer&, const WorldCommand&)` — 带 tag 的 variant 序列化
- `DeserializeWorldCommand(Deserializer&)` — 按 tag 反序列化
- 显式包含 `serialize.h`（此前仅靠间接包含）

**理由**：F-09 — WorldCommandAction 从 marker-only 升级为 typed payload，
使内容编译器可生产带类型的 world commands。

## 回滚

所有变更可逐项回滚。恢复旧 `p99_ms` 字段名需要同时更新 `benchmark.cpp`、`tools/bench/main.cpp` 和 `test_render.cpp` 中对应引用。恢复 marker-only worldcommand 需要还原 `contentc.py` 与 `storylet.cpp` 的 case 2 分支。