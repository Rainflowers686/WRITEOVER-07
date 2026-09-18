# 开发入口

玩家下载和操作说明在 [首页](../README.md)。这里记录从源码构建与查找实现的位置，不是试玩前置步骤。

## Windows 构建

项目使用 C++17。仓库现有 Windows presets 使用 VS2022；需要相应 C++ Build Tools、Python 3.10+ 和能读取 preset schema 6 的 CMake 3.25+。

当前课程提交线在公开 `v0.2.0-candidate.1` 之后还包含 Windows 中文路径存档兼容性、
空模块占位清理和 GCC 兼容性修正。源码基线的精确 commit、构建包和校验值以最终
交付回执为准；不要把历史试玩包当作当前源码的二进制证明。

从仓库根目录运行：

```powershell
python tools/contentc/contentc.py --data-dir data --out-dir data
cmake --preset release
cmake --build --preset release
.\out\build\release\Release\writeover_app.exe --data-dir data
```

调试版本将 preset 改为 debug，程序位于 `out/build/debug/Debug/`。构建成功只说明这一配置可生成程序，不能替代战役、存档和包验证。

Linux 和 macOS 的 preset 与构建目标见 [CMakePresets.json](../CMakePresets.json)。Windows 的原生输入、音频行为不能直接等同于 POSIX 终端实现。

## 目录与阅读顺序

| 位置 | 内容 |
|---|---|
| include/writeover/ | 公共接口与状态契约 |
| src/app/ | 程序启动、各系统组装、菜单及感知表现 |
| src/core/ | 引擎循环、存档、设置 |
| src/world/、src/player/、src/ai/ | 世界、玩家和 NPC 行为 |
| src/systemic/、src/narrative/ | 持久事实、叙事与条件触发 |
| src/render/、src/platform/ | 字符渲染与操作系统边界 |
| data/ | 作者数据、编译内容和手工字符资产 |
| tools/、scripts/、tests/ | 内容编译、构建打包和验证 |
| docs/production/ | 当前交接与历史证据 |
| docs/adr/ | 接口或约束变更记录 |

开始改代码前读 [AGENTS.md](../AGENTS.md) 及对应模块说明。更完整的系统解释见 [课堂讲解交接](production/COMPLETE_GAME_TEACHBACK.md)，最终审计入口是 [审计清单](production/POST_COMPLETE_GAME_AUDIT_MANIFEST.md)。

公共接口、存档格式和字符渲染路线有项目约束。历史文档里的计划不等于已实现内容，
当前版本范围以及公开试玩版与课程源码版的区别以 [版本记录](release/VERSIONING.md) 为准。

## 验证与发布

默认验证入口是 [FAST_REQUIRED](engineering/TEST_STRATEGY.md)，宽覆盖路线用 EXTENDED。它们包括 CTest、确定性内容、战役、存档故障、包烟测和原负载 benchmark，但不会证明真人观感。不要在仓库根直接运行会写测试 fixture 的单元程序，优先使用配置好的 CTest 工作目录。

版本元数据、手动打包验证与新预发布流程见 [发布说明文档](release/RELEASE_PIPELINE.md)。不得把一次旧运行的通过结果用于证明后续代码改动已经通过。
