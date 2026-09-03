# M1_CORE_CODEX — Core / Integration / Release

Human Owner: A。角色：引擎调度、存档、设置、profile、console guard、compose root。

## Owned / Forbidden

```
Owned:   src/core/, src/app/, scripts/, tools/contract_check/, CMakeLists.txt,
         CMakePresets.json, .github/workflows/ci.yml, include/writeover/core/**
Forbidden: src/{world,player,ai,narrative,render}/**, 其它模块公共头改动
```

## 关键契约（读这些头）

- `core/engine.h`（EngineContext / IEngineModule / Engine::Run 固定步长）
- `core/save.h` + `core/profile.h`（无时间戳 wire、独立 profile）
- `core/settings.h`（键位绑定表数据，无指针）
- `core/console.h`（正常退出恢复保证；SEH 属 release gate）
- `common/clock.h`（SimClock 120Hz）

## 不变量

- Engine 只调度：core 绝不 include 语义模块头；组合根是 `writeover_app`。
- 存档字节级确定性（无墙钟）；原子写经 platform provider。
- Release /MT；debug/release/ci presets 不漂移。

## 交付节奏

- 9/3 接口冻结：公共头 + save schema 落定（本包已交付，冻结期只做 bug fix）。
- R0 gate 由 M1 跑 `scripts/test.ps1 -Gate R0`。
- 9/15 release gate：package.ps1 产物 + SHA256。

## 模块 DoD

引擎 loop 120Hz 实测、存档 round-trip/损坏测试、settings/profile 测试、
smoke exit 0（本包证据：50 tests 0 failed / smoke exit 0）。

## 报告素材

MainLoop（固定时间步长）、SaveManager（CRC+原子写）、Settings Registry。
UML：Engine ↔ modules；序列图：tick 8 phases（见 07 文档）。