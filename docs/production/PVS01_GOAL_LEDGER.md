# PVS-01 GOLD GOAL LEDGER — WRITEOVER-07

## Mission state

- Mission: `PRODUCT v1.2 / SYSTEMIC GAMEPLAY FOUNDATION / PVS-01 GOLD`
- Mission start head: `33e5e73c3f8845ed7236f749d0392fa9d53f59918`
- Current closure head: the final Gold receipt commit printed by `git rev-parse HEAD`
- Branch: `main` only
- Current phase: Gold closure evidence and cross-platform gates
- Local status at the time of this ledger update: all scoped changes are being
  validated before the final commit and push

The ledger records the actual Gold closure work. It supersedes the earlier
Silver receipt; the old Silver reports remain in Git history, but are not the
current release verdict.

## Grand objective

Deliver a small, coherent, production-wired PVS route:

`B1 Wake -> Calibration / Logistics -> Service / Medical -> 1F Lobby -> Security Checkpoint -> Restroom / Staff -> Elevator Lobby`

The route is deliberately bounded. It proves one playable systemic slice and a
real production input/replay path; it does not claim that the full 41-floor
building, all future M1-M6 content, or a complete AI planner has been shipped.

## Gold closures in this pass

- The active world path is the production Half-block TrueColor framebuffer:
  `240x67` terminal cells and `240x134` logical pixels. The frame is rendered
  from dark material bands with local cyan/amber/purple accents, depth-aware
  sprite occlusion, a weapon viewmodel and bounded effects.
- Pistol fire is live hitscan gameplay with spread, resolved aim, recoil,
  muzzle flash, hit feedback and deterministic systemic firearm events. A
  non-lethal weapon path produces a stunned NPC rather than silently killing
  it. Movement includes sprint speed and bounded head motion; reduced-shake and
  reduced-flicker settings preserve information.
- Windows has a procedural in-memory WinMM PCM backend for SFX/VO cues. POSIX
  builds use an honest `posix-subtitles-only` fallback because this repository
  does not assume a platform audio middleware dependency.
- Six authored NPC profiles are validated and compiled to
  `data/npcs/npcs.bin`. The active B1 runtime contains five NPCs: one
  `Full` cognition NPC and four `SemiHuman` cognition NPCs. `Guard` is a role
  and faction occupation, not a third cognitive tier.
- The autonomous adapter runs a bounded deterministic
  `Observe -> Remember -> Evaluate -> Choose -> Act` loop. Perception reads
  geometry, memories are added to the single systemic world, typed NPC events
  reach the EventBus, and the cleaner body route performs observation followed
  by an explicit response decision.
- The PVS route performs real search, badge reveal, theft, directed memory and
  relationship reaction, drag start/update/end, concealment, delayed body
  discovery, response, terminal session/audit, camera outage, credential /
  bribe / knowledge seams and the normal room route. These are small
  composition-root adapters; they do not replace module ownership with a new
  gameplay framework.
- F5/F9 uses the existing fail-closed SaveManager and includes Player, World,
  RNG, Events, AI, Narrative and Systemic sections. The final cross-platform
  check writes a save with Windows Release and loads that exact file with Linux
  Release.
- Four deterministic production replay scripts cover normal, aggressive,
  stealth and systemic behavior. They all reach 1F, exercise the body path,
  show real NPC loop counters, and complete Save/Load. The replay backend is a
  deterministic production input backend, not a test-only direct state mutator.
- Linux x64 GCC, Linux x64 Clang, macOS arm64 CI configuration, and Linux
  aarch64 cross-compilation/linking are in the CMake/CI matrix. Platform seams
  are limited to input, terminal, audio and atomic file replacement.
- The systemic schema gate now validates structure, closed enums, ranges,
  uniqueness, cross-references, locations, capacities, provenance fields and
  reserved future arrays. The content compiler emits bounded binary NPC
  profiles and rejects invalid authoring instead of silently dropping fields.

## Replay evidence

The exact scripts and concise receipts are also recorded in
`docs/production/PVS01_REAL_PLAY_MATRIX.md`.

| Route | Production command | Result |
|---|---|---|
| Normal | `writeover_app.exe --data-dir data --width 120 --height 40 --frames 3500 --replay tools/replay/pvs01_normal.txt` | PASS; route reaches 1F; 5 NPCs = 1 Full + 4 Semi; autonomous loops 221; discovery responses 1; Save/Load YES |
| Aggressive | `writeover_app.exe --data-dir data --width 120 --height 40 --frames 3600 --replay tools/replay/pvs01_aggressive.txt` | PASS; route reaches 1F; 5 NPCs = 1 Full + 4 Semi; autonomous loops 247; discovery responses 1; Save/Load YES |
| Stealth | `writeover_app.exe --data-dir data --width 120 --height 40 --frames 4500 --replay tools/replay/pvs01_stealth.txt` | PASS; route reaches 1F; 5 NPCs = 1 Full + 4 Semi; autonomous loops 394; discovery responses 1; Save/Load YES |
| Systemic | `writeover_app.exe --data-dir data --width 120 --height 40 --frames 3500 --replay tools/replay/pvs01_systemic.txt` | PASS; route reaches 1F; 5 NPCs = 1 Full + 4 Semi; autonomous loops 221; discovery responses 1; Save/Load YES; narrator typography active |

The Windows Release replay process reported `winmm-procedural`; the Linux
Release systemic replay reported `posix-subtitles-only`. Representative event
journals were non-empty (`30` normal/systemic, `42` aggressive, `50` stealth),
systemic events were non-empty (`37`, `49`, `57` respectively), and memories
were non-empty (`223`, `249`, `396` respectively). The body was searched and
the final player room was `room_1f_security` in each run.

## Validation gates

These are the final local gate classes. The exact final values belong in the
Gold report and final task receipt; a gate is not marked PASS unless the command
actually ran on the final source state.

- C++ Debug and Release configure/build: PASS locally.
- CTest and direct release test executable: PASS locally; the final direct
  executable reports 161 tests and 0 failed (the two `ASSERT FAILED` lines are
  intentional self-checks of the test harness).
- Smoke: PASS locally through the production application path.
- Bench: PASS locally. The bench includes the real systemic lookup/update
  workload, production raster, terminal encoding and a combined bounded
  production frame proxy with 25 runtime NPCs, EventBus dispatch, production
  raster, 25 sprite draws, weapon viewmodel, half-block composition and ANSI
  encoding. Platform device writes are excluded from the measured CPU proxy.
- Contract check: PASS locally; forbidden patterns, dependency direction and
  public-header baseline all pass.
- Static audit: PASS locally, `COUNT=0`.
- Content compiler and schema tests: PASS locally; current content compiler
  tests are 7/7 and systemic schema tests are 10/10.
- Invalid-seed startup gate: PASS locally; malformed systemic seed prevents
  startup.
- Cross-platform save: PASS locally for Windows Release save -> Linux Release
  load using the same `saves/pvs_manual.wo07` bytes.
- GitHub Actions: required remote result is recorded after the final push. The
  macOS arm64 job is intentionally a real Debug build/test/smoke gate plus a
  Release benchmark gate; no macOS physical host is assumed locally.

## Local performance evidence

The final Windows Release run after the integrated-frame benchmark was added reported:

- `PVS_RENDER_TIME_MS=1.007` worst-1%-average ms; budget PASS.
- `SYSTEMIC_LOOKUP_TIME_MS=0.013` worst-1%-average ms; budget PASS.
- `SYSTEMIC_UPDATE_TIME_MS=0.178` worst-1%-average ms; budget PASS.
- `PVS_TOTAL_FRAME_TIME_MS=1.311` worst-1%-average ms; budget PASS against
  `<= 6.0 ms`, with platform device writes excluded.
- terminal full `0.252 ms` / `38,448` bytes; delta `0.041 ms` / `2,375`
  bytes; unchanged `0.111 ms` / `0` bytes; worst-case safety `50,659` bytes;
  all declared encoder budgets PASS.

The final Linux Release rerun reported `PVS_RENDER_TIME_MS=1.165`, systemic
lookup `0.014`, systemic update `0.078`, and
`PVS_TOTAL_FRAME_TIME_MS=1.391`; all passed the same declared budgets. Linux
terminal full/delta/unchanged/worst-case times were `0.173`/`0.037`/`0.028`/
`0.266 ms` with the same bounded byte results. These numbers are evidence, not
a promise to future hardware.

The proxy is intentionally described narrowly. It is not a claim that every
future M5 planner or every future fully populated M1-M6 scene has been
benchmarked. It proves the bounded foundation workload and the current PVS
frame composition stay inside the declared CPU envelope on the measured host.

## Three final red teams

### Code review

PASS with no open Fatal or Major finding. The review checked ownership,
fail-closed parsing, duplicate IDs, bounded vectors/strings, authored profile
loading, save section validation, deterministic replay input, event dispatch,
NPC state transitions, body response separation and platform boundaries. No
second gameplay world, ECS, plugin layer, hook, branch or PR was introduced.

### Real-play / visual-effects review

PASS for the four deterministic production routes and cross-platform save/load.
The route reaches the authored 1F destination, and the runtime receipts prove
real autonomous phases, systemic events, body response and Save/Load. The
visual frame is a dark industrial Half-block production frame rather than the
reference renderer: floor and ceiling stay in dark luminance bands, props and
NPCs have localized accents, and the weapon/effect path is visible. Narrator
authority uses a large staged block-glyph composition with diagonal placement,
shake/glitch/accent behavior and reduced-flicker/reduced-shake fallbacks that
retain the text.

### Perfection review

PASS for this bounded Gold target, with controlled quality limits explicitly
listed below. The renderer uses deterministic procedural materials and an
embedded block glyph atlas rather than a large hand-authored external asset
pack. Windows audio is procedural WinMM and POSIX is subtitles-only. These are
known product decisions and platform limitations, not hidden claims of a
finished middleware/art pipeline.

## Controlled defers and quality limits

- Direct human-like mouse/keyboard completion of the route is not reproducibly
  available in this execution environment. Deterministic production replay is
  PASS; direct control remains `PARTIAL` and is the next human playtest
  observation, not a code failure.
- A physical macOS arm64 runtime session is not available locally. macOS
  arm64 build/test/smoke/benchmark is delegated to the configured GitHub Actions
  runner; runtime receipt is `UNVERIFIED` locally until that job completes.
- ARM64 cross-compilation/linking is PASS and the ELF architecture is verified;
  execution on a Kunpeng/ARM64 host remains `UNVERIFIED`.
- The full 41-floor building directory, complete M5 planner, complete M6
  content population and formal large-scale art/audio asset production remain
  intentionally outside PVS-01 Gold. The current route uses authored B1,
  calibration, service, 1F and side-route data only.

None of these controlled limits is an open Fatal or Major blocker for the
bounded PVS-01 Gold target. They must remain visible in the final report.

## Open counts

- Open Fatal: `0`
- Open Major: `0`
- Open Medium: `0` for the scoped Gold gates; controlled limits above are
  disclosed quality/availability conditions, not silently closed findings
- Open Low: `0`
- Open P0: `0`
- Open P1: `0`

## Commit / remote receipt

- Final Gold closure commit: the SHA printed in the final task receipt.
- Remote: `https://github.com/Rainflowers686/WRITEOVER-07`
- Branch: `main` only; no PR and no additional branch.
- Remote visibility: pre-existing `PUBLIC`; no visibility mutation is authorized
  or required by this PVS task.
- Required final check: remote `refs/heads/main` SHA equals local `HEAD`, and
  every required GitHub Actions job is green for that exact head.

## Stop rule

After the final report, remote verification and cleanup are complete, stop this
mission. Do not start M5/M6 implementation, broad art production, framework
expansion, or unrelated refactoring from this ledger.
