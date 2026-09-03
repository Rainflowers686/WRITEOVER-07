# AGENTS.md — Constitutional rules for every Codex agent in WRITEOVER-07.
# Read BEFORE touching any file. Violations are rejected at review.

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
- NO network code, SDL/DirectX window, runtime LLM, P1/P2 scope.

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

## 5. Daily rhythm
- Feature branch off `dev`; PR <= 400 lines; CI green before review.
- Ring code review: another human owner approves; Codex never self-approves.
- Daily integration 18:00, dev->main auto-merge 22:00.

## 6. Validation commands
```powershell
cmake --preset ci
cmake --build --preset ci
ctest --preset ci --output-on-failure
scripts/smoke.ps1
scripts/bench.ps1 -Preset release
scripts/contract_check.ps1
```
Stop and report if build fails twice without a new hypothesis.