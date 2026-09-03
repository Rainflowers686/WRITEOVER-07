# 03_BUILD_TOOLCHAIN (REPAIR PASS v2)

> 修订：原版只有示例 Presets 且缺 testPresets；无真实 CI 内容编译步骤。此版与
> `repo_seed/CMakePresets.json` / `.github/workflows/ci.yml` / `scripts/` 一一对应。
> 构建证据见 `evidence/FOUNDATION_VALIDATION.md`（本机实跑全部通过）。

## Frozen Toolchain

| Item | Choice | Rationale |
|------|--------|-----------|
| CMake | ≥3.20 (locked 3.29.2 实测) | presets 支持；VS2022 Build Tools |
| C++ Standard | **C++17** | /std:c++17；无 C++20 特性（`std::span` 为 C++20，禁用） |
| Compiler | MSVC v143 (VS2022 Build Tools) | 课程标准工具链 |
| Warnings | `/W4 /WX` | warnings-as-errors，实测 0 警告 |
| Source charset | `/utf-8` | 源码/运行字符集 UTF-8 |
| Runtime | Debug `/MDd`；Release `/MT` | 由 preset 变量 `CMAKE_MSVC_RUNTIME_LIBRARY` 设置 |
| Conformance | `/permissive-` + `/EHsc` | 严格模式；我们的代码不 throw（见 00/20） |
| Architecture | x64 (VS generator `architecture: x64`) | `/arch:AVX2` 不在此锁定——由 profile 后决策 |

## CMake Presets (repo_seed/CMakePresets.json)

- `configurePresets`: `debug` (MDd+WO_ENABLE_DEV), `release` (MT 静态运行时),
  `ci` (MDd+WO_ENABLE_DEV)
- `buildPresets`: debug / release / ci（各自带 `configuration`）
- `testPresets`: debug / ci（`outputOnFailure`）
- 生成器固定 `Visual Studio 17 2022`，单配置由 `--config` 选择。

## CMake Targets（真实存在并可链接）

| Target | Type | Depends On |
|--------|------|-----------|
| `writeover_common` | static | (none) |
| `writeover_world` | static | common |
| `writeover_player` | static | common, world |
| `writeover_ai` | static | common, world, player |
| `writeover_narrative` | static | common, world, player, ai |
| `writeover_render` | static | common, world, player |
| `writeover_core` | static | **common only**（引擎/存档/设置/profile） |
| `writeover_platform` | static | common, core, render, player, world, ai, narrative |
| `writeover_app` | exe | core+world+player+ai+narrative+render+platform |
| `writeover_tests` | exe | common/core/world/player/ai/narrative/render |
| `writeover_bench` | exe | common/world/player/render |
| `mapc` | exe | common, world |
| header 自检目标 | OBJECT×N | 每个 public header 独立编译通过 |

## 编译命令（实测 exit 0）

```powershell
cmake --preset debug
cmake --build --preset debug            # → out/build/debug/Debug/*.exe
ctest --preset debug --output-on-failure
cmake --preset release
cmake --build --preset release
python tools/contentc/contentc.py --data-dir data --out-dir data
```

## CI (.github/workflows/ci.yml)

windows-latest 上：setup-python → `contentc --check` 编译内容 →
`cmake --preset ci` → build → `ctest --preset ci` → `mapc` 校验 .woc →
`writeover_app --smoke` → forbidden/deps 扫描。PR 非绿不得合入。

## 警告与第三方

- `/W4 /WX`；第三方依赖 = 0（vendored header = 0，`SYSTEM` 抑制无对象）。
- `NOMINMAX`/`WIN32_LEAN_AND_MEAN`/`_CRT_SECURE_NO_WARNINGS` 为全 target 定义，
  `windows.h` 只在 `src/platform/windows/` 出现（check_forbidden 自动验证）。

## 报告友好

**STL**: 无第三方构建库；纯 CMake。
**Design Pattern**: Build Pattern（目标拆分）。
**Course Note**: Release `/MT` → 单 exe 免装 VC 运行库；CI 每一步都是真命令。