# ADR-0009: Character-Art Runtime Reboot

- 状态: 已实现，用户视觉验收待定
- 日期: 2026-09-04
- 决策者: Rainflowers686
- 受影响模块: M2 Render（保留既有 M1/M3/M4/M5/M6 合同）

## 背景

PVS-01 的正常世界路径使用 half-block TrueColor 像素打包和程序化彩色
轮廓。它虽然满足了几何、终端和性能工程门槛，但不符合
WRITEOVER-07 的 Character-Art / ANSI / Unicode 产品方向。

## 决策

- 正常世界直接生成语义 `CharCell` 网格；Height-Span ray geometry 仍是
  深度、可见性和投影的唯一几何来源。
- 新的 Character Renderer 解析有边界的 UTF-8 字符画资产，按材质、距离
  和 LOD 选择 glyph，并使用透明 authored sprites、字符手枪和字符效果。
- B1 资产保存在 `data/characters/b1_character_art.txt`，不引入通用资产
  引擎。Near/Mid/Far 由独立的 authored rows 表示。
- 增加一个有边界的 Full-Human 对话/inspect 肖像绘制候选；它是 CharCell
  overlay，不创建 dialogue engine，也不改变 M6 所有权。
- 原 PVS-01 renderer 仍保留给历史回归测试和对比证据，但
  `composition_root.cpp` 不再把它接入正常世界。
- 普通玩家 HUD 隐藏 preset 和 grid 等工程信息；F3 developer overlay
  仍可显示这些诊断。
- Character Renderer 的公开头文件、HUD 头文件和 legacy renderer 说明
  更新，并同步 public-header hash baseline。

## 约束与验收边界

本决定不扩展 41 层内容，不重做 systemic/save/platform/release foundation，
不创建新分支、PR、release 或 tag。Windows Release C01–C06 帧证据、回归
测试和性能结果只能证明工程路径与当前视觉候选；产品 Gold 仍由 Rain
查看证据后单独验收。

## 回滚/回退

若产品验收否决 Character-Art 细节，后续应在本 ADR 边界内继续迭代
`character_renderer` 与 B1 authored assets；不得用 destructive Git 操作
覆盖 authoritative worktree，也不得移动或改写 `v0.1.0-pvs01-gold`。
