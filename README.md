# repo_seed — WRITEOVER-07 C++ 工程骨架（真实可编译）

公共契约、构建、测试的唯一真源。详见包根 `README.md` 与 `engineering/`。

```powershell
# 快速验证（需 VS2022 Build Tools + CMake≥3.20 + Python≥3.10）
python tools/contentc/contentc.py --data-dir data --out-dir data
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure     # 157 tests, 0 failed
.\out\build\debug\Debug\writeover_app.exe --smoke --data-dir data
```

## 目标（CMake）

writeover_common / systemic / world / player / ai / narrative / render / core / platform /
writeover_app（组合根 exe）/ writeover_tests / writeover_bench / mapc。

## 目录

```
include/writeover/  冻结公共头（common/systemic/core/world/player/ai/narrative/render）
src/                实现（app=组合根；platform/windows=Win32 边界）
tests/              自研 harness + 157 测试
tools/              bench · mapc · contentc · contract_check
scripts/            bootstrap · build · test · bench · smoke · package · contract_check
data/               作者 JSON + 编译产物（.woc/.bin）
docs/adr/           ADR-0001 示例
```

## 纪律

- 公共头改动必须走 ADR（27 文档）；`scripts/contract_check.ps1` 机器把关。
- 未实现功能如实 NOT_READY/NOT_RUN，禁止 fake PASS。