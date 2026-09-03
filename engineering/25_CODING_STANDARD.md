# 25_CODING_STANDARD

## Language

- C++17 only (no C++20 features, no experimental)
- MSVC v143 target; portable-ish, no vendor-only extensions

## Formatting (enforced by .clang-format)

```
- 4-space indentation
- Allman braces (opening brace on its own line)
- 120 column limit
- One statement per line
- Pointer alignment: left (`int* p`)
- Space inside parens: no (`f(x)`, not `f( x )`)
```

## Naming

| Kind | Convention | Example |
|------|-----------|---------|
| Types (class/struct/enum) | PascalCase | `WorldEvent`, `GameAction` |
| Functions | snake_case | `cast_world_ray`, `get_posture_box` |
| Variables | snake_case | `player_pos`, `interval_count` |
| Member variables | trailing underscore | `frame_count_` |
| Constants | kCamelCase | `kFixedDeltaTime`, `kMaxIntervals` |
| Enum values | PascalCase | `Posture::Crouch` |
| Macros | UPPER_SNAKE | `WO_FEATURE_PRONE` |
| Files | snake_case.h/.cpp | `world_event.h` |

## Includes

```
1. own header
2. project public headers (<writeover/...>)
3. STL (<vector>, <optional>)
4. platform (only in platform dir)
```

- Include what you use (IWYU).
- Never `using namespace std` in headers.
- Headers must be self-contained (compile alone in a TU test).

## const & Value Semantics

- Mark every non-mutating method `const`.
- Pass by `const&` for non-trivially-copyable types; by value for trivially copyable.
- Prefer returning values over out-params.
- Prefer `struct` for data-only types (POD-ish with explicit methods OK).

## Auto & Deduction

- `auto` allowed when the type is obvious from the right-hand side or when the type is long (`std::unique_ptr<...>`).
- Do not use `auto` to obscure non-obvious types.
- No `auto&&` turbulence.

## Comments

- Explain WHY, not what.
- Doc comments on public API: purpose, params, return, determinism note where relevant.
- No commented-out code in merges.
- Chinese or English comments both allowed (course is Chinese); be consistent within a file. Public headers: bilingual header comments accepted.

## RAII & Lifetimes

- No raw `new`/`delete`; ownership via value or `std::unique_ptr`.
- No owning raw pointers.
- Prefer `std::array`/`std::vector` over C arrays in new code.

## Scoping

- Local variables declared at first use.
- `const` everywhere applicable.
- No magic numbers: named constants.
- No global mutable state. Cross-cutting services via `EngineContext`.

## Determinism Hygiene

- Random: only through sim RNG (passed as parameter), never `std::rand`.
- No unordered iteration affecting decisions.
- No wall clock in gameplay.

## Error Handling

- `WO_ASSERT` for invariants (Debug).
- `Result<T>` for recoverable failures.
- Log failures; no silent catch.

## Testing Hygiene

- Every new function with behavior gets a test.
- Tests assert real behavior; no empty asserts, no disabled tests committed.

## 报告友好

Each module engineering doc has: OOA objects · UML · STL usage · design patterns (real only) · core algorithms · test evidence · problems/solutions. Those are written into `codex/` module prompts too.
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
