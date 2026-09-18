# WRITEOVER 07 课程设计报告

## 项目概述

WRITEOVER-07 是 C++17 单机 Character-Art 第一人称探索与战斗游戏。玩家在 B1
醒来，通过交互、调查和通行决策抵达屋顶。当前战役有 19 个可玩房间、32 条场景
转移和三种结局。目录中的 41 个楼层属于世界设定，不等于 41 个可玩关卡。

本项目用字符单元组织空间轮廓，并以颜色补充材质和提示语义。NPC、枪械和重要
物件使用手工设计的字符资产，没有把普通图片量化成字符像素。交互和叙事保留
文字游戏的阅读、选择与状态反馈，操作采用第一人称移动和目标选择。

课程依据为同目录《C++课程设计实验大纲26新.pdf》第 7、8、9、17、18 页：
面向对象分析设计、STL、存取进度、新游戏、彩色控制台、跨平台编译说明、图示
设计表达、测试报告和成员职责。本项目没有多人联机、商店经济或升级树，不将
大纲中的例举属性误写为已经实现的系统。作品含战斗与身体处置，课堂演示应优先
展示调查、非致命选择和后果承担，内容适宜性最终由任课教师判断。

## 需求与验收范围

核心用例包括启动新游戏、继续、探索房间、检视、交互、操作终端、使用凭证、
战斗、查看案件档案、使用电梯目录、保存读取、最终选择和调整设置。
分析图源见 [用例图](diagrams/use_cases.mmd) 与 [WBS](diagrams/wbs.mmd)。

玩家需要知道当前能做什么、为什么某条路不可用，以及失败后如何恢复。因此
产品层提供当前绑定提示、滚动长文本、已知记录、分别命名的恢复槽位和中英文
切换。不会把所有结局条件提前作为攻略显示，也不会把 NPC 隐藏知识当成检视结果。

非功能要求包括固定步长模拟、有界资源解析、确定性内容编译、失败不破坏旧存档、
低分辨率文字可读性、独立用户数据目录和跨平台构建。自动门槛与真人感受分别
验收；测试通过不是视觉、音频或首次玩家体验通过的替代品。

## 工作分解

WBS 按已完成工作的责任领域分解，不倒推或编造成员历史工时。

| 工作包 | 实际产物 | 主要代码或数据 |
|---|---|---|
| 引擎与平台 | 固定步长调度、输入和终端后端 | src/core/engine.cpp、src/platform |
| 世界与内容 | 房间网格、实体、转移、内容编译 | src/world、data/rooms、data/scenes、tools/contentc |
| 玩家与战斗 | 移动姿态、射击、弹药、装填、电击 | src/player、src/ai/runtime.cpp |
| AI 与系统状态 | 巡逻、感知、记忆、身体和警戒后果 | src/ai、src/systemic |
| 叙事 | 有条件的 storylet、对话队列、结局资格 | src/narrative、tower_campaign_runtime.cpp |
| 字符渲染 | DDA、投影、朝向、武器、HUD | src/render、data/characters |
| 产品体验 | 菜单、改键、语言、感知记录、开场提示 | src/app/player_product.h 等私有组件 |
| 存档 | 分段编码、校验、原子替换、运行态回滚 | src/core/save.cpp、src/app/player_save.h |
| 验证与发布 | 分层 QA、干净包、元数据与 CI | scripts/run_qa.ps1、tools/release、.github/workflows |
| 课程交付 | 报告、图源、测试记录、演示与成员确认 | docs/course |

## 面向对象架构

`game_main.cpp` 负责启动及 New Game 重建循环；`RunComposition` 是应用组装点。
它创建模块并连接回调。`Engine` 只认识 `IEngineModule` 和 `IRenderModule`，
不直接依赖具体房间、NPC 或故事。引擎按注册顺序调用固定步长更新：Input、
Player、World、AI、Narrative；画面呈现另由 `IRenderModule` 调用。

当前生产模块 `WorldModule`、`PlayerModule`、`AiModule`、`NarrativeModule` 和
`RenderModule` 的组装实现位于 `src/app/composition_root.cpp`。历史目录中曾保留
同名的空占位文件；本轮已删除它们，生产回调应以 `composition_root.cpp` 的注册和
连接关系为准。
[类关系图](diagrams/architecture.mmd) 只画主要关系，避免把所有结构体挤在一张图里。

| 类型或组件 | 责任 | 依赖方向 |
|---|---|---|
| Engine | 固定 120 Hz 调度、呈现节拍、退出 | 抽象模块接口与 common |
| PlayerModule | 玩家控制器、战斗和当前房间状态 | player 与世界查询接口 |
| WorldModule | 当前网格、世界事实和房间读取 | world 与内容编解码 |
| AiModule | 为 AutonomousNpcSystem 提供当前房间、玩家姿态与事件 | AI runtime、SystemicWorld、查询接口 |
| NarrativeModule | 故事条件、队列和呈现文本来源 | narrative、事实读取 |
| RenderModule | 生产画面、HUD、产品叠层和终端提交 | render、terminal backend、只读状态来源 |
| SystemicWorld | 实体、关系、知识、身体等持久状态 | common 标识与事件 |
| TowerCampaignRuntime | 有界电梯目的地、目标和结局资格 | 注入 FactReader，不拥有第二份事实库 |
| SaveManager | 文件封装、分段校验与原子保存 | common IO 与序列化 |

组装文件目前约 6873 行，集中了回调、存取档和回放诊断，是明确的维护风险。
本轮没有为了报告美观重写架构。私有产品组件降低了部分复杂度，但不能据此称
整个应用已完成彻底模块化。

## 核心算法一 字符空间的射线步进

`CastColumnRay` 在二维网格上使用 DDA。根据相机方向计算下一条 x/y 格线的距离
以及每跨一格的增量，选择较近边界前进。两轴距离在容差内相等时同时推进，
避免经过网格角点时凭空访问仅角接触的中间格。

每个边界比较两侧地板、天花板和实体墙状态。整墙终止该射线；高度变化生成
相应遮挡段，继续追踪后方空间。越界按实墙处理。遮挡段数组有容量上限，溢出
会标记 truncated，不写越界内存。每列工作量随所穿越格数增长，而不是为每个
屏幕字符扫描整个地图。图源见 [DDA 流程](diagrams/raycast.mmd)。

投影考虑字符单元的长宽比。人物根据观察者与 actor yaw 的关系选 Front、Back、
SideLeft、SideRight，再按距离选择 LOD。侧身是独立资产，不把正面镜像冒充侧面。
门保持世界平面、深度和建筑开口关系。Unicode 宽字符仅在文字合成层分配双列，
没有把场景变成 half-block 或 Braille 光栅。

## 核心算法二 存档事务

外层 `.wo07` 格式版本为 1，头部 24 字节，每节头部 12 字节，并校验节 CRC 和
整体校验。枚举定义了八种节 ID，当前生产写入七节：Player、World、Rng、Events、
Ai、Narrative、Systemic。不能把枚举中的 SettingsGameplay 当成当前实际写入节。

Player 私有负载有 `PLY2` 标记和版本 1，验证健康／死亡一致性、有限数值、枚举、
布尔值及尾部完整性。保留对上一版完整无标记负载的读取，不接受含糊的缺尾文件。
剩余冷却时间在读入时相对当前游戏帧重建，避免把旧时钟绝对值带入新运行。

保存先写临时文件，再调用原子替换。POSIX 使用 rename-over-existing，不先删除
旧文件；Windows 生产路径安装 MoveFileExW 提供者。这里保证正常替换失败时旧
文件不被预先删除，不宣称对任意断电和文件系统都具有未经验证的耐久性。

读取先解析和暂存各节，再验证实体和房间，最后提交。提交阶段失败时恢复原
运行态快照并校验回滚。成功后清除暂态感知和菜单显示，防止未来信息穿越存档。
主槽位和最近恢复文件分别原子写入，不是跨文件事务，部分失败必须如实提示。
图源见 [存取档事务](diagrams/save_transaction.mmd)。

## AI 系统后果与结局

`AutonomousNpcSystem` 按当前房间更新感知、噪声、调查、巡逻和战斗。行为有
状态、记忆和有界导航；这不是完整战术小队 AI 或跨整个设施的实时追击模拟。
身体被发现、凭证、摄像头和枪声通过事实或系统事件影响后续读取者。

`TowerCampaignRuntime::EligibleEndings` 直接读事实。AMEND 要求 authority_ready；
DISCLOSE 还要求 network_discovered 与 operations_cooperated；BREACH 要求
transfer_reached，并有 force_route 或 security_alerted。已完成战役不再返回
可选结局。这样存档恢复的是同一组资格事实，不另存一份可能过时的菜单选项。

案件档案中的新关键记录从 `KnowledgeAsset.known_by` 投影，只显示玩家实际获得
的记录。感知历史是有界表现队列，不是任务历史权威，关闭它不会撤销事实。

## 中英文与产品体验

配对资源以稳定 ID 管理，加载时检查 ID 一致和模板合法性。事件与对话保留
规范文本，绘制时投影为当前语言，因此切换语言不需要重触发事件。中文按显示
列剪裁和换行，CharCell 的宽字符头／续列标记让 ANSI 和 Win32 输出保持一致。
这不包含通用 emoji／复杂字素排版保证。

终端尺寸刷新后，同时更新画面与交互投影尺寸。过小窗口使用独立暂停原因；
扩大只解除这一原因，不把暂停菜单也强制关闭。改键通过新按键捕获、冲突检查、
确认再保存，方向键和 F/Esc 保留安全导航。设置真实消费者见
[设置清单](../engineering/SETTINGS_INVENTORY.md)，没有消费者的旧兼容字段不宣传。

## STL 与 C++ 设计方法

| 组件 | 本项目实例 | 选择理由和约束 |
|---|---|---|
| std::vector | SaveSection 数据、NPC 列表、电梯目的地 | 连续存储与顺序遍历；解析前验证数量边界 |
| std::array | 上下文按键表、翻译模板四个捕获槽 | 固定容量，表达编译期上限 |
| std::deque | PerceptionFeed 的最多 48 条消息 | 尾部追加、头部淘汰，避免无限增长 |
| std::sort | runtime.cpp 的 NPC 按标识排序 | 稳定遍历来源，减少容器插入顺序影响 |
| std::stable_sort | 翻译模板按文字特异性排序 | 同级保持作者顺序，防止短后缀抢先匹配 |
| std::clamp | 难度伤害和显示偏好范围 | 显式限制数值，不用来掩盖损坏存档 |
| 迭代器与范围循环 | NPC 排序及容器遍历、存档字节比较 | 容器与算法配合，不自行维护裸数组索引体系 |

多态接口把调度与具体模块隔开，`unique_ptr` 表达运行时所有权，回调表达注入的
查询和动作。Composition Root 是组装方式；EventBus 具有观察者式发布／订阅；
平台 IO 提供者是策略注入；NPC 使用状态导向行为。这里不把每个类硬套成设计模式，
也不把 `WorldAction` 一类命令式数据自动宣传为完整可撤销 Command 框架。

## 工程问题与处理

| 问题 | 实际处理 | 验证边界 |
|---|---|---|
| 替换存档时预删除旧文件 | 改为原子替换，不先 unlink | 失败保留旧文件测试；真实断电未做 |
| 半截 Player 数据被当作旧版 | 带标记私有负载与完整旧版读取 | 缺尾、非法数值、战斗状态拒绝测试 |
| 关闭菜单后按住鼠标误开火 | 输入上下文与释放隔离 | 产品／输入测试，实际改键回放 |
| 朝向名与屏幕侧向混淆 | 从 yaw、观察者、资产选择核对 | 36 个精确资产映射，非美感评分 |
| 门像悬浮贴片 | 墙开口、门平面与深度组成 | 生产导出预览，真人多角度另验 |
| 终端跨行颜色状态泄漏 | 明确初始化并携带 SGR 状态 | 完整／增量／不变帧测试 |
| 呈现间隔重复收费 | 独立呈现截止时刻，错过周期跳过 | 相同负载 benchmark；不宣称实测显示 FPS |
| 中文错列与缩放错位 | 显示列合成、宽头续列和共享画布尺寸 | 单元与生产导出；实际字体仍需人验 |

## 验证与交付

默认 FAST_REQUIRED 保护完整主线、产品、存档、包和性能。EXTENDED 保留恢复、
场景和全结局宽覆盖；不在每次提交重复所有路线。跨平台 CI 区分 Windows 产品
回归、GCC/Clang/macOS 原生检查和 Linux ARM64 交叉链接。参见
[验证策略](../engineering/TEST_STRATEGY.md)。最终测试报告必须引用本版回执，
不得把旧 PVS 的结果当成本轮结果。

课程测试按大纲分别记录代码质量 50 分和功能体验 50 分，由对方小组填写。
自动化结果不能代填评分。报告、演示及真人清单共享同一个事实边界，不保证课程
成绩。姓名、学号、组号、出勤和实际分工由成员确认，不根据提交日志推断个人贡献。

## ARM64 编译说明

从 x86 Windows 可以在 WSL Linux 环境使用 `aarch64-linux-gnu-g++` 交叉工具链。
在仓库中运行 `cmake --preset linux-arm64-link`，再执行
`cmake --build --preset linux-arm64-link --parallel 2`。
工具链定义位于 `cmake/toolchains/aarch64-linux-gnu.cmake`，将目标设为 Linux/aarch64，
使用交叉 GCC/G++ 并隔离目标库查找。此方法说明不等于本机已经完成 WSL 配置。

CI 在 Ubuntu 安装交叉编译器后执行相同步骤，并用 `file` 检查 ELF 架构。
目标是 Linux ARM64 ELF，不是 Windows ARM64 PE。部署鲲鹏设备还需匹配目标
libc／动态库、复制运行时 data 并在目标终端测试。没有鲲鹏真机运行证据，故只
报告交叉编译／链接／ELF 检查。课程第 9 页要求编译说明，不强制实现真机部署。

## 成员确认

成员职责表见 [MEMBER_RESPONSIBILITIES.md](MEMBER_RESPONSIBILITIES.md)。未确认
字段保持 NEEDS_USER_INFO。课堂可能随机抽取成员，建议每人能解释引擎调用方向、
一条交互后果链和存取档失败处理，而不是只记住演示操作。
