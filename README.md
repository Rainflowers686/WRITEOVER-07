# WRITEOVER-07 · 重写协议：执行官07

一款在终端里运行的单人第一人称游戏。你在设施的 B1 醒来，沿着通行权限、人员口供和记录留下的线索向上走。门可以按规程打开，也可能被绕过；摄像头、枪声和倒下的人会让后面的遭遇发生变化。

墙壁、人物和手里的武器都由字符组成。走近看，能认出门框的线条、护甲的阴影和枪械的轮廓。

[下载 Windows 试玩版](https://github.com/Rainflowers686/WRITEOVER-07/releases/tag/candidate-0.1.0-complete-campaign-20260915) · [玩家指南](docs/release/PLAYER_GUIDE.zh-CN.md) · [本版更新](docs/release/RELEASE_NOTES_0.1.0-complete-campaign-candidate.md) · [反馈问题](https://github.com/Rainflowers686/WRITEOVER-07/issues)

![电梯门与第一人称武器](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/lift_front.png)

*上图来自游戏实际输出的字符单元，经 SVG 导出为预览图。终端字体、窗口大小和颜色设置会影响你在电脑上看到的效果。*

## 先玩起来

当前下载是 Windows x64 的预发布候选版，版本为 `0.1.0-complete-campaign-candidate`。

1. 在下载页的 Assets 中选择 `WRITEOVER-07-audit-candidate.zip`，不要选 GitHub 自动生成的 Source code。
2. 解压整个 ZIP，保留程序旁边的 `data` 文件夹。
3. 运行 `WRITEOVER-07.exe`。第一次选 **New Game**，已有进度时选 **Continue**。

程序在 Windows 10/11 的终端中运行。用支持 Unicode 的等宽字体，把窗口拉大一些；菜单最低需要 48×18 个字符单元。当前包未做代码签名。如果系统提示未知发布者，先核对下载来源和发布页校验值，不必关闭系统防护。

## 你可以怎么玩

你可以先调查设备、读记录、与工作人员交谈，再决定如何通过下一道关卡。开枪能解决眼前的问题，也会留下声音和现场。电击器提供非致命选项，但放倒一个人并不等于没人会发现。

案件档案会显示当前目标、线索、已知证据和通行情况。没看清刚才的提示，可以在暂停菜单里翻最近事件或对话。检视能帮助你辨认眼前的人和物件，但不会直接告诉你所有隐藏条件。

这版有 19 个可玩房间，从 B1 延伸到上层设施及屋顶，包含支路、回访和三种结局。设施目录提到的 41 个楼层是世界设定，**不是 41 个可以进入的关卡**。

## 常用操作

下表是 Windows 默认键位；游戏内 Controls 会显示你当前的实际绑定。

| 操作 | 按键 |
|---|---|
| 移动／观察 | WASD／鼠标 |
| 交互／检视 | F／鼠标右键 |
| 开火／装填 | 鼠标左键／R |
| 手枪／SMG／电击器 | 1／2／3 |
| 快速使用电击器 | V，消耗实际弹药 |
| 冲刺／跳跃 | Shift／Space |
| 蹲下／趴下 | Ctrl／Z |
| 左右探身 | Q／E |
| 案件档案 | F1 |
| 手动保存／读取最近恢复存档 | F5／F9 |
| 暂停／返回 | Esc |

菜单中用 W/S 选择、F 确认，A/D 滚动长文本。右键是检视，不是瞄准镜模式。

## 进度、设置和阅读

Windows 默认把存档放在 `%LOCALAPPDATA%\WRITEOVER-07\saves\`，设置放在同一用户数据目录的 `settings.cfg`，不需要写入游戏安装目录。

手动保存、章节检查点、最终选择前和结局完成后的存档各有用途。暂停菜单可以重启检查点，或回到最终选择前；New Game 会先确认，再重置当前进度。升级前请备份自己的存档，跨版本兼容性没有作无限保证。

Settings 可以调整字幕、文字时长、感知提示详略、高对比度、减弱晃动／闪烁、音量和帧率上限。Frame Limit 提供 Auto、30、60、120；Auto 最高跟随现有 120 Hz 模拟节拍，实际流畅度还取决于终端和电脑。

## 试玩前知道这些就够了

- 当前提供的是 Windows x64 下载。Linux、macOS 有源码构建与 CI 检查，本次没有发布它们的新二进制包。旧 PVS-01 的下载不能代表这版战役。
- Windows 支持原生鼠标与设备可用时的程序化音效；Linux/macOS 的输入和音频体验不等同于 Windows。
- 人物和场景仍是风格化字符美术。真实终端观感、音频试听与首次玩家体验还需要反馈；这里不承诺固定通关时长或实测显示帧率。
- 项目目前没有附带开源许可证。源码可查看不等于已授予任意再分发或商用许可。

遇到问题请在 [Issues](https://github.com/Rainflowers686/WRITEOVER-07/issues) 写明版本、系统、终端、所在房间和复现步骤。有错误文字就原样附上；提交前请遮掉用户名、私人路径或其他个人信息，不要上传整个用户数据目录。

## 想看代码

项目使用 C++17、CMake 和 Python 内容编译工具，运行时没有联网或在线模型依赖。开发环境、构建命令和目录说明见 [开发入口](docs/DEVELOPMENT.md)。

玩家无需先读工程报告。需要核对本版来源时，可以看 [版本与验证记录](docs/release/VERSIONING.md)；源码审计从 [审计清单](docs/production/POST_COMPLETE_GAME_AUDIT_MANIFEST.md) 开始。
