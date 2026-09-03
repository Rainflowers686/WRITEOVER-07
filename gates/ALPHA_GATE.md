# ALPHA_GATE — 首个房间可玩（对齐首条通关路线打通）

入口：`scripts/test.ps1 -Gate Alpha`。

## 目标定义

Room 01（校准靶场）完成首条 beat：出生 → 移动/观察 → 开枪 → 遇工程师 →
旁白首句 → 因果回调可见（F3 可证明）。

## 可执行检查

- [x] room_01_calibration 内容 + mapc exit 0（16×12 网格：低墙/平台/低空区域实例）
- [x] 控制器基础（移动/跳跃/蹲/趴）单测全过
- [x] combat hitscan + ammo/reload 单测全过
- [x] storylet typed 管线 + smoke 加载路径 exit 0
- [ ] 第一人称可玩帧循环（交互输入→命令→mutation→渲染）→ M1/M3/M2 集成
- [ ] F3 Dev 面板在此 room 内实时可读 → M1/M2

## 未实现 = NOT_READY（如实）

首条完整 beat 属 9/11 FULL_CLEAR 前置；本 gate 的"可玩"依赖六路施工后集成，
foundation 阶段不硬编码 PASS。

## 判定

代码可查项全绿 + 交互 beat 手测通过 = PASS；否则砍次要路线（T 节 #26）。