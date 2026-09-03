# 第三轮报告后的交叉审查与修正（作为 P0-v1.0 的技术/制作修正）

第三轮 Claude 的大方向被接受，但以下问题不能原样交给开发 Agent。

## 1. 所有正常画质预设保持同一 Simulation Rate

禁止：
- ULTRA120 Sim=120
- COMPAT Sim=90

最终：
- 正常预设全部 `Simulation = 120 Hz`
- 只降低 Render / Present
- 只有极端应急模式才允许 60Hz Simulation，并且不作为正常设置暴露

原因：否则不同画质预设会改变物理、AI、跳跃积分、事件时序和确定性。

## 2. Height-Aware Grid 必须升级为 Height/Span Raycaster

不是传统“一列射线打到第一个墙就结束”。

每条 DDA 射线可以产生多个可见垂直 span：
- floor height 上升 → 前壁 span
- ceiling height 下降 → 上壁 span
- 低墙 / 高台 / 沟槽 / 低天花 → 正确影响视线

R0 必须证明：
- 站立能看见低墙后 NPC
- 蹲下后低墙遮挡视线
- 高平台正确抬升
- 沟槽正确下沉
- 低天花在姿态切换后正确显示

## 3. 姿态尺度修正

冻结初始值（可在 R0 手感校准中小幅调整，但区间不允许冲突）：

- Stand collider height = 1.80m
- Crouch collider height = 1.20m
- Prone collider height = 0.55m

- Stand eye = 1.60m
- Crouch eye = 1.00m
- Prone eye = 0.42m

净空区间：
- Prone-only = 0.65–0.85m
- Crouch-only = 1.30–1.55m
- Standing = ≥1.90m

## 4. 跳跃距离由物理推导，不写“≤2格可跳”

建议初始参数：
- vertical jump velocity = 5.0 m/s
- gravity = 22 m/s²
- sprint speed 约 6 m/s

实际沟槽宽度由地图 Bible 定：
- safe ≈1.5m
- hard ≈2.0m
- limit ≈2.3m

R0 通过后以真实轨迹测量再冻结。

## 5. Camera Hitscan 保留，但加 Near-Wall Weapon Handling

P0：
- 命中判定从摄像机/视点中心发射 hitscan
- 近墙时武器下压/收起
- 极近墙禁止 ADS
- 极近墙禁止或延迟射击
- 枪模不可明显穿墙

P1 才考虑 camera aim point + muzzle ray 的双射线模型。

## 6. R0 不预先复杂多线程

先：
- 世界模拟 + Render 单线程
- Audio 独立
- Present 先同步

Profiler 证明 Present 阻塞后：
- 加 latest-frame mailbox + Present thread

Profiler 证明 Raycaster 超预算后：
- 才按列并行 Render

原则：**先 profiler，再并发**。

世界逻辑始终单线程。

## 7. 输入后端必须三层 fallback

Primary：
- Raw Input relative mouse
- message-only / hidden window
- focus gate

Fallback：
- cursor delta + center recapture

Fallback 2：
- 纯键盘 Q/E / 方向键旋转

R0 必测：
- Alt+Tab
- 失焦
- Windows Terminal
- conhost
- 中文 IME
- 1000Hz 鼠标
- 触控板
- 崩溃退出恢复

## 8. 音效不强制全部程序合成

可程序生成：
- UI
- 系统提示
- 简单电子提示音

重要 SFX：
- 枪械
- 门
- 金属环境
- 脚步
- 警报

来源允许：
- 自录
- CC0
- 明确允许再分发的素材

必须维护 `ASSET_PROVENANCE.csv`。

## 9. 旁白正式改为 ONE VOICE IDENTITY × THREE DSP/PERSONA STATES

不是三个独立配音演员。

- Normal / Guide
- Authority / Director
- Corrupted Echo / Residual

叙事收益：玩家逐渐发现“多个声音状态其实来自同一个系统”。

## 10. 时间预算修正

第三轮报告的房间分钟数最少已经约 30 分钟，却写完整通关 22–28 分钟，存在算术冲突。

最终目标：
- 正常首次通关：25–30 分钟
- 探索型玩家允许超过 30 分钟

建议房间预算：
- R1：4 min
- R2：3 min
- R3：3–4 min
- R4 旗舰：5–6 min
- R5：4 min
- R6：4 min
- Transit/micro：2 min

## 11. 设置数量改成结构，不死守“14这个数字”

最终：
- 14 个主设置行
- Accessibility 子页 3 个开关
- Developer 页独立

不为了凑 14 删除真实必要设置。

## 12. R0 技术盒改为统一高风险验证场

不要做完整第一关。

技术盒依次包含：
- 入口走廊
- 低墙
- 蹲伏门
- 趴伏管道
- 跳跃沟槽
- 翻越障碍
- 梯子/平台
- 1 NPC
- 1 扇会因事件锁上的门

统一验证：
Renderer / Height Span / Collision / Mouse / Weapon / NPC sensing / WorldEvent / Narrator / Save / Performance。
