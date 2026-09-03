# 26_FORBIDDEN_PATTERNS

## Machine-Checkable Forbidden List

| # | Pattern | Why | Check |
|---|---------|-----|-------|
| 1 | Second EntityId / Vec / EventBus implementation | contract drift, duplicate bug surface | grep identifiers in contract_check |
| 2 | `std::rand()` / `rand()` | non-deterministic gameplay | grep |
| 3 | Wall-clock in gameplay (`time()` in sim code) | breaks determinism | grep in src (excluding benchmark/platform) |
| 4 | `unordered_map`/`unordered_set` iteration affecting decisions | unstable order | review; prefer ordered map |
| 5 | Owning raw pointer (`new`/`delete`/`malloc`) | lifetime bugs | grep `new `, `delete ` |
| 6 | Raw struct binary dump (`reinterpret_cast` for serialize, fwrite(struct)) | padding/endian bugs | grep reinterpret_cast |
| 7 | `system("cls")` or `"cls"` command strings | flicker, resets state | grep |
| 8 | `std::cout` per-character output in render | slow/flicker | grep `cout` in render |
| 9 | Silent `catch (...) {}` | hides errors | grep catch |
| 10 | Magic payload bytes (array-of-bytes event data) | untyped, unversionable | review |
| 11 | Public API drift without ADR | breaks other modules | contract_check diff |
| 12 | Render mutating world state | architecture violation | review + include scan |
| 13 | Network code (sockets, winsock) | scope violation | grep socket |
| 14 | SDL/DirectX window / GPU | scope violation | grep SDL/DirectX |
| 15 | Runtime LLM / network AI | scope violation | hard review |
| 16 | P1/P2 scope creep | schedule risk | PR review |
| 17 | Speculative generic framework (meta-template maze, lock-free everywhere, reflection) | complexity | review |
| 18 | Fake benchmark/test result (hardcoded PASS/CSV) | lies in evidence | review + CI double-run |
| 19 | `wchar_t` as glyph storage | encoding ambiguity | contract_check |
| 20 | `Update(float dt)` module interface | variable time step | contract_check |

## Human-Checkable Rules

- No changing a frozen public header without ADR + owner approval.
- No adding third-party dependencies (runtime or dev) — zero.
- No `#include <windows.h>` outside `src/platform/windows/` and `tests/` (platform tests).
- No new threads.
- No random iteration order in any decision path.
- No `printf`/`cout` debugging left in merged code (use Logger).

## Enforcement Points

1. **CI**: `tools/contract_check/check_forbidden.ps1` runs grep-based checks on every PR; fails on hits.
2. **Review**: human owner checks review checklist (from AGENTS.md).
3. **Codex protocol**: `PUBLIC_CONTRACT_CHANGED = NO` line prevents bypass.

## 报告友好

**Course Note**: 这些禁止项是为了让 6 人 + Codex 在 16 天内不互相破坏；每一条都有脚本或人查可验证。
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
