# 00_ENGINEERING_CONSTITUTION

> The engineering constitution for WRITEOVER-07. All Codex agents and human owners MUST read this first.

---

## 1. Project Identity

- **Name**: WRITEOVER-07 / 重写协议：执行官07
- **Type**: Single-player terminal-based first-person character 3D FPS
- **Platform**: Windows 10/11, Windows Terminal or conhost
- **Course**: C++ course design (requires Save/Load/NewGame, 6-person team, course report)

## 2. Scope (Frozen)

- 6 Signature Rooms + 3 connective micro-spaces
- 2 Full + 2 Medium + 3 Light NPC + 6–10 guards
- 3 weapons (pistol, SMG, electro-stunner)
- 3 endings
- Height-Span Grid Raycaster renderer
- Single-player, offline
- No runtime LLM
- No free dialogue
- No network
- No Sector/Portal
- No Stretch Rooms

## 3. Three Pillars (Non-negotiable)

1. High-refresh character 3D FPS with satisfying shooting and stealth
2. A narrator with authority, resources, that lies and can be countered
3. Every meaningful action leaves observable causal consequences

## 4. Authority Hierarchy

1. Course hard requirements
2. Opus PRODUCTION_EXECUTION_FREEZE
3. P0-v1.0 product freeze
4. Round-3 technical corrections
5. Opus S-section engineering requirements/sketches
6. Fable design (this package)

If a sketch conflicts with a higher constraint, the sketch is modified, not the product.

## 5. Public Contract Ownership

**Codex agents must NOT change:**
- Public types and IDs
- Save schema
- Simulation timing (120Hz fixed)
- Module dependency graph
- Add third-party dependencies
- Add threads
- Change renderer paradigm
- Change content schema
- Change error policy

**To change any of the above:**
1. Write ADR in `docs/adr/`
2. Get human module owner approval
3. Get Opus review (for Opus-designated critical changes)
4. Update all affected headers and dependent modules

## 6. Ownership / Lifetime

- Prefer value types
- Use `std::unique_ptr` for dynamic ownership
- Raw pointers are only for non-owning observation
- No owning raw pointers, no manual `delete`
- No global mutable state
- `EngineContext` is the single dependency injection point (not a service locator)

## 7. Error Handling

- Programmer errors → assert (fail fast in debug)
- Recoverable runtime errors → `Result<T>` (std::variant-backed; supports
  non-default-constructible values — see common/result.h)
- Corrupted data → fail closed
- User input errors → graceful feedback
- Our code does not throw: `/EHsc` + zero explicit throw/try/catch in our TUs;
  STL allocation failures remain possible and are a top-level process policy
  (documented in 20_ERROR_HANDLING_LOGGING.md). `/EHsc` is NOT the same as
  "no exceptions" — the real rule is "no exceptions in OUR code".
- No silent catch

## 8. Determinism

- Fixed 120Hz simulation
- Single deterministic RNG (XorShift128+) with full state save/load
- No wall-clock in gameplay logic
- Deterministic iteration order
- Same build + same seed + same input = same state

## 9. Threading

- R0: single-threaded Sim + Render + Present
- Audio: separate platform thread (playback only)
- Profile first, parallelize second
- World logic NEVER multi-threaded

## 10. Dependency Direction

```
common (no dependencies)
    ↑
world / player (controlled deps; player reads world queries)
    ↑
ai (reads world+player queries) / narrative (reads world+player+ai)
    ↑
core (common ONLY — engine/save/settings/profile)
    ↑ (no reverse dependency)
render (reads world/player snapshots)
    ↑
platform/windows (implements input/terminal/audio/atomic-io interfaces)
    ↑
writeover_app  ← COMPOSITION ROOT (M1 owns; the ONLY TU that links everything)
```

Forbidden:
- render mutates world
- ai includes terminal
- narrative includes Win32
- world includes HUD
- player owns save file
- platform code leaks into semantic core
- writeover_core links world/player/ai/narrative/render (composition lives in `writeover_app`, not core)

## 11. No-Go Patterns (Machine/Manually Checkable)

- Second EntityId implementation
- Second Vec library
- Second EventBus
- `std::rand()`
- Wall-clock in gameplay
- Unordered iteration affecting decisions
- Owning raw pointer
- Raw struct binary dump
- `system("cls")`
- Per-character cout
- Silent catch
- Magic payload bytes
- Public API drift without ADR
- Render/world shared mutable state
- Hidden network code
- SDL/DirectX visible window
- Runtime LLM
- P1/P2 scope
- Speculative generic framework
- Fake benchmark/test result

## 12. Coding Standard (Abridged)

- C++17
- MSVC /W4 /WX
- `/utf-8` source encoding
- 4-space indent, Allman braces
- `snake_case` for functions/variables, `PascalCase` for types
- `kCamelCase` for constants
- No `auto` where the type is not obvious
- Prefer `const` everywhere
- Include what you use
- No `using namespace std` in headers
- Comments: explain why, not what

## 13. Testing

Every module:
- ≥3 unit tests
- 1 core data structure test
- 1 core algorithm test
- 1 edge/error handling test
- Real tests, no fake PASS

## 14. Feature Flags

```cpp
#define WO_FEATURE_VO             1
#define WO_FEATURE_GOAP           1
#define WO_FEATURE_PRONE          1
#define WO_FEATURE_VAULT          1
#define WO_FEATURE_MANTLE         1
#define WO_FEATURE_GAMEPAD        1
#define WO_FEATURE_PRESENT_THREAD 0
```

Fallback changes implementation/flag, never public API.

## 15. Definition of Done (Function)

```
- Compiles, 0 warnings
- Has unit test coverage
- Has edge/error test
- Has doc comment (purpose/params/returns)
- No public API drift
```

## 16. Definition of Done (Module)

```
- Compiles, 0 warnings
- ≥3 unit tests passing
- Integration test passing
- Debug panel or debug command available
- Performance budget met
- UML class diagram updated
- Core algorithm flowchart/sequence diagram
- STL usage list
- Design pattern notes (only real ones)
- 60–90 second teach-back outline
```

## 17. Codex Protocol (Summary)

Before work: read constitution, public contracts, module AGENTS, owned files, invariants.
During work: only touch owned files, no public API, no dependency, no refactor, no scope expansion.
After work: build, unit test, integration, benchmark, diff inspection, contract drift check, exact commands/results.

## 18. This Constitution Is a Contract

Violations by Codex are rejected at review.
Violations by humans are fixed at daily integration.
No exceptions without ADR.