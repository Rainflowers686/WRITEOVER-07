# R0_GATE — R0 技术验证闸门（9/3 22:00）

入口：`pwsh -File scripts/test.ps1 -Gate R0`（文档 + 可执行检查）。

## 可执行检查（当前状态由本机真实运行给出）

- [x] debug configure exit 0
- [x] debug build exit 0（/W4 /WX）
- [x] `ctest --preset debug` → **50 tests, 0 failed**
- [x] `writeover_bench` → raycast p99=0.26ms（240 列）BUDGET=PASS
- [x] `writeover_app --smoke` → exit 0（终端初始化/60 ticks/渲染/原子存档/恢复）
- [x] `mapc` 校验 room_01_calibration.woc → exit 0
- [ ] **Raw Input 鼠标主后端**（当前 cursor-delta 活动）→ M3 施工项
- [ ] **120Hz 实机高刷验证**（≥120Hz 显示器，ULTRA120 preset）→ 需真机
- [ ] **height/span 碰撞边界手测**（低墙/平台/沟槽）→ 需游戏可玩 build

## 文档门槛（技术决策锁定）

- [x] 2.5D 列边界开口 ray 契约（09 文档 + 黄金测试）
- [x] 固定 120Hz Sim、单线程 Sim+Render+Present（06/21 文档）
- [x] terminal 启发式探测 + ANSI TrueColor 路径（12 文档）
- [x] 存档无时间戳 wire + 原子写（08 文档 + 测试）

## 判定规则

全部可执行项 PASS（或显式降级项按 T 节 Kill Gate #24 处理）才放行 9/3 接口冻结。
NOT_RUN 项必须在 WBS 里安排日期，不隐藏。