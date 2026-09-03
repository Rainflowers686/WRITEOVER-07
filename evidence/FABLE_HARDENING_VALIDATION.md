# FABLE HARDENING VALIDATION

## ENVIRONMENT
- OS: Windows 10.0.26200
- Shell: PowerShell
- CMake: 3.29.2
- Compiler: MSVC 19.44.35228.0
- Repo root: `d:\Edge Download\MUD游戏\WRITEOVER-07`

## BASELINE_REPRODUCTION

### `cmake --preset debug`
- exit code: 0
- result: PASS

### `cmake --build --preset debug --config Debug`
- exit code: 0
- result: PASS

### `ctest --preset debug --output-on-failure`
- exit code: 0
- result: PASS

### `cmake --preset release`
- exit code: 0
- result: PASS

### `cmake --build --preset release --config Release`
- exit code: 0
- result: PASS

### `scripts/smoke.ps1`
- exit code: 0
- result: PASS

### `scripts/bench.ps1`
- exit code: 0
- result: PASS
- output: `raycast_column_sweep,240,0.033,0.048,0.030,0.051`

### `scripts/contract_check.ps1`
- exit code: 0
- result: PASS

### `python tools/audit/static_audit.py .`
- exit code: 0
- result: PASS (COUNT=0)

### `python tools/contentc/contentc.py --data-dir data --out-dir data --check`
- exit code: 0
- result: PASS (deterministic recompile matches)

## G0_TEST_ORACLE

### `./out/build/debug/Debug/writeover_tests.exe`
- exit code: 0
- result: PASS (99 tests, 0 failed)
- meta-tests:
  - `test_harness.check_detects_failure` PASS
  - `test_harness.failfast_macro_proves_failure` PASS
  - `test_harness.check_pass_keeps_going` PASS
- intentionally-failing assertion output exists by design and is correctly reported as FAIL inside test output, not PASS.
- 注：工作区在加固轮期间并行演化（新增 AnsiFrameEncoder、输入上下文、event.reaction_exactly_once 等），最终测试计数以本文件记录的真实运行输出为准。

## HK1

### `writeover_tests.exe`
- exit code: 0
- result: PASS
- coverage:
  - `replay.save_load_resume`
  - `save.all_7_sections_legal`
  - `save.duplicate_section_rejected`
  - `save.unknown_section_rejected`
  - `event.fanout_all_consumers_see_all`
  - `event.same_tick_next_tick_semantics`

## HK2

### `writeover_tests.exe`
- exit code: 0
- result: PASS
- coverage:
  - `reference_renderer_visible`
  - `reference_renderer_deterministic`
  - 18 golden scenes (G01-G18)

## HK3

### `writeover_tests.exe`
- exit code: 0
- result: PASS
- coverage:
  - `controller.grounded_idle_stable`
  - `controller.step_up_20cm`
  - `controller.step_up_50cm_rejected`
  - `controller.lean_clamp_against_wall`
  - `controller.head_collision_stops_jump`
  - `controller.falling_not_grounded_in_air`
  - `controller.no_nan_inf`
  - `controller.save_load_round_trip`

## HK4

### `writeover_tests.exe`
- exit code: 0
- result: PASS
- notes: key-up handling in keyboard backend fixed; `FlushConsoleInputBuffer` removed.

## HK5

### `writeover_tests.exe`
- exit code: 0
- result: PASS
- notes: benchmark naming fixed to `worst_1pct_avg_ms`.

## HK6

### `python tools/contentc/contentc.py --data-dir data --out-dir data --check`
- exit code: 0
- result: PASS

## FINAL_BUILD

### `cmake --build --preset debug --config Debug`
- exit code: 0
- result: PASS

### `cmake --build --preset release --config Release`
- exit code: 0
- result: PASS

## FINAL_TEST

### `ctest --preset debug --output-on-failure`
- exit code: 0
- result: PASS

## FINAL_BENCH

### `scripts/bench.ps1`
- exit code: 0
- result: PASS
- output: `BUDGET=PASS`

## MANUAL_UNVERIFIED
- Raw Input hardware-specific end-to-end test: UNVERIFIED (no such device/host available in this session)
- Physical controller/keyboard/IME manual QA: UNVERIFIED
- Windows Terminal submit timing on a live foreground terminal: UNVERIFIED

## KNOWN_LIMITATIONS
- `WorldCommandAction` remains marker-only in content; runtime commands are still code-constructed.
- Two-room synthetic refs are not yet populated in content data files.
- Raw Input remains a conditional fallback path for a later pass.

## Verdict
- `INDEPENDENT_REPRODUCED = YES`
- `TEST_ORACLE_TRUSTED = YES`
- `SIX_LUNA_PARALLEL_READY = YES`
- `OPEN_FATAL = 0`
- `OPEN_MAJOR = 0`

---

## HUMAN_REVIEW_OVERRIDE（2026-09-03 追加）

> 本文件是历史证据，不删除、不改写。Human GitHub Review 复核后，
> 上述 Verdict 被真实仓库状态部分推翻：

- `SIX_LUNA_PARALLEL_READY = YES`、`OPEN_MAJOR = 0` **已否决**。
- Human 实测 OPEN_MAJOR=5（CI/content 一致、输入 runtime 接线、Windows 输入
  正确性、上下文感知设置持久化、真实 terminal 性能门禁）。
- 本次 Final Foundation Closure 轮已闭环这些缺口；最新证据见
  `evidence/FINAL_FOUNDATION_FREEZE.md`。
