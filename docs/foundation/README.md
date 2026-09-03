# OUTPUT_FABLE_ENGINEERING_FOUNDATION_REPAIRED

WRITEOVER-07 / 重写协议：执行官07 — Fable 5.1 工程地基修复包（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）。

上一轮 `OUTPUT_FABLE_ENGINEERING_FOUNDATION.zip` 仅含 Markdown，`repo_seed/ schemas/ codex/
gates/ evidence/` 全为空目录，却声称 `REPO_SEED_BUILDS=YES`。本轮按外部技术验收
（`02_TECHNICAL_ACCEPTANCE_REPORT.md`）逐项修复：**真实 C++ 工程 + 真实构建/测试证据**。

## 内容

```
OUTPUT_FABLE_ENGINEERING_FOUNDATION_REPAIRED/
├── README.md
├── FOUNDATION_VERDICT.md        ← 修订：由证据决定，不再空口 GO
├── FOUNDATION_RED_TEAM.md       ← 修订：按修复后方案重做
├── S_DRAFT_AUDIT.md             ← 追加 REPAIR PASS 节（AUDIT 复检）
├── REPAIR_CHANGELOG.md          ← 本包与上一轮的差异清单
├── engineering/                 ← 31 篇规范（0–30 + 11_INPUT_CONTRACT）
├── repo_seed/                   ← 真实可编译 C++ 工程（本次核心交付）
│   ├── AGENTS.md
│   ├── CMakeLists.txt / CMakePresets.json / .github/workflows/ci.yml
│   ├── include/writeover/...    ← 公共头（冻结合同）
│   ├── src/...                  ← 实现（common/core/world/player/ai/narrative/render/platform/app）
│   ├── tests/                   ← 自研 harness + 50 测试
│   ├── tools/                   ← bench / mapc / contentc / contract_check
│   ├── scripts/                 ← bootstrap/build/test/bench/smoke/package/contract_check
│   ├── data/                    ← authoring JSON + 编译产物 .woc/.bin
│   └── docs/adr/
├── schemas/                     ← ROOM/NPC/STORYLET/FACT/SAVE/SETTINGS + 内容管线
├── codex/                       ← 全局规则 + 任务模板 + M1..M6 模块提示词（≥8）
├── gates/                       ← R0..RELEASE 共 8 个 gate
└── evidence/                    ← FOUNDATION_VALIDATION + 真实命令日志
```

## 证据摘要（完整见 evidence/）

| 项 | 结果 |
|----|------|
| `cmake --preset debug` | exit 0 |
| `cmake --build --preset debug` | exit 0（/W4 /WX 0 警告） |
| `ctest --preset debug` | exit 0，**50 tests, 0 failed** |
| `cmake --preset release` + build | exit 0（/MT 静态运行时） |
| `writeover_app --smoke --data-dir data` | exit 0（真终端→60 ticks→渲染→原子存档→恢复） |
| `writeover_bench.exe` | exit 0，240 列 p99=0.26ms，BUDGET=PASS |
| `scripts/contract_check.ps1` | exit 0（forbidden/deps/headers） |
| `mapc data/rooms/...woc` | exit 0 |

## 使用

见 `evidence/SCRIPT_ENTRYPOINTS.md`。先决条件：VS2022 Build Tools（Desktop C++）、CMake≥3.20、Python≥3.10（内容编译用）。

```powershell
cd repo_seed
pwsh -File scripts/bootstrap.ps1
cmake --build --preset debug
ctest --preset debug --output-on-failure
pwsh -File scripts/smoke.ps1
pwsh -File scripts/bench.ps1 -Preset release
```

## 重要

- 本包**不做**新的一轮规划红队；产品范围与 Opus 制作计划冻结不变。
- `CODEX_READY` 是否 YES 只由 `FOUNDATION_VERDICT.md` 判断（见其验收映射）。
- 未实现项一律 `NOT_RUN`/`NOT_READY` + 真实原因，无 fake PASS。