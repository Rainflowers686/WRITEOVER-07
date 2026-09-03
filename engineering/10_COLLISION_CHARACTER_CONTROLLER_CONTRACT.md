# 10_COLLISION_CHARACTER_CONTROLLER_CONTRACT (REPAIR PASS v2)

> 修订（M-006 closure）：LocomotionState 为**单一状态源**——没有重复的
> `onGround`（由 contact+traversal 表达）、没有 `velocityZ/jumpVelocity` 双垂直
> 速度、没有 `gravityAccumulator`。碰撞检查覆盖 AABB 覆盖的**所有格子**。
> 实现：`player/controller.h/.cpp`。

## LocomotionState（单一状态源）

```cpp
struct ContactState { bool grounded, on_ladder, on_climbable; };
struct LocomotionState {
    Vec3 position;            // feet 位置（世界坐标）
    Vec3 velocity;            // 唯一的合成速度（水平滑移 + 垂直重力都在里面）
    Posture posture;          // Stand/Crouch/Prone（正交轴 1）
    Traversal traversal;      // Grounded/Jump/Fall/Vault/Climb/Mantle（正交轴 2）
    Lean lean;                // Center/Left/Right（正交轴 3）
    float yaw, pitch;         // 摄像机朝向（pitch 受 ±30° 限制）
    uint16_t jump_cooldown_frames;
    ContactState contact;     // 接触描述
    bool IsGrounded() const;  // == contact.grounded
};
```

- 姿态/移动/探头三正交轴可组合（蹲+跳+探头 = 合法）。
- 不存在 `onGround` 与 `Traversal::Grounded` 双写源：着陆时统一由
  `IntegrateLocomotion` 的垂直碰撞分支写入 contact.grounded + traversal。

## 净空与姿态盒

- `GetPostureBox(posture, pos)`：radius 0.35；高 = 1.80/1.20/0.55；脚底在 pos.z。
- 净空窗口（冻结）：prone-only 0.65–0.85 / crouch-only 1.30–1.55 / stand ≥1.90。
- `TrySetPosture`：新姿态盒在**当前脚位**全部覆盖格都 fits 才切换；不满足原地保留。

## 碰撞查询（覆盖所有格子，不是中心采样）

- `CanFit / CanMove / IWorldQuery::AabbBlocked`：遍历 AABB 覆盖的 (col,row)，
  任一实心或垂直区间不符 → blocked。
- 运动积分轴分离（先 X 后 Y 再 Z），贴墙滑动自然；垂直落地 snap 到
  `FloorHeightAt(x,y)`，速度归零，contact.grounded=true。

## 运动参数（冻结）

```cpp
kWalkSpeed 3.2  kSprintSpeed 5.2  kCrouchSpeed 1.6  kProneSpeed 0.8
kGravity 9.8  kJumpVelocity 4.6  kJumpCooldownFrames 30 (0.25s)
kMaxFallVelocity 18.0   // 限制终端下落错误
```

## 测试锚点（tests/test_player.cpp）

`player.posture_clearance`（0.8m 天花仅 prone）、`player.locomotion_orthogonal`、
`player.integrate_move_blocked`、`player.jump_then_land`（~0.94s 弧线后落回）。

## 报告友好

**Design Pattern**: State（正交 FSM）。
**Course Note**: 姿态三轴正交 + 单速度向量 = 大一同学可解释、Codex 不易抄错。