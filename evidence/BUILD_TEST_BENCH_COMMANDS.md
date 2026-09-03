# BUILD_COMMANDS / TEST_COMMANDS / BENCH_COMMANDS — 真实执行记录

以下全部命令在本机（Windows 11, VS2022 Build Tools 17.14 / MSVC 19.44, CMake 3.29.2）实际运行。

## Build (Debug)

```text
> cmake --preset debug
exit code 0
-- Selecting Windows SDK version 10.0.26100.0 ...
-- The CXX compiler identification is MSVC 19.44.35228.0
-- WRITEOVER-07 foundation configured
-- Configuring done (6.4s)
-- Build files have been written to: .../repo_seed/out/build/debug

> cmake --build --preset debug
exit code 0
writeover_common.lib / writeover_world.lib / writeover_player.lib / writeover_ai.lib
writeover_core.lib / writeover_narrative.lib / writeover_render.lib / writeover_platform.lib
writeover_app.exe / writeover_bench.exe / writeover_tests.exe / mapc.exe
0 errors (warnings-as-errors /W4 /WX)
```

## Build (Release)

```text
> cmake --preset release
exit code 0
> cmake --build --preset release
exit code 0
Release/...exe artifacts, 0 errors
```

## Test

```text
> ctest --preset debug --output-on-failure
exit code 0
50 tests, 0 failed
```

抽检通过用例（对应验收清单 F 节）：

```text
[PASS] save.compose_parse_round_trip
[PASS] save.corrupt_rejected          (坏档 fail-closed)
[PASS] save.rejects_bit_flip
[PASS] settings.encode_decode
[PASS] profile.round_trip
[PASS] ray.flat_hits_wall             (2.5D 全遮挡边界)
[PASS] ray.low_wall_floor_rise        (抬高地板的低墙区间)
[PASS] ray.full_occlusion
[PASS] ray.projection_up_down
[PASS] bench.p99_is_one_percent_low
[PASS] player.posture_clearance       (净空窗口)
[PASS] player.jump_then_land
[PASS] player.combat_reload
[PASS] fact.belief_fact_round_trip    (FactValue 含 EntityId)
[PASS] infrastructure.apply_door
[PASS] room.codec_round_trip
[PASS] map_validator.spawn_fits
[PASS] ai.goap_plans_chain
[PASS] ai.goap_no_plan_impossible
[PASS] ai.memory_recall_order
[PASS] storylet.once_fired_skipped
[PASS] narrator.capability_gates_command
[PASS] narrator.cannot_mutate_facts   (旁白不能直接改 FactStore)
[PASS] causality.ledger_ring
[PASS] replay.same_seed_identical_bytes
[PASS] save.deterministic_round_trip  (wire 无墙钟时间戳)
```

## Bench

```text
> writeover_bench.exe
exit code 0
scenario,count,avg_ms,p99_ms,min_ms,max_ms
raycast_column_sweep,240,0.185,0.260,0.167,0.274
BUDGET=PASS
```

## Smoke

```text
> writeover_app.exe --smoke --data-dir data
writeover_game exit=0 (smoke)
saves/smoke.wo07  (100 bytes)
```

## Contract checks

```text
> scripts/contract_check.ps1
check_forbidden: OK
check_deps: OK
check_public_headers: OK (baseline recorded on first run)
exit 0
```

## Content pipeline

```text
> python tools/contentc/contentc.py --data-dir data --out-dir data
room: room_01_calibration.json -> room_01_calibration.woc (2180 bytes)
facts: 4 -> facts.bin
storylets: 2 -> storylets.bin
contentc: OK
exit 0

> mapc.exe data/rooms/room_01_calibration.woc
mapc: OK data/rooms/room_01_calibration.woc
exit 0
```

## NOT_RUN 声明（真实原因）

| 项 | 原因 |
|----|------|
| GitHub Actions CI 实跑 | 本机未配置 remote/actions runner；`.github/workflows/ci.yml` 已提供 |
| ARM64 编译 | 本机为 x64；结构依据已在工程文档给出 |
| 干净机器（他机）测试 | 需要 ≥3 台未知机器，属 9/16 发布 gate |
| Raw Input 鼠标主后端 | 本机已实现 cursor-delta 回退；Raw Input 注册属 M3 施工项（R0 验证） |