# HK-3 Character Controller Geometry

## 目的

将角色控制器几何学加固为可安全交付 Luna 使用的状态。

## 修复

### F-16 — GroundProbe 与 grounded 稳定性

**问题**：`IntegrateLocomotion` 在 Z 轴 AABB 不被阻塞时无条件设置 `grounded=false`，以及 `TryJump` 后第一 tick 由于 `GroundProbe` 的 epsilon 而重新接地。

**修复**：
- 新增 `GroundProbe()`：检查脚是否在 floor 的 epsilon 范围内
- 新增 `!(ls.velocity.z > kEpsVelocity)` 守卫：上升中（跳起）不重新接地
- 使用 `GroundProbe` 统一确定 `grounded` 状态，不再依赖 AABB 检查结果

### F-17 — 自动 Step-Up

**问题**：水平移动检查 `AabbBlocked` 时将 floor rise 也视为障碍，导致玩家无法进入高台。

**修复**：
- 新增 `IsStepRiseOnly()` 辅助函数：检查 AABB 覆盖的所有 cell，如果唯一的阻塞因素是 floor rise ≤ `kMaxStepHeight`（0.35m），则视为可爬台阶
- X/Y 轴碰撞：当 `AabbBlocked` 但 `IsStepRiseOnly` 返回 true 时，不取消水平移动
- Z 轴 floor snap：如果水平移动后脚在 floor 以下，且上升 ≤ `kMaxStepHeight`，则 snap 到 floor 高度
- `TryStepUp()` 重写为独立辅助函数

### F-18 — Lean 几何约束

**问题**：`SetLean` 只设置枚举值，不检查墙壁。

**修复**：
- 新增 `LeanClamp()`：对给定方向（Left/Right）进行二分搜索，找到不穿透墙壁的最大 lean offset
- 范围 0 到 `kLeanOffset`（0.35m）

### 其他新增

- `HeadCollision()`：检查角色头部是否触及天花板，取消向上速度
- `NearWallDistance()`：射线检测前方近距离墙壁距离
- `StepDownAvailable()`：检查脚下方是否有可下台阶
- `VaultEligibility()` / `MantleEligibility()` / `LadderEligibility()`：纯几何资格查询

## 属性测试

| 测试 | 断言 |
|---|---|
| `controller.grounded_idle_stable` | 120 tick 在平地上保持 grounded，z 不漂移 |
| `controller.step_up_20cm` | 0.2m 台阶可自动步进，脚在 0.2m 处 |
| `controller.step_up_50cm_rejected` | 0.5m 台阶不自动步进，位置 < 3.0 |
| `controller.lean_clamp_against_wall` | 靠墙 lean 被 clamp，离墙方向 full offset |
| `controller.head_collision_stops_jump` | 跳起碰天花板后速度归零 |
| `controller.falling_not_grounded_in_air` | 跳起后在空中不接地，落地后才接地 |
| `controller.no_nan_inf` | 500 tick 随机输入后无 NaN/Inf |
| `controller.save_load_round_trip` | locomotion 状态序列化 round-trip 一致 |

## 门控 (G3)

PASS：
- [x] idle grounded stable
- [x] 0.35m step-up
- [x] >step max rejected
- [x] jump/land stable
- [x] head hit
- [x] posture clearance（现有测试）
- [x] lean wall clamp
- [x] no NaN/Inf property fuzz
- [x] save/load round trip

## 给 Luna

- M3 / M4 依赖此控制器几何接口
- 后续可调手感参数（kMaxStepHeight, kLeanOffset 等），但几何契约不变