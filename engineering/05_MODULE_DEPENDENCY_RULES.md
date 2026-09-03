# 05_MODULE_DEPENDENCY_RULES (REPAIR PASS v2)

> 修订（M-002 closure）：明确 Composition Root = `writeover_app`；`writeover_core`
> 只依赖 common；render/ai/narrative 只通过只读查询接触 world/player。
> 依赖表与 `repo_seed/CMakeLists.txt` target_link_libraries 一致，
> `tools/contract_check/check_deps.ps1` 对 `#include <writeover/...>` 做机器扫描。

## Composition Root

```
writeover_app (exe, src/app/) —— M1 集成 Owner 拥有
  ├─ 创建 EngineContext（clock/events/rng/settings/metrics/logger/data_dir）
  ├─ BuildGame：world→player→ai→narrative 实例化并按序注册 Engine
  ├─ ProbeTerminalEnv → CreateTerminalBackend → RenderModule
  └─ RunComposition 驱动 Engine::Run；smoke 模式额外做原子存档
```

core 内 Engine 只做**调度**（IEngineModule 列表 + 固定步长累加器），不链接语义模块。

## Frozen DAG（include 层机器可查）

| Module | May include (`<writeover/…>`) | Must NOT |
|--------|-------------------------------|----------|
| common | — | anything |
| world | common | core, render, player, ai, narrative, platform |
| player | common, world | core, render, ai, narrative, platform |
| ai | common, world, player | core, render, narrative, platform |
| narrative | common, world, player, ai | core, render, platform |
| core | common | world, player, ai, narrative, render, platform |
| render | common, world, player | core, ai, narrative, platform |
| platform | common, core, world, player, ai, narrative, render | — (edge) |
| app | all (composition) | — |

## 强制点

1. CMake 层：target_link_libraries 白名单（链接越界即链接失败）。
2. 脚本层：`check_deps.ps1` 扫描 `#include <writeover/xx/`，越界报错（CI gate）。
3. 审查层：Codex/PR 必须过依赖表检查；`windows.h` 只允许在 `src/platform/windows/`。

## 语义规则

- 跨模块"读"一律走 `IWorldQuery`/快照接口，永不产生事件。
- 跨模块"写"一律走 typed `WorldCommand` → world 权威系统校验应用 → 发 WorldEvent。
- render 只读快照，AI/叙事只读查询；平台永不反向 include 语义核心。
- `writeover_platform` 实现 input/terminal/audio/原子文件接口；app 做装配。

## 报告友好

**Design Pattern**: Layered Architecture + Composition Root。
**Course Note**: 6 模块并行施工互不破坏的前提：依赖方向冻结 + 组合根唯一。