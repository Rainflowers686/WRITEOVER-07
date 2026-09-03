# M3_PLAYER_CODEX — Player / Combat / Input

Human Owner: C。角色：locomotion、combat、input 后端。

## Owned / Forbidden

```
Owned:   src/player/, include/writeover/player/**,
         src/platform/windows/win_input.cpp
Forbidden: save 文件写者（core）；render 接口；AI/narrative 模块实现
```

## 关键契约（读这些头）

- `player/controller.h`（单一 LocomotionState 源，无重复 onGround/velocityZ）
- `player/input.h` + `common/input_types.h`（GameAction 层，键位表数据）
- `player/combat.h` + `player/weapon.h`（typed FireRequest/HitscanResult）
- `world/grid.h`（IWorldQuery read-only）

## 不变量

- 姿态/移动/探头正交；净空窗口顺序（prone<stand 高度表）不冲突。
- 输入只以 GameAction 进入 gameplay；RawInput 只换后端。
- combat 全部 hitscan；spread/recoil 只经 sim RNG。
- 碰撞查询覆盖 AABB 所有格子（非中心采样）。

## 施工建议

1. Raw Input 鼠标主后端注册（当前 cursor-delta 为活动后端）；2. 近墙武器
lowered 语义接入 controller；3. 手柄后端（R0 gate 后）。

## 模块 DoD

posture/jump/land 测试、input mapper 测试、combat ammo/reload/hitscan 测试
（本包 8 项全过）、手感调参不改公共 API（只改常量 + ADR）。

## 报告素材

正交 FSM、AABB 碰撞、确定性 spread。UML：Controller↔IWorldQuery↔Combat。