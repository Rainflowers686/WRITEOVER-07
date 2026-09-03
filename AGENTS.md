# AGENTS.md — Constitutional rules for every Codex agent in WRITEOVER-07.
# Read BEFORE touching any file. Violations are rejected at review.

## 0. Fable-5 hardening is DONE (2026-09-03)
- Test oracle is fail-fast; test evidence is trustworthy (see
  `docs/reference/HK1_SIM_EVENT_SAVE.md`, G0 in
  `docs/audit/04_HARD_KERNEL_GATES.md`).
- HK-1..HK-6 reference implementations + docs:
  `docs/reference/HK1_SIM_EVENT_SAVE.md` … `HK6_CONTENT_STORYLET.md`.
- Static audit is now a regression gate for the fixes:
  `python tools/audit/static_audit.py .` (must be COUNT=0).
- Public contract changes recorded in `docs/adr/ADR-0002-fable-hardening-contract-changes.md`.

## 1. Mandatory reading order
1. `AGENTS.md` (this file)
2. `include/writeover/**` public contracts (frozen)
3. Your module: `codex/M*_*_CODEX.md` (in the foundation package)
4. `engineering/00_ENGINEERING_CONSTITUTION.md`

## 2. Absolute prohibitions
- NO changes to public types/ids in `include/writeover/` without ADR + owner.
- NO second ID / Vec / EventBus implementation.
- NO `std::rand`, wall-clock in gameplay, unordered iteration affecting decisions.
- NO raw owning pointers; no struct-dump serialization; no `system()`; no
  per-character cout; no silent catch; no magic payload bytes.
- NO new dependencies (zero third-party); NO new threads.
- NO changing save schema, sim timing (120Hz fixed), renderer paradigm, content
  schema, error policy.
- NO network code, SDL/DirectX window, runtime LLM.
- ONLY PRODUCT_BASELINE_V1_1 AUTHORIZED EXPANSIONS ARE ALLOWED. 任何未列入
  v1.1 的旧废案仍禁止自行恢复（见 `docs/product/PRODUCT_BASELINE_V1_1.md`）。

## 3. Owned files
Only touch files inside your module's owned directories (see module prompts).
Never edit another module's `src/` or tests directly.

## 4. Every task ends with an evidence block
```
CURRENT_FACTS
OWNED_FILES
INVARIANTS
PLAN
IMPLEMENTED_CHANGES
BUILD_COMMAND
BUILD_RESULT          (exact exit code; NOT_RUN if not executed)
TEST_COMMAND
TEST_RESULT
INTEGRATION_RESULT
BENCHMARK_RESULT      (if relevant)
DIFF_SUMMARY
PUBLIC_CONTRACT_CHANGED = NO   (or ADR number)
KNOWN_LIMITATIONS
STOP_REASON
```
Never write PASS without running. `NOT_RUN` is honest; fabricated PASS is a
contract violation.

## 5. Git workflow (main-only)
- Direct commits to `main` only. No dev branch, no feature branches, no PRs.
- 不允许六 Agent 同时无约束写同一文件。
- Commits reviewed by a human owner before CI is treated as authoritative.
- CI runs on every push to `main` (`ctest` + internal test executable).

## 6. Validation commands
```powershell
cmake --preset ci
cmake --build --preset ci
ctest --preset ci --output-on-failure
.\out\build\ci\Debug\writeover_tests.exe
scripts/smoke.ps1
scripts/bench.ps1 -Preset release
scripts/contract_check.ps1
python tools/audit/static_audit.py .
python tools/contentc/contentc.py --data-dir data --out-dir data --check
```
Stop and report if build fails twice without a new hypothesis.