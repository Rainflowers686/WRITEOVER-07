# MANUAL R0 CHECKLIST — 需人工在真实机器上验证

> 以下项目需要真实物理硬件/前台终端环境，自动化无法覆盖。
> 状态：全部 UNVERIFIED（自动化替代已尽力完成）。
> R0 技术内核已由自动化 gate 验证；此清单供 9/3 22:00 R0 截止前人工确认。

## 输入 (M3 / HK-4)

| # | 项目 | 状态 | 备注 |
|---|------|------|------|
| 1 | 键盘 WASD + 空格 + Shift 手感 | UNVERIFIED | 需前台终端实测 |
| 2 | Alt+Tab 失焦再回焦后键不粘滞 | UNVERIFIED | 需人工切换窗口 |
| 3 | 中文 IME 打开时打字不触发游戏动作 | UNVERIFIED | 需实际输入中文 |
| 4 | 鼠标 Raw Input 相对位移 | UNVERIFIED | 无 Raw Input 设备可测 |
| 5 | 触控板/1000Hz 鼠标 | UNVERIFIED | 需对应硬件 |

## 终端 (M2 / HK-5)

| # | 项目 | 状态 | 备注 |
|---|------|------|------|
| 6 | Windows Terminal 全屏无闪烁 | UNVERIFIED | 需真实前台终端 |
| 7 | 120Hz 下无 Present backlog | UNVERIFIED | 需 >=120Hz 显示器 |
| 8 | 退出后终端恢复（光标/颜色） | UNVERIFIED | 需运行后 Ctrl+C 退出 |
| 9 | conhost 回退路径正常显示 | UNVERIFIED | 需 conhost 环境 |

## 玩法 (M3 / HK-3)

| # | 项目 | 状态 | 备注 |
|---|------|------|------|
| 10 | 0.2m 台阶自动步进手感 | UNVERIFIED | 自动化已覆盖几何 |
| 11 | 跳+蹲+趴+探头组合无卡角 | UNVERIFIED | 需人工走位 |
| 12 | 翻越/攀上候选出现提示 | UNVERIFIED | eligibility 逻辑已测 |

## 存档 (M1 / HK-1)

| # | 项目 | 状态 | 备注 |
|---|------|------|------|
| 13 | 游戏中存档→读档→回到原场景 | UNVERIFIED | MiniWorld 已测等价性 |
| 14 | 读损坏存档拒绝且不崩溃 | UNVERIFIED | 自动化已测拒绝路径 |

## 内容 (M6 / HK-6)

| # | 项目 | 状态 | 备注 |
|---|------|------|------|
| 15 | storylets.bin 加载后叙事触发 | UNVERIFIED | 自动化验证加载成功 |
| 16 | 两房引用解析 | UNVERIFIED | 数据层尚未生产两房 |

## 结论

- 自动化可验证项：全部 PASS（见 FABLE_HARDENING_VALIDATION.md）
- 人工项：全部 UNVERIFIED，均为物理交互/前台终端类，不影响
  `SIX_LUNA_PARALLEL_READY = YES` 判定（判定只要求技术内核就绪）
