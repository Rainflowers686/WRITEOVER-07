# WRITEOVER-07 · 重写协议：执行官07

*用手工绘制终端字符呈现的第一人称沉浸模拟游戏。*

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white) ![Windows x64](https://img.shields.io/badge/Windows-x64-0078D6?logo=windows&logoColor=white) ![Playtest](https://img.shields.io/badge/Status-Playtest-FC6D26)



**导航：**[游戏特点](#它和常见的第一人称游戏有哪些不同) · [下载与开始](#下载与开始) · [常用操作](#常用操作) · [开发者信息](#开发者信息)

[English](README.md) | [简体中文](README.zh-CN.md)

[下载试玩](https://github.com/Rainflowers686/WRITEOVER-07/releases/latest)


一款用字符搭起来的第一人称沉浸模拟游戏。你在地下 B1 醒来，带着编号、一把枪，和一份说不清你是谁的文件。

你可以读门禁为什么拒绝你，翻档案，找值班的人聊两句，用凭证或终端打开下一道门，也可以直接开枪。你在一个房间里做过的事会留下来：摄像头、被发现的身体、升高的警戒，以及某个人的记忆，都会影响后面的门怎么开。

![字符构成的电梯入口，画面右下角是第一人称武器](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/lift_front.png)

*画面来自游戏真实的字符渲染导出。终端里的字体、字号和窗口尺寸会改变实际观感。*

Windows x64 试玩包已发布，内置简体中文与 English。[下载最新试玩版](https://github.com/Rainflowers686/WRITEOVER-07/releases/latest) · [玩家指南](docs/release/PLAYER_GUIDE.zh-CN.md)

## 项目状态

试玩阶段。最新 Release 提供包含英文和简体中文的 Windows x64 版本。

## 它和常见的第一人称游戏有哪些不同

- **整座设施由字符构成。** 门框、控制台、护甲、人的侧脸和手里的枪都是手工排出来的字符素材。颜色区分材质，朝向变化有对应的正面、背面和侧面轮廓。
- **先读懂房间，再决定要不要开枪。** 门禁会给出拒绝的理由，终端会留下记录，值班的人知道一点你不知道的事。你可以先把现场看明白，再决定怎么过。
- **世界会记住你做过什么。** 摄像头、身体、声音、警戒、凭证、NPC 记忆和已走过的路线都会进入后续判断。
- **这里的人各有岗位。** 安保巡逻，清洁工处理现场，维护技术管线路和旁路，医务人员先看人再看编号。他们只掌握自己那一部分信息。
- **旁白有自己的立场。** 它用设施的语言记录你的行为，语气会变，说的话需要和现场、档案、NPC 的说法放在一起判断。
- **地图会逐步打开。** 电梯不是一开始把所有楼层都交给你。Records、Operations、Network、Security、Archive 和 Authority 会随着你留下的事实逐步解锁。
- **同一道门不止一种开法。** 凭证、工作人员协助、终端操作、维护旁路、安静通过，或者武力。
- **三种结局取决于你留下了什么。** 途中取得的证据和做法，决定最后能选什么。

## 你在 B1 醒来

医务人员先确认你能不能站起来。读卡器关心你的凭证。安保在等一个符合流程的回答。没有人急着解释你为什么在这里。

从复苏区往上走，经过校准、医疗和安检，再到档案与调度。到了那里，问题开始变化：一份文件能打开下一道门，却不一定解释发生过什么。有人愿意帮你，也有人只想把这班值完。

战役一路通到屋顶。你要弄清自己的处境，也要决定什么样的版本会被系统留下。

## 先读房间，再动手

第一人称视角告诉你空间：门在哪、谁挡在路上、交火时哪里能躲。文字告诉你这里怎么运转：设备上写了什么、记录里记了什么、眼前的人为什么不肯让开。

进入一个房间，可以先看设备、读提示、找工作人员交谈，再决定要不要拔枪。案件档案会整理当前目标、已知线索、已取得的记录与通行情况。错过一句话，可以打开最近事件和对话回看。

操作是实时键鼠控制，不需要背文字指令。游戏为单人离线设计，不是联机 MUD。

![案件档案界面，显示目标、线索、已取得记录与通行状态](docs/course/assets/case-file-zh.png)

*案件档案中的目标、线索、已取得记录与通行状态。界面为中英双语，图中是中文版本。*

## 世界会记住你做过什么

摄像头有覆盖范围，也可以被打断。同一个人被打晕和被打死，是两种现场。有人发现了身体，事情就不再只是你和一个守卫之间的事。

游戏把这些记成事件：谁看见了、谁听说了、警戒升到哪一级、谁记住了这件事。它们会跟着你离开当前房间。B1 的处理方式可能决定后面哪个部门认得你，也可能让某个检查点和上一次不一样。

清洁工值得多看一眼。你之前怎么对他、走廊里留下了什么，会影响他是替你收尾、叫医疗，还是报告给安保。身体即使被藏起来，也不等于从系统里消失。后续到场的人仍可能发现它，世界会继续沿着这个结果往下走。

![设施走廊中的安保装置与巡逻人员](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/security.png)

## 这里的人有自己的班要上

内容数据里定义了 17 个 NPC，分属 6 个派系、7 类岗位；实际运行时按场景实例化。有人按路线巡逻，有人守着自己的工位，有人只知道和自己职责有关的那部分信息。

他们靠视野和声音判断发生了什么。巡逻中的安保会被响动吸引，会去查看，威胁升级后进入交战节奏。他们的记忆有限，但会用在之后的回应里。同一个事件，对守卫、技术员、医生和清洁工的意义并不一样。

对白是预先写好的，行为由游戏状态驱动。项目章程禁止运行时接入大模型和网络代码，这里没有 ChatGPT 式的自由聊天 NPC。复杂感来自感知、记忆、岗位和规则互相叠加，而不是在线生成台词。

![手工字符素材构成的 NPC 与守卫](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/human.png)

## 旁白不一定站在你这边

> 档案区不问你是谁。它问的是，哪个版本会留下来。

旁白用设施自己的词：许可、流程、登记、归档。它把你的行为整理成它认为合适的说法，也会按自己的判断提醒你。它的叙述有时和刚刚发生的事情对不上。

所有台词都是预先写好的文字，按情境出现，并以文字呈现。当前版本没有配音，也没有用大模型实时生成旁白。你可以把它的话和记录、现场、NPC 的说法放在一起对照。

主菜单上那句 **THE RECORD IS NOT THE EVENT** 不是装饰。整个战役一直在追问同一件事：真正发生过的事，和最后被系统写进记录里的版本，到底是不是一回事。

## 设施不会一次把全部地图告诉你

电梯目录写着 41 层，但当前战役真正可玩的节点是从 B1 到屋顶的 19 个房间。上层的八个主要目的地会按进度逐步开放：

- 1F Arrival / Public Lobby
- 8F Records Core
- 12F Operations Control
- 18F Network Node
- 24F Security Transfer
- 30F Executive Archive
- 36F Authority Core
- Roof / Exit

目录会明确区分 **CURRENT / AVAILABLE / LOCKED / RESTRICTED / SEALED**。你看到的锁定状态不是单纯的菜单装饰，背后读的是已经发生过的事实。

Service Concourse、Records Archive、Power Utility、Observation Gallery 和 Transit Control 也不是一条直线。可以交谈、检查终端、回头找线索、利用凭证、制造或避免噪声，再决定是否把问题升级成武力。部分房间允许回访，之前获得的信息会在后面变成新的路线条件。

## 一个现场，不止一种过法

凭证、工作人员协助、终端操作和维护旁路都能打开特定的门。安静通过、制造噪声、改变现场，在不同情境下会得到不同回应。你也可以选择武力，然后继续面对它造成的局面。

武器有三个槽位：手枪、SMG 和电击器。电击器可以留下活着的人，同时留下需要处理的现场。系统会区分昏迷和死亡，身体是否暴露、是否被发现也会继续影响后续事件。

三种结局不会在开始时全部出现。你取得的证据、走过的路线和留下的后果，决定最后有哪些选择可做。想自己摸索的玩家可以跳过下面的剧透区；结局名称和大致开启条件都放在那里。

## Case File 记录的是你真正知道的东西

案件档案会随着战役变化，不是固定任务清单。它会显示当前 Objective、下一条 Lead、已经掌握的 Evidence、Route 信息、Force trace 和已归档证据数量。

Recent Events 记录玩家实际感知到的重要事件。它不会凭空告诉你另一个房间正在发生什么。Dialogue History 单独保存已经看过的对话，错过一句话不需要立刻读档。

Inspect 也遵守同一条信息边界。你能检查的是当前真正看得见的身体、设备和已经获得的证据，不会因为按了一次检视键就把 NPC 的内部状态或未来结局泄露出来。

## 菜单不是停在游戏外面的另一套世界

启动界面提供 Continue、New Game、Controls / Help、Accessibility / Settings 和 Quit。没有可用存档时，Continue 会直接标成 unavailable。

Pause Menu 会根据当前进度开放或关闭功能，包括：

- Resume
- Manual Save
- Load Last Save
- Restart Checkpoint
- Replay Final Choice
- Case File
- Recent Events
- Dialogue History
- Controls / Help
- Accessibility / Settings
- New Game
- Ending Summary
- Quit

没有 Checkpoint 时会显示 unavailable。还没到最终选择时，Replay Final Choice 不会假装存在。玩家死亡后也不能 Resume 或继续覆盖手动存档。

菜单输入还有额外保护。关闭菜单时，之前按住的鼠标键不会突然变成一枪。窗口失去焦点会暂停；窗口小于 48×18 个字符单元时也会暂停。窗口恢复大小只解除 resize 这一种暂停原因，不会把玩家主动按下的 Pause 一起取消。

## 存档、回退与结局重试

游戏把不同用途的存档分开处理：

- Manual Save
- Chapter Checkpoint
- Pre-Final Save
- Completion / Ending Save
- Latest Resume / Recovery

接近最终选择时会保留独立的 Pre-Final 恢复点。通关后，Pause Menu 可以出现 **Replay Final Choice**，让玩家回到最终决策前，而不是为了重看一个已经解锁的结局从头再打一遍。

成功读取旧存档后，未来时间线产生的临时提示和历史记录会被清除，避免把“还没有发生的消息”带回过去。

读取过程会先把数据放进临时状态，完成检查后再提交到活世界。格式损坏、截断或验证失败时会拒绝这次读取，而不是留下一个只恢复了一半的游戏。文件替换也尽量保留旧的有效存档，不先粗暴删除再覆盖。

New Game 同样不是把坐标传回 B1。它会重新构建运行时世界，清掉旧 facts、NPC memories、objectives、storylet state 和 inventory，同时保留设置与磁盘上的既有存档。

## 有些东西不会主动告诉你

下面是轻微剧透。它们不会阻止正常通关，但愿意观察时间、楼层、摄像头和记录的玩家会看到更多。

<details>
<summary>展开：隐藏路线、延迟后果与彩蛋</summary>

### 02:10 的 Staff Route

B1 有一条具体的维护换班信息：**maintenance shift change at 02:10**。它可以进入玩家知识状态，并参与 Staff Route。一个看起来像背景时间表的细节，实际可能是通行信息。

### Unlisted Observation Route

18F Network Node 会要求玩家寻找一条 **unlisted observation route**。它不是电梯目录里一开始就写给你的正式路线。

### 摄像头盲区会在后面兑现

Observation Gallery 可以让玩家发现并制造 camera blind spot。这个知识不是只用来读一段文本，后续 Transit surveillance 会根据相关状态产生不同结果。

### Power 的噪声会传到 Transit

Power Utility 可以安静配合 Technician，也可以强行 reroute。强行处理会制造 utility noise，后面的 Transit 会读取这条结果，安保响应也会跟着变化。

### 藏起来的身体仍然可能被发现

Hide Body 不是“删除对象”。游戏有身体被藏匿后 Cleaner 到场、继续发现并作出反应的路径。非致命制服和死亡也会留下不同现场。

### 电梯里那些不能去的楼层

目录中的 **B4 SEALED / B3 NO-STOP / B2 RESTRICTED / 04 NO-STOP** 目前不是可玩地图。它们是设施尺度的一部分，也故意给玩家留下一些没有解释完的空间。

### Roof 还有最后一步

到达 Roof 后仍有一个可以交互的最终状态终端，提示是 **READ FINAL STATUS**。结局不是屏幕一黑就把玩家赶回主菜单。

### SUBJECT 07 一直在变

从开场的编号、Records 的档案，到 Executive Archive、Authority Core 和 Ending Summary，游戏不断重写“Subject 07”在制度里的定义。真正的谜题不只是身份，而是谁有权决定哪个版本留下。

### Secret 类型的知识

知识系统里存在 Secret 类型记录。Authority 路线中的部分信息会以秘密知识进入玩家已知状态，而不是和普通提示混在一起。

### 三种结局的大致开启条件

- **AMEND** 是最终决策阶段的基础可选方向。
- **DISCLOSE** 还需要你真正发现 Network 里的信息，并在 Operations 留下合作路线。
- **BREACH** 依赖上层安保推进，以及 force / alert 相关事实。

结局内容本身不在这里展开。游戏会根据你真正做过的事，决定哪些选项出现。

</details>

## 整座设施都是用字搭的

走近看，门框、控制台、护甲、人的侧脸和手里的枪都由字符组成。人物和武器使用手工设计的字符素材，材质靠颜色区分，朝向变化有对应轮廓。画面追求能读懂的第一人称空间，同时让字符本身保持可见。

渲染管线直接输出字符单元，颜色和明暗来自调色板，深度决定用哪个字符。人物还有朝向和距离层级，武器、门框和重要设备也保留自己的字符轮廓。

![从 B1 通向屋顶的战役结尾画面](docs/production/evidence/ultra_visual_20260914/roof-skyline.png)

## 一眼看完

- 19 个可玩房间、32 处已编写场景过渡、3 种当前战役结局
- 41 层世界观目录，8 个上层主要可选目的地随进度解锁
- 内容数据定义 17 个 NPC，7 类岗位，6 个派系
- 状态型 NPC：视野、声音、巡逻、调查、有限记忆和战斗升级
- 摄像头、身体、噪声、警戒、凭证、关系与路线会跨房间留下后果
- Case File、Recent Events、Dialogue History、Ending Summary
- Manual / Checkpoint / Pre-Final / Completion / Resume 多种存档角色
- Replay Final Choice
- 简体中文与 English 内置，可随时切换
- 对比度、减弱晃动与闪烁、感知信息详略、文字停留时长、分项音量、FOV、难度和帧率上限等选项
- 可改键，冲突检查、确认和恢复默认按键
- 手枪、SMG、电击器三个武器槽位

## 下载与开始

最新试玩包在 [Releases](https://github.com/Rainflowers686/WRITEOVER-07/releases/latest)，当前发布标签为 `v0.2.0-course`。

1. 在 Assets 里下载 `WRITEOVER-07-v0.2.0-candidate.1-win-x64.zip`，不要选 Source code。
2. 完整解压，保持 `data` 文件夹与程序在一起，运行 `WRITEOVER-07.exe`。
3. 首次启动先选简体中文或 English，再选“新游戏”。存在恢复存档时可以选择“继续”。

平台：64 位 Windows。程序包未签名，请核对下载来源与包内 `SHA256SUMS.txt`。

<details>
<summary>版本与来源</summary>

- 源码中的 `PRODUCT_VERSION` 为 `0.2.0-candidate.1`。
- 最新发布标签是 `v0.2.0-course`（2026-09-18），指向提交 `d5017c172dcd97aa36d27e7cf75282ed925611ab`；包内 `version.json` 记录同一提交、平台 `windows-x64` 与 CI run `35290223738`。
- 包名沿用候选版前缀，是因为发布流程要求标签、包名与内部版本来自同一来源。
- `main` 上可能有发布之后的文档或兼容性提交。复现课程提交时，以交付回执记录的 commit、源码包和校验值为准。

</details>

## 常用操作

| 操作 | Windows 默认键 |
|---|---|
| 移动 / 观察 | WASD / 鼠标 |
| 交互 / 检视 | F / 鼠标右键 |
| 开火 / 装填 | 鼠标左键 / R |
| 切换武器 | 1 手枪、2 SMG、3 电击器 |
| 案件档案 | F1 |
| 保存 / 读取恢复存档 | F5 / F9 |
| 暂停 / 返回 | Esc |

游戏里的按键提示会读取当前绑定。改键之后，交互、开火、案件档案和加载提示不会继续假装你还在用默认按键。

## 语言、设置与舒适度

简体中文与 English 可以在设置中切换。语言属于展示偏好，不会重置战役事实或重放剧情。

当前产品菜单提供：

- Sensory detail：Off / Important / Detailed
- Text duration：Short / Normal / Long
- Subtitles
- High Contrast
- Reduce Camera Shake
- Reduce Flicker
- Mouse Sensitivity
- Master Volume
- Narrator Volume
- SFX Volume
- Frame Limit：Auto / 30 / 60 / 120
- Language
- Field of View
- Difficulty：Easy / Normal / Hard
- Interaction Emphasis
- Invert Mouse Y
- Rebind Keys

难度主要影响玩家承受的伤害，不改变证据与通行路线。Auto 帧率上限最高跟随 120 Hz 固定模拟节拍，不代表显示器上的实测帧率。

## 试玩前需要知道

游戏包含枪械、致命与非致命战斗、昏迷与死亡、藏匿身体，以及封闭机构中的悬疑情节。

当前下载只提供 Windows x64。Linux 与 macOS 有构建和自动化检查，但没有对应平台的下载包，这些检查也不代表同样的输入或音频体验。没有完整人物配音，没有在线 AI，也没有多人模式。

自动化测试覆盖不了真人手感。第一次试玩、音频听感和通关时长仍需要人工确认。

## 反馈

遇到问题请提到 [Issues](https://github.com/Rainflowers686/WRITEOVER-07/issues)，附上版本、系统、终端、所在房间和复现步骤。如果只是不知道该往哪走，也请说说当时看到的目标和提示。截图与日志请先去掉个人信息。

## 开发者信息

项目使用 C++17。核心运行由固定步长模拟、DDA 光线投射、原生 CharCell 渲染、事件驱动系统玩法、状态型 NPC、数据驱动 facts/storylets 和事务型存档组成。

<details>
<summary>展开：工程与产品完成度</summary>

### 状态与内容

- 当前完整战役资料记录 19 个可玩房间、32 处已编写场景过渡（当前战役显式接入 31 处）、69 个 facts 和 27 个 storylets。
- JSON / text 作为可审阅作者输入，编译成确定性的运行时内容。
- 必需内容、Character-Art 或关键文本缺失时，包验证会失败，而不是静默缺资源继续运行。

### 输入与菜单

- 按键可以按动作重新绑定，冲突会提示，不会悄悄覆盖。
- 方向键保留安全菜单导航，Esc 可以取消改键。
- UI 提示会投影当前真实按键绑定。
- 菜单关闭时会隔离仍按住的输入，避免 Mouse1 从菜单返回后变成意外开枪。
- 失焦和窗口过小使用独立暂停原因。

### 双语与终端排版

- 中英文使用同一份游戏事实，在显示阶段投影为对应语言。
- 中文按照显示列宽处理，不把 UTF-8 字节数当作字符宽度。
- 3D 层保持单宽字符，CJK 双宽字符留给界面合成层。

### 存档

- Manual、Checkpoint、Pre-Final、Completion 和 Resume 角色分开。
- Load 先解析并验证临时对象，再提交活状态；失败可回退。
- 文件替换不依赖先删除旧文件。
- 成功 Load 会清除不属于当前时间线的暂态提示。
- New Game 重建运行时对象，避免旧 NPC memory 或 facts 泄漏进新档。

### 渲染与性能

- 固定模拟与 presentation cadence 分离。
- DDA 负责第一人称空间，Character renderer 输出最终字符单元。
- ANSI frame encoder 支持增量输出；完全不变的帧可以走零输出 fast path。
- 当前性能记录中，内部 Character-Art 核心渲染约为 924 FPS 等效 CPU 吞吐，内部整帧管线约为 697 FPS 等效吞吐，重定向终端提交代理约 113.5 FPS。它们都不是可见终端帧率。模拟节拍为 120 Hz。

### 验证

- 最终 Release 单元测试为 247 / 247。
- Windows、Linux GCC、Linux Clang、macOS ARM64 均有对应 CI 构建 / 测试。
- Linux AArch64 是 link / ELF 架构检查，不等于实体鲲鹏设备测试。
- 还有针对完整路线、Cleaner history、藏尸 / 发现、非致命处理、security bypass、save/load 和结局条件的 deterministic replay / gate。

### 关于底层 Hidden Loop

叙事基础库里还保留一套更一般化的 **TruthBand × DominanceBand 四象限 ending resolver**，以及单独的 **HIDDEN_LOOP / RESIDUAL** 元条件，并有单元测试覆盖。

这不是当前完整战役的第四或第五个可玩结局。当前集成 Campaign 仍然只有 **AMEND / DISCLOSE / BREACH** 三种结局。这里把它写出来，是为了区分“引擎层有的能力”和“当前玩家真的能打到的内容”。

</details>

[开发与构建](docs/DEVELOPMENT.md) · [课程材料](docs/course/README.md) · [版本与来源](docs/release/VERSIONING.md) · [更新说明](docs/release/RELEASE_NOTES_v0.2.0-candidate.1.md)

仓库目前没有开源许可证，公开源码不代表可以再分发或商用。
