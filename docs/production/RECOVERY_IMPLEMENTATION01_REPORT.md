# WRITEOVER-07 Recovery Implementation-01

## Scope

This report records the bounded B1 truthful-player-loop recovery.  It does
not claim Product Gold, public-release readiness, or Rain's visual acceptance.
No new floor set, weapon set, release, tag, branch, pull request, or Steam
operation was created.

## IMPLEMENTED

- Credential possession is checked above the credential-validity predicate.
  The B1 reader path resolves the presented item, requires the current holder
  to be the player, validates revocation and clearance, and only then invokes
  the existing infrastructure door state.
- B1 interaction uses bounded target identity plus player pose, facing,
  proximity, and line-of-sight checks for the authored body, cart, camera,
  terminal, NPC, and reader targets.
- The existing infrastructure door is the authoritative B1 gate.  Denial
  leaves it closed; success unlocks and opens it, changes collision/passability,
  emits the door event, and survives the bounded save/load route.
- Terminal access requires the targeted terminal, a player-held valid badge,
  and clearance.  A denied attempt does not create a session or audit/route
  state; a successful attempt creates the session and audit state.
- Autonomous perception excludes stunned and dead actors from direct-witness
  behavior.
- Continuous observations and repeated gunshots use the existing memory store
  with a semantic key and bounded refresh window, instead of appending an
  identical memory at every decision interval.
- The cleaner executes deterministic movement toward the cart, collision
  rejection, arrival-radius checking, a bounded inspection interval, body
  discovery, and a separate response phase.  Discovery is not a due-frame-only
  mutation and a hidden body in another room is rejected.
- Guard line-of-sight damage now reaches the authoritative player health state.
  Health is reflected by the normal HUD, death removes movement/interaction
  authority, and bounded health/dead state is persisted and restored.
- A bounded non-lethal hit creates a runtime unconscious body, transfers the
  credential from the incapacitated actor, and connects search, badge
  acquisition, drag, hide, cleaner discovery, and response through existing
  systemic seams.
- Replay output separates process exit, input consumption, expected gameplay
  state, and chapter checkpoint state.  Success and negative routes require
  explicit state assertions rather than exit code alone.
- The minimal Windows terminal initialization needed for the real capture
  enables VT processing and UTF-8 output.  The existing Character-Art runtime
  path and clean player HUD remain in use.

## VERIFIED

### Local code and regression gates

The following were run against the canonical repository after the recovery
changes:

- Windows Release configure: PASS
- Windows Release build: PASS
- Windows Debug configure/build: PASS
- Release CTest: PASS, 1/1 CTest target
- Debug CTest: PASS, 1/1 CTest target
- Direct Release test executable: PASS, 172 tests, 0 failed
- Content compiler check: PASS, deterministic recompile matches
- Content compiler tests: PASS, 7/7
- Systemic schema check: PASS
- Systemic schema tests: PASS, 10/10
- Static audit: PASS, `COUNT=0`
- Contract check: PASS
- Direct Windows Release smoke: PASS, exit 0
- Debug smoke wrapper: PASS
- Release benchmark: PASS

The repository has no Release CTest preset for `scripts/smoke.ps1`; therefore
`scripts/smoke.ps1 -Preset release` is not an applicable command in this
repository.  The direct Release executable smoke was run and passed.  This
wrapper mismatch is retained as a tooling limitation, not relabeled as a
Release smoke failure.

### Truthful B1 replay evidence

- `replay_b1_success.txt`: Release success route reached non-lethal hit, body
  creation, search, badge acquisition, drag/hide, cleaner discovery/response,
  terminal session, real gate open/crossing, save, and load.  It reported
  `REPLAY_EXPECTED_STATE_REACHED=YES` and `CHAPTER_CHECKPOINT_REACHED=YES`.
- `replay_b1_gate_denied.txt`: same gate target without the credential reported
  `ACCESS_ATTEMPTED=YES`, `ACCESS_DENIED=YES`, `GATE_OPEN=NO`, and
  `GATE_CROSSED=NO`.
- `replay_b1_terminal_denied.txt`: terminal target without a badge reported
  `TERMINAL_ATTEMPTED=YES`, `TERMINAL_DENIED=YES`, and no terminal session.
- `replay_b1_health_death.txt`: Release replay reported real player death,
  save/load, recovery, and positive live health after recovery.
- Unit-level counterfactuals cover badge on the body, badge held by another
  entity, player-held badge, transferred/dropped badge, and revoked badge.
- Unit-level cleaner counterfactuals cover pre-arrival, blocked route, another
  room, and incapacitated cleaner cases.  The body remains hidden and no
  response is emitted when the cleaner cannot perceive/arrive.
- Unit-level memory cases cover continuous sight, repeated gunshot, and a
  separate meaningful event.

### Remote CI

The implementation head was pushed to `main` and the normal CI workflow was
rerun after one macOS arm64 benchmark-runner outlier.  Run
`34149110181` completed successfully for Windows, Linux, Linux Clang, macOS
arm64, and ARM64 link.  Its successful jobs covered content/schema checks,
Debug and Release builds/tests, smoke, benchmark, forbidden-pattern,
dependency, public-header, and static-audit gates as configured by
`.github/workflows/ci.yml`.

The workflow review confirmed that a normal push to `main` invokes CI only.
The release workflow is tag/workflow-dispatch gated; no release, tag, or Steam
operation was performed.

### Real runtime evidence

- Real Windows Release default entry was launched in a maximized Windows
  Terminal.
- Captured runtime diagnostics selected `ansi-truecolor`, `240x67`,
  `ULTRA120`, `TRUECOLOR`, and `windows-terminal`.
- The real screenshot contains the Character-Art world path, authored NPC
  strokes, pistol viewmodel, and player HUD without raw CSI or UTF-8 mojibake.
- The real continuous capture is 8.000 seconds, 2560x1440, H.264, 80 frames
  at 10/1.  It is executable output, not an SVG/CharCell reconstruction.

Evidence files:

- [real Windows Terminal screenshot](evidence/recovery_implementation01/real_windows_terminal_c01.png)
- [real Windows Terminal runtime capture](evidence/recovery_implementation01/real_windows_terminal_c01.mp4)
- [runtime evidence record](evidence/recovery_implementation01/real_windows_terminal_c01_runtime.md)
- [B1 replay evidence](evidence/recovery_implementation01/replay_b1_success.txt)
- [counterfactual replay evidence](evidence/recovery_implementation01/replay_b1_gate_denied.txt)
- [terminal denial evidence](evidence/recovery_implementation01/replay_b1_terminal_denied.txt)
- [health/death evidence](evidence/recovery_implementation01/replay_b1_health_death.txt)
- [code-truth red team](evidence/recovery_implementation01/red_team_code_truth.md)
- [behavior-truth red team](evidence/recovery_implementation01/red_team_behavior_truth.md)
- [player-experience red team](evidence/recovery_implementation01/red_team_player_experience.md)

## NOT_VERIFIED

- No independent manual 60–90 second playthrough was performed.  Deterministic
  replay proves the asserted state chain but is not a substitute for manual
  player input and comprehension.
- No manual screen recording of a 60–90 second session exists.  The committed
  runtime video is 8 seconds; `VIDEO_CAPTURE=PENDING_MANUAL` for the requested
  duration.
- Rain's visual acceptance of the Character-Art composition, NPC readability,
  pistol recognition, contrast, comfort, and overall style is not verified.
- Linux, macOS, and ARM64 were verified by their configured CI build/test/link
  gates, not by local interactive gameplay or terminal screenshots.
- The cleaner movement is bounded deterministic direct movement with collision
  rejection, not general pathfinding.  Older room transitions outside the B1
  authored target set still contain legacy proximity seams.
- The benchmark reports `worst_1pct_avg_ms`, the average of the slowest 1% of
  samples; it is not a strict p99 quantile and does not prove end-to-end 120 Hz
  terminal presentation.  Platform writes are excluded from the integrated
  proxy measurement.
- Full game scope, all floors, generalized negotiation, and non-B1 authoring
  are outside this recovery implementation.

## USER_ACCEPTANCE_PENDING

Rain should open the real screenshot and video, launch the Windows Release
executable from the canonical root, and play the bounded B1 route manually.
The final questions are whether the player can orient, understand the target,
perform the non-lethal interaction, obtain and use the badge, perceive the
cleaner consequence, and reach the checkpoint without the scene reading as a
debug tool.  Visual acceptance remains pending even though the engineering
gates are green.

## Status record

OLD_ROOT = `D:\Edge Download\MUD游戏\WRITEOVER-07` (preserved)

NEW_ROOT = `D:\AAAbiancheng\00_Projects\40_Coursework\2026_CPP_Immersive_ASCII_FPS`

IMPLEMENTATION_HEAD = `18be6190fe1cbe8ec47c88f6d1a3065985719062`

CURRENT_HEAD = `HEAD after the report-only closure commit; resolve with git rev-parse HEAD`

ORIGIN_MAIN = `origin/main; must equal CURRENT_HEAD after the final push`

WORKTREE = CLEAN at the last source/evidence validation; rechecked after report closure

COMMITS_CREATED = 4 recovery commits after the pre-existing Character Reboot checkpoint

RECOVERY_COMMITS = `96975a2`, `4b53b88`, `e1a9ebd`, `18be619`

CHARACTER_REBOOT_CHECKPOINT = `f0188855350b77df2d97693a3f544df8698dc085`

PUSHED = YES for the implementation head; report closure push is part of final handoff

REMOTE_CI = SUCCESS for implementation head rerun `34149110181`; final report-closure run is recorded in the final handoff

CREDENTIAL_TRUTH = VERIFIED for bounded B1 success/denial and systemic counterfactuals

DOOR_TRUTH = VERIFIED for B1 lock/unlock/open/passability, crossing, and bounded save/load route

TERMINAL_TRUTH = VERIFIED for targeted holder/validity/clearance success and denial

MEMORY_DEDUP = VERIFIED for continuous sight and repeated gunshot refresh

UNCONSCIOUS_WITNESS = VERIFIED: stunned/dead actors cannot become direct witnesses in the corrected path

CLEANER_REAL_MOVEMENT = VERIFIED for bounded deterministic movement and collision rejection

DISCOVERY_REQUIRES_ARRIVAL = VERIFIED with arrival radius plus inspection interval

PLAYER_HEALTH = VERIFIED for damage, HUD reflection, death, save/load, and recovery

NONLETHAL = VERIFIED for runtime hit to unconscious body

BODY_LIFECYCLE = VERIFIED for runtime body, search, drag, hide, discovery, and response

DRAG_HIDE = VERIFIED for bounded B1 movement/state/capacity path

SCENE_RELATIVE_TIME = VERIFIED for B1 intro and cleaner action timing

REPLAY_GAMEPLAY_ASSERTION = VERIFIED: explicit state assertions separate from process exit

COUNTERFACTUAL_CREDENTIAL = VERIFIED: held/player/other/dropped/transferred/revoked and replay denial cases

COUNTERFACTUAL_DISCOVERY = VERIFIED: arrival, blocked, another-room, and incapacitated-cleaner cases

REAL_WINDOWS_TERMINAL_SCREENSHOT = VERIFIED: `evidence/recovery_implementation01/real_windows_terminal_c01.png`

REAL_VIDEO = VERIFIED: real executable capture, 8 seconds; requested manual 60–90 seconds NOT_VERIFIED

CHARACTER_VISUAL_ACCEPTANCE = PENDING

OPEN_FATAL = 0 for the bounded recovery slice

OPEN_MAJOR = 0 for the bounded recovery slice

OPEN_P0 = 0 for the exercised recovery slice

OPEN_P1 = 0 for the exercised recovery slice

VERDICT = READY_FOR_RAIN_REVIEW

