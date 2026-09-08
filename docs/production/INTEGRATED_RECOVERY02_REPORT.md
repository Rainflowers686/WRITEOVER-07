# WRITEOVER-07 — Integrated Recovery-02 Report

## Scope and status

This report records the bounded spatial, temporal, and gameplay-truth recovery
slice implemented from the live `main` worktree.  It is not a product-Gold,
release, or complete M1–M6 claim.  No new map, weapon type, story branch,
renderer, ECS, tag, release, or Steam operation was added.

The recovery target was a truthful B1 loop: player input changes authoritative
world state, the active NPC runtime observes and acts on that state, and the
player-facing result is observable in the runtime/replay output.

| Field | Value |
|---|---|
| `START_HEAD` | `a4c03f2a0f0b9b6a68e28aef02d3ddfedaa39871` |
| `IMPLEMENTATION_HEAD` | `73e9e9a` (`integration: close bounded recovery truth seams`) |
| `START_ORIGIN_MAIN` | `a4c03f2a0f0b9b6a68e28aef02d3ddfedaa39871` |
| `CANONICAL_ROOT` | `D:\\AAAbiancheng\\00_Projects\\40_Coursework\\2026_CPP_Immersive_ASCII_FPS` |
| `BRANCH` | `main` only |
| `OLD_GOLD_TAG` | `v0.1.0-pvs01-gold` preserved; no tag mutation |
| `CHARACTER_VISUAL_ACCEPTANCE` | `PENDING` |

## IMPLEMENTED

### Time, presentation, and module wiring

- Added a private application time gate so pause freezes gameplay frame and
  gameplay-facing module time while presentation remains explicitly separated.
- `Engine::Run` renders once after a fixed-step simulation advance; the
  no-step path no longer presents a second gameplay-duration update.
- Player, world, AI, and narrative use the bounded game-frame source where the
  recovery slice needs scene-relative timing.
- Render timers, subtitles, and dialogue do not continue consuming gameplay
  time while paused.

### B1 interaction and world truth

- B1 interaction resolves a target from the active scene-entity table, player
  pose, bounded facing, range, and 3-D segment line of sight.
- Credential access still separates credential validity from current
  possession: B1 reader use requires the player to hold the badge before the
  gate state can change.
- Accepted gate access changes the real infrastructure door state and the
  player can cross it. Denied access leaves the gate closed.
- Terminal success requires the selected terminal, range/target check, and a
  valid held badge; the terminal session/audit state is changed only on
  success.
- The active B1 door, terminal, camera, cart, and other bounded targets share
  the private placement table used by rendering and interaction. This is a
  recovery seam, not a new general entity framework.
- Six explicit bounded room-portal records replace the relevant scattered
  non-B1 threshold callbacks for the current calibration/security/medical/
  restroom/elevator routes.

### Runtime population and visual truth

- Authored NPC profiles are loaded from the compiled profile data and retained
  by spawn room; active-room filtering controls runtime behavior and rendering.
- In B1, a stunned/dead runtime actor is not rendered as a standing actor while
  its exposed systemic body is rendered as the single body representation.
  Hidden bodies are not rendered.
- B1 does not add the overlapping Administrator twice; the relevant active
  guard/profile is room-scoped.
- Rendered weapon identity is derived from the active combat slot. The pistol
  viewmodel is not drawn or labelled as the active weapon when the active slot
  is SMG or stunner.

### AI, perception, memory, and the cleaner slice

- Heard-only gunshot memory does not fabricate a visible player subject or
  target. Unknown noise remains unknown unless sight establishes attribution.
- Continuous observations refresh one bounded semantic memory instead of
  creating one identical durable memory at every decision interval.
- The cleaner has a real position and bounded movement toward the work/cart
  area. Arrival, inspection delay, body discovery, and response are separate
  runtime steps; a due frame alone cannot discover a body before arrival.
- Body discovery is observation first and response second. The existing
  bounded response can report or call medical according to the implemented
  decision path; discovery is not unconditionally a security alert.
- A stunned/unconscious NPC cannot become a direct witness without a valid
  perception path.
- AI load preflights saved NPC identities before mutating the live AI runtime.

### Narrative, save/load, and fail-closed data

- Storylet selection consumes a real fact in the B1 flow and executes the
  bounded implemented actions: typed trigger event, subtitle/dialogue line,
  world command, and game-over command where authored.
- Dialogue queue and causality ledger are included in the narrative save
  section and loaded transactionally with the storylet engine.
- Application load stages player, world, event bus, RNG, narrative, AI, and
  systemic sections before the bounded live commit; the exact saved room is
  probed and compared before switching.
- Storylet, dialogue, and causality loaders reject bounded-invalid input:
  oversized counts, invalid enum/ID values, duplicate storylet/fired IDs,
  truncation, and trailing compiled bytes where applicable.
- The existing systemic duplicate-ID and bounded-storage policies remain the
  source of truth; no replacement storage framework was introduced.

### Gameplay assertions

- Replay output distinguishes `REPLAY_PROCESS_EXIT_OK`,
  `REPLAY_INPUT_CONSUMED`, `REPLAY_EXPECTED_STATE_REACHED`, and
  `CHAPTER_CHECKPOINT_REACHED`.
- The recovery assertions check state predicates rather than accepting process
  exit code as gameplay proof.
- Player health is an authoritative state used by damage, death/recovery, and
  the HUD. The current enemy path is deliberately bounded health proof only;
  it is not documented here as complete enemy combat AI.

## VERIFIED

### Local engineering gates

All commands below were run against the current Release/Debug build after the
latest source changes. `PASS` means the command actually returned exit 0.

| Gate | Result | Evidence |
|---|---|---|
| Debug configure | `PASS` | `cmake --preset debug` |
| Debug build | `PASS` | `cmake --build --preset debug --config Debug` |
| Release configure | `PASS` | `cmake --preset release` |
| Release build | `PASS` | `cmake --build --preset release --config Release` |
| Debug CTest | `PASS` | 1/1 test passed |
| Release CTest | `PASS` | 1/1 test passed |
| Release direct unit executable | `PASS` | `182 tests, 0 failed` |
| Content compiler check | `PASS` | deterministic recompile matches |
| Content compiler tests | `PASS` | 7/7 |
| Systemic schema check | `PASS` | 1 file validated |
| Systemic schema tests | `PASS` | 10/10 |
| Seed compile | `PASS` | compiler command returned exit 0 |
| Static audit | `PASS` | `COUNT=0` |
| Contract check | `PASS` | forbidden/dependency/public-header checks OK |
| Debug smoke script | `PASS` | build, CTest, six `mapc` checks, app smoke all OK |
| Release direct smoke | `PASS` | executable returned exit 0 |
| Release benchmark | `PASS` | render/total/raycast/systemic/terminal budget checks OK |

The direct Release smoke reported:

```text
TERMINAL_BACKEND=win32-writeconsole
TERMINAL_DIMENSIONS=80x30
TERMINAL_QUALITY_PRESET=COMPATIBILITY
TERMINAL_COLOR_CAPABILITY=ANSI16
TERMINAL_PROBE=legacy-conhost
```

This is a real executable boot/smoke probe, not foreground Windows Terminal
visual evidence.

The latest benchmark labels its tail statistic
`worst_1pct_avg_ms`; it is not renamed to p99. Representative values from the
latest Release run were:

```text
raycast_column_sweep                         worst_1pct_avg_ms=0.118
systemic_kernel_lookup                      worst_1pct_avg_ms=0.021
systemic_update_workload                    worst_1pct_avg_ms=0.166
character_render_workload_240x67            worst_1pct_avg_ms=0.966
character_total_runtime_frame_240x67        worst_1pct_avg_ms=1.638
```

The total-runtime figure excludes platform terminal writes. It is useful
renderer/runtime proxy evidence, not an end-to-end 120 Hz proof.

### Release replay evidence

These were executed by the Release executable with separate user-data
directories and explicit assertions.

| Replay | Result | Important asserted state |
|---|---|---|
| `tools/replay/recovery_b1_success.txt` | `PASS` | B1 → calibration; badge held; terminal session; gate open/crossed; non-lethal hit; body created, hidden, discovered; cleaner response; save/load; chapter checkpoint reached |
| `tools/replay/recovery_b1_denied.txt` | `PASS` | access attempted and denied; gate remains closed; no checkpoint |
| `tools/replay/recovery_b1_terminal_denied.txt` | `PASS` | terminal attempted and denied; no terminal session |
| `tools/replay/recovery_b1_badge_only.txt` | `PASS` | badge held; `ACCESS_ATTEMPTED=NO`; no gate access |
| `tools/replay/recovery_b1_health_death.txt --room room_1f_security` | `PASS` | 1F guard health proof; player died; save/load recovery; expected state reached |

The successful B1 replay reported, among other fields:

```text
REPLAY_EXPECTED_STATE_REACHED=YES
CHAPTER_CHECKPOINT_REACHED=YES
SLICE_SHOT_HIT=YES NONLETHAL_HIT=YES BODY_CREATED=YES BODY_HIDDEN=YES
BODY_DISCOVERED=YES ACCESS_ATTEMPTED=YES ACCESS_DENIED=NO GATE_OPEN=YES
GATE_CROSSED=YES TERMINAL_SESSION=YES CLEANER_RESPONSE=YES
SAVE_OK=YES LOAD_OK=YES
```

The B1 badge-only replay reported `BADGE_HELD_BY_PLAYER=YES` while
`ACCESS_ATTEMPTED=NO`, which preserves the telemetry boundary that possession
is not an access attempt.

The health/death replay was also run with its original B1 start. That route did
not encounter the current valid guard and therefore did not satisfy its health
predicate. It was not relabeled as B1 success. The same Release replay,
explicitly started in `room_1f_security`, passed the bounded health/death and
recovery predicate. This is 1F health evidence, not complete enemy combat.

## NOT_VERIFIED

- No foreground Windows Terminal screenshot was captured in this recovery
  pass. The available direct smoke used the legacy-conhost/non-foreground
  probe above.
- No continuous manual 60–90 second Windows Terminal video was captured.
  Deterministic Release replays are not a substitute for manual play or video.
- No direct human mouse/keyboard playthrough was claimed.
- Linux/macOS/ARM64 remote CI was not yet observed at the time of this report;
  it remains a post-push verification gate.
- No fault-injected end-to-end failure during the final live save/load commit
  was run. The staged-load and bounded-loader tests are real, but this specific
  failure injection remains outside the verified set.
- General actor orientation/depth/opacity, LOD hysteresis, authored room
  entity/portal schema, general navigation, tactical combat, full quest graph,
  and event-driven narrator authority remain outside this slice.
- The current cleaner behavior is a bounded recovery action loop, not a
  general NPC navigation or negotiation system.

## USER_ACCEPTANCE_PENDING

Rain still needs to inspect the actual runtime presentation and, if desired,
manually exercise the B1/1F entry points. In particular, the following remain
acceptance items rather than automated claims:

- whether the Character-Art environment, NPC/body presentation, subtitles,
  and weapon viewmodel feel coherent in a real foreground terminal;
- whether the B1 interaction prompts are understandable without replay output;
- whether the cleaner discovery consequence is legible to a player;
- whether the bounded 1F damage proof feels like an appropriate slice without
  being mistaken for complete enemy combat.

## Truth matrix

The capability-by-capability boundary is recorded separately in
[`CAPABILITY_TRUTH_MATRIX.md`](CAPABILITY_TRUTH_MATRIX.md). The independent
finding ledger and post-implementation disposition are in
[`../audit/INTEGRATED_RECOVERY02_LEDGER.md`](../audit/INTEGRATED_RECOVERY02_LEDGER.md).

## Git and remote safety

- The implementation checkpoint was created on `main` only:
  `73e9e9a integration: close bounded recovery truth seams`.
- No branch, PR, force push, tag movement, release, or Steam action was used.
- The old `v0.1.0-pvs01-gold` tag was preserved.
- The pre-existing unrelated dirty files and prior visual evidence were not
  staged or deleted. The canonical worktree therefore intentionally remains
  dirty outside the focused checkpoint.
- Before any push, `.github/workflows/ci.yml` was inspected: normal `main`
  pushes trigger CI. `.github/workflows/release.yml` is gated by `v*` tags or
  explicit manual dispatch, so a normal main push does not create a release.

## Verdict

`READY_FOR_RAIN_SYSTEM_REVIEW`

This is the highest honest status for the bounded recovery slice. It means the
implemented B1/1F truth seams and their automated Release assertions are ready
for Rain's system review. It does not mean Product Gold, Visual Gold, public
release readiness, or completion of M1–M6.

```text
IMPLEMENTED = bounded B1/1F spatial, temporal, interaction, NPC, body,
               narrative, save/load, and replay truth seams listed above
VERIFIED = local Debug/Release gates plus explicit Release replay assertions
NOT_VERIFIED = foreground visual/video evidence, manual play, remote CI at report time,
               final-commit fault injection, and general M1-M6 capabilities
USER_ACCEPTANCE_PENDING = YES
```
