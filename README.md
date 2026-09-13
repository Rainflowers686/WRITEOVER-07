# repo_seed — WRITEOVER-07 C++ 工程骨架（真实可编译）

公共契约、构建、测试的唯一真源。详见包根 `README.md` 与 `engineering/`。

```powershell
# 快速验证（需 VS2022 Build Tools + CMake≥3.20 + Python≥3.10）
python tools/contentc/contentc.py --data-dir data --out-dir data
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure     # unit + standalone public-header tests
.\out\build\debug\Debug\writeover_app.exe --smoke --data-dir data
```

## 目标（CMake）

writeover_common / systemic / world / player / ai / narrative / render / core / platform /
writeover_app（组合根 exe）/ writeover_tests / writeover_bench / mapc。

## 目录

```
include/writeover/  冻结公共头（common/systemic/core/world/player/ai/narrative/render/audio/platform）
src/                实现（app=组合根；platform/windows 或 platform/posix=平台边界）
tests/              自研 harness + regression tests
tools/              bench · mapc · contentc · contract_check
scripts/            bootstrap · build · test · bench · smoke · package · contract_check
data/               作者 JSON + 编译产物（.woc/.bin）
docs/adr/           ADR-0001 示例
```

## 纪律

- 公共头改动必须走 ADR（27 文档）；`scripts/contract_check.ps1` 机器把关。
- 未实现功能如实 NOT_READY/NOT_RUN，禁止 fake PASS。

## 实用运行：Chapter One 与 Act II-A

先从仓库根目录编译作者内容，再运行确定性检查：

```powershell
python tools/contentc/contentc.py --data-dir data --out-dir data
python tools/contentc/contentc.py --data-dir data --out-dir data --check
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset debug
```

生产 smoke 可以直接指定房间；`--room` 只改变起始房间，不绕过运行时
状态规则：

```powershell
out/build/debug/Debug/writeover_app.exe --smoke --frames 1 --width 240 --height 67 --data-dir data --room room_b1_revival --user-data-dir out/manual/b1 --dump-frame out/manual/b1/frame.svg
out/build/debug/Debug/writeover_app.exe --smoke --frames 1 --width 240 --height 67 --data-dir data --room room_1f_security --user-data-dir out/manual/security --dump-frame out/manual/security/frame.svg
out/build/debug/Debug/writeover_app.exe --smoke --frames 1 --width 240 --height 67 --data-dir data --room room_elevator_lobby --user-data-dir out/manual/elevator --dump-frame out/manual/elevator/frame.svg
out/build/debug/Debug/writeover_app.exe --smoke --frames 1 --width 240 --height 67 --data-dir data --room room_act2_service_concourse --user-data-dir out/manual/concourse --dump-frame out/manual/concourse/frame.svg
```

Act II-A room ids are `room_act2_service_concourse`,
`room_act2_records_archive`, `room_act2_power_utility`,
`room_act2_transit_control` and `room_act2_observation_gallery`. They are
authored CharCell spaces with functional equipment groups, NPCs, storylets and
backtracking transitions; they are not a separate rendering mode.

For a direction/LOD/weapon sheet, use the authored review executable and then
convert the SVG with ImageMagick if a PNG is needed:

```powershell
out/build/debug/Debug/writeover_art_review.exe data/characters/b1_character_art.txt out/manual/art_review
magick out/manual/art_review/human_sheet.svg out/manual/art_review/human_sheet.png
magick out/manual/art_review/weapon_sheet.svg out/manual/art_review/weapon_sheet.png
```

The acceptance sequence is always AUTHOR -> REAL PRODUCTION RENDER ->
CRITIQUE -> REVISE -> REAL PRODUCTION RENDER. Source glyph rows, unit tests or
an old screenshot do not replace a current production-frame check. The
protected visual contract and future completion map are in
`docs/production/CHAPTER01_CREATIVE_DIRECTION.md` and
`docs/production/POST_LUNA_GPT6_HANDOFF.md`.
