# WRITEOVER-07 — Chapter One first-audit remediation

Audit source: `D:\AAAbiancheng\00_Projects\_audit_runs\WRITEOVER-07\20260909_chapter01_full_audit`

This ledger classifies every record in `LIVE_FINDINGS.jsonl` against the
current Alpha-03 continuation head. It is a current-code receipt, not an
amendment to the historical audit snapshot. The remediation commit is
`2efde27325dd26baeb2fdc33b584acdfa81c6911`.

## Summary

* Records classified: 14/14.
* Original P1 records: 4; current-scope P1 records closed: 4.
* Current confirmed Fatal/P0: 0/0.
* Current normal-play P1: 0.
* One objective P3 (`FindSceneTransitionAt` dead data) remains `FUTURE_SCOPE`
  because the current authored route is already driven by stable transition IDs
  and a broad transition-runtime refactor is not required for Chapter One.
* Supplemental Audit-02B is not included in this ledger. Its coverage gate is
  still incomplete (`FILE_COVERAGE_PASS=NO`, all three coverage percentages
  `0%`), and no 02B findings file is available.

## Per-finding ledger

### DS-C1-AUDIT-0001

* `ID`: DS-C1-AUDIT-0001
* `ORIGINAL_SEVERITY`: P1
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: Disable `guard_7`, use the authored `security_to_elevator`
  route, and enter the elevator lobby without the normal security interaction.
* `ROOT_CAUSE`: Entry accepted `security_guard_disabled()` while the terminal
  completion branch required `security_checkpoint`; the destination had no
  useful recovery exit.
* `FIX`: The shared `enter_scene_transition` path now records the durable
  `fact_chapter_security_checkpoint` when the disabled-guard authorization is
  the reason the security-to-elevator link is accepted. The entry and completion
  predicates therefore agree without teleporting or weakening the checkpoint.
* `TEST`: `tools/replay/chapter01_security_bypass.txt`, 3,500 frames; the
  scenario matrix also executes this counterfactual.
* `RESULT`: PASS. Current receipt includes `CHAPTER_CHECKPOINT_REACHED=YES`,
  `FACT_CHAPTER_SECURITY_CHECKPOINT=YES`, elevator visit, and no transition
  denial. The 18-case replay gate passes.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: Other future links must keep one authorization owner; no
  current Chapter One link remains untested by this fix.

### DS-C1-AUDIT-0002

* `ID`: DS-C1-AUDIT-0002
* `ORIGINAL_SEVERITY`: P2
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: Obtain and hold the badge, open the B1 gate, do not use the
  calibration terminal, and attempt to cross.
* `ROOT_CAUSE`: The B1 gate called `switch_room` directly while the generic
  transition policy required a badge plus an active terminal session.
* `FIX`: The gate now calls `enter_scene_transition("b1_to_calibration")`, so
  one transition policy owns the crossing. The bounded loud route is an
  explicit durable `fact_b1_loud_action` alternative, not an accidental skip.
* `TEST`: `tools/replay/chapter01_terminal_skip_denied.txt`, 2,000 frames;
  recovery replay gate and scenario matrix.
* `RESULT`: PASS. Badge-only/no-terminal route reports `GATE_OPEN=YES`,
  `TRANSITION_DENIED=YES`, `GATE_CROSSED=NO`, `TERMINAL_SESSION=NO`, and no
  calibration-room visit. Normal quiet and loud routes still pass.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: The loud route is intentionally bounded current content,
  not a general bypass language for future chapters.

### DS-C1-AUDIT-0003

* `ID`: DS-C1-AUDIT-0003
* `ORIGINAL_SEVERITY`: P2
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: Stun or kill `guard_7` in `room_1f_security` and inspect the
  active room after the standing sprite disappears.
* `ROOT_CAUSE`: Body creation and body rendering were both hard-coded to the
  B1 officer/body and the body loop was nested inside the B1 render branch.
* `FIX`: Shot feedback creates a bounded BodyRecord for any incapacitated
  runtime NPC, with a deterministic derived ID for non-primary actors. The
  room-local body render loop is outside scene-specific NPC branches and uses
  the authoritative BodyRecord position and floor pose.
* `TEST`: `chapter01_security_bypass` exercises a stunner body in the security
  route; `spatial.body_world_presentation` covers body pose/move/hide; the
  206-test Debug and Release direct executables pass.
* `RESULT`: PASS. No standing guard remains after incapacitation; the runtime
  body is available to the systemic path and is rendered in its active room.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: This is bounded body presentation, not a full corpse/loot
  authoring catalogue for future rooms.

### DS-C1-AUDIT-0004

* `ID`: DS-C1-AUDIT-0004
* `ORIGINAL_SEVERITY`: P3
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `FALSE_POSITIVE`
* `REPRODUCTION`: The original claim that `storylet_r1_02` could never fire is
  contradicted by the audit's own trace: authored facts are seeded and the
  storylet is eligible at frame 0.
* `ROOT_CAUSE`: The original finding conflated the separate residual issue that
  `fact_r1_guard_dead` was not updated on death; that residual issue is
  DS-C1-AUDIT-0012.
* `FIX`: No code change for the false claim. The residual fact-pipeline issue
  was fixed under 0012.
* `TEST`: Current 206-test direct suites, Alpha/Chapter aggressive replays,
  and fact receipt.
* `RESULT`: FALSE_POSITIVE retained as a historical classification; no
  unclassified defect remains under this ID.
* `COMMIT`: `N/A (historical false-positive; residual fixed in 0012)`
* `REMAINING_RISK`: None for the original claim; future storylet coverage still
  needs its own truthful route assertions.

### DS-C1-AUDIT-0005

* `ID`: DS-C1-AUDIT-0005
* `ORIGINAL_SEVERITY`: P3
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `FUTURE_SCOPE`
* `REPRODUCTION`: The current authored Chapter One route always supplies the
  badge path, so no current replay reaches the alternate bribe branch; the old
  local flags were also not sufficient after load.
* `ROOT_CAUSE`: The branch predates the current bounded route and mixed local
  `bribe_done`/`schedule_found` flags with durable facts/knowledge.
* `FIX`: The current branch now derives bribe completion from durable
  `fact_alpha_bribe_accepted` and derives schedule discovery from systemic
  knowledge. It is not made artificially reachable by changing Chapter One
  content.
* `TEST`: 206 direct tests and current save/load replays pass; source review
  confirms durable reconstruction. There is no honest current bribe fixture.
* `RESULT`: The persistence defect is corrected; alternate bribe reachability
  remains future content scope, not a current normal-play failure.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: A future author must add a real counterfactual fixture before
  presenting bribe as an active Chapter One route.

### DS-C1-AUDIT-0006

* `ID`: DS-C1-AUDIT-0006
* `ORIGINAL_SEVERITY`: P1
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: Inspect the committed replay list and CI command, then count
  the cases actually invoked.
* `ROOT_CAUSE`: The gate stopped at the nine Recovery/Alpha cases while the
  report claimed a 15-case current gate.
* `FIX`: `scripts/recovery_replay_gate.ps1` now runs the nine original cases,
  six Chapter One cases, and three bounded regression fixtures: 18 cases total.
  The Windows CI job invokes this same script and does not substitute a report.
* `TEST`: Current Release gate run and `.github/workflows/ci.yml` source audit.
* `RESULT`: PASS. `RECOVERY_REPLAY_GATE=PASS`; all 18 cases separate process,
  input, expected-state and result receipts. Negative routes expect denial, not
  fake success.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: Remote CI must still be observed on the pushed final head;
  local success is not a remote receipt.

### DS-C1-AUDIT-0007

* `ID`: DS-C1-AUDIT-0007
* `ORIGINAL_SEVERITY`: P3
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `FUTURE_SCOPE`
* `REPRODUCTION`: `FindSceneTransitionAt` has no current caller and authored
  transition bounds are not used by the current interaction-driven route.
* `ROOT_CAUSE`: The bounded SceneTransition data model contains a spatial helper
  from an earlier interaction concept; current normal routes resolve stable
  transition IDs after explicit target focus.
* `FIX`: No broad transition-runtime refactor was added. Current route links are
  still compiled, validated, loaded and consumed by ID; the dead helper is not
  claimed as a live capability.
* `TEST`: Content check, 13/13 content tests, 10/10 schema tests, 206 direct
  tests, all current replays and scenario matrix pass.
* `RESULT`: Deferred as objective future-scope hygiene; not a current Chapter
  One soft-lock or correctness failure.
* `COMMIT`: `N/A (future-scope boundary)`
* `REMAINING_RISK`: Before spatially selecting arbitrary future links, either
  wire this helper or remove the dead bounds contract with a dedicated test.

### DS-C1-AUDIT-0008

* `ID`: DS-C1-AUDIT-0008
* `ORIGINAL_SEVERITY`: P1
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: Start `room_1f_security` without a manual save, allow the
  valid guard damage path to kill the player, and press F9.
* `ROOT_CAUSE`: The dead branch accepted F9 but a missing save only emitted a
  generic load failure and left the authoritative player dead indefinitely.
* `FIX`: A failed F9 while dead and in an authored room now resets that room's
  player pose to its authored spawn, clears velocity, restores health to 100,
  and presents an explicit restart subtitle. A valid save still uses normal
  staged load.
* `TEST`: `tools/replay/chapter01_no_save_death.txt`, 3,000 frames, plus the
  existing checkpoint-death replay and direct unit coverage.
* `RESULT`: PASS. Current receipt includes `PLAYER_DIED=YES`,
  `PLAYER_RESTARTED=YES`, health 100 and `PLAYER_DEAD=NO`; no input limbo.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: Restart is intentionally a bounded authored-room recovery,
  not a complete game-over/menu framework.

### DS-C1-AUDIT-0009

* `ID`: DS-C1-AUDIT-0009
* `ORIGINAL_SEVERITY`: P2
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: The audit measured 57 assets and 43 rows against 64/48 hard
  limits and showed that crossing either cap discarded the whole authored bank.
* `ROOT_CAUSE`: Parser bounds were too close to the current authored bank and
  all-or-nothing fallback was silent at ordinary content growth.
* `FIX`: Bounded parser limits are now 128 assets, 64 rows and 64 columns;
  these are explicit content budgets, not unlimited input.
* `TEST`: Release/Debug direct suites and content checks pass; source and
  Visual Bible record the same caps.
* `RESULT`: PASS for the current content budget. No fallback occurs for the
  current authored bank; per-record isolation remains outside this slice.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: Future content must remain within the bounded budget and
  should add a focused overflow test before increasing it again.

### DS-C1-AUDIT-0010

* `ID`: DS-C1-AUDIT-0010
* `ORIGINAL_SEVERITY`: P2
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → receipt corrected
* `REPRODUCTION`: Compare the historical Alpha-02 15/15 statement with the old
  committed nine-case gate and the uncovered elevator soft-lock.
* `ROOT_CAUSE`: A local 15-case run was documented without updating the
  committed gate, and the soft-lock assertion was overstated.
* `FIX`: The committed gate now executes 18 cases, including the soft-lock
  counterfactual, no-save recovery and terminal-skip denial. This remediation
  ledger and the Alpha-03 report explicitly distinguish historical statements
  from current receipts.
* `TEST`: Current 18-case gate and 36-row scenario matrix.
* `RESULT`: PASS for current receipt truth; the old Alpha-02 document remains
  historical and is not used as current evidence.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: Remote CI receipt is pending until the final push.

### DS-C1-AUDIT-0011

* `ID`: DS-C1-AUDIT-0011
* `ORIGINAL_SEVERITY`: P2
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → bounded consumer added
* `REPRODUCTION`: Run the cleaner report path and inspect whether the durable
  facility alert affects any player-facing Chapter One state.
* `ROOT_CAUSE`: `SystemicWorld::AlertLevel()` was written by a report response
  but not read by production code; per-NPC `alertness` was likewise diagnostic.
* `FIX`: The security objective now consumes `AlertLevel()`: Suspicious state
  presents the bounded elevated-response objective. `RuntimeNpc::alertness`
  remains explicitly diagnostic and is not claimed as a live gameplay consumer.
* `TEST`: Alpha/Chapter memory and aggressive replays, direct systemic tests,
  and Release smoke/benchmark.
* `RESULT`: PASS for one bounded observable consumer; no claim of a global
  security escalation framework.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: Future alert consumers need their own integration tests;
  monotonic current alert semantics remain documented.

### DS-C1-AUDIT-0012

* `ID`: DS-C1-AUDIT-0012
* `ORIGINAL_SEVERITY`: P3
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: Kill the B1 badge owner and inspect the durable fact state.
* `ROOT_CAUSE`: The fact was seeded false and consumed by storylet content, but
  the death feedback path never set it true.
* `FIX`: Primary B1 death feedback now writes `fact_r1_guard_dead=true`; the
  reset registry includes the fact so each authored replay begins deterministically.
* `TEST`: Aggressive Alpha/Chapter replays and current fact receipt
  `FACT_R1_GUARD_DEAD=YES`; 206 direct tests pass.
* `RESULT`: PASS.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: The current fact is bounded to the primary B1 guard; other
  authored deaths need separate fact definitions if future content needs them.

### DS-C1-AUDIT-0013

* `ID`: DS-C1-AUDIT-0013
* `ORIGINAL_SEVERITY`: P3
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: Remove/avoid the chapter objective and check whether a
  generic room objective can still satisfy the route latch.
* `ROOT_CAUSE`: The previous latch became true for any non-empty objective,
  including fallback room text.
* `FIX`: `RenderModule` now accepts an explicit presentation prefix; Chapter One
  sets `B1:` and only that active quest objective records
  `ObjectiveWasPresented`.
* `TEST`: All current success/denial/memory/backtrack replays, scenario matrix,
  and 206 direct tests. The success predicate requires the durable observation
  plus completion/checkpoint; it does not require a completed quest to retain an
  active objective.
* `RESULT`: PASS; the old tautology is no longer the current assertion.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: A future quest needs its own presentation identity rather
  than reusing the B1 prefix.

### DS-C1-AUDIT-0014

* `ID`: DS-C1-AUDIT-0014
* `ORIGINAL_SEVERITY`: P1
* `CURRENT_HEAD`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `CURRENT_CLASSIFICATION`: `CONFIRMED_CURRENT` → fixed and verified
* `REPRODUCTION`: Build a player package from the Release executable and run it
  without the repository working directory; inspect authored art and narrative
  resolution.
* `ROOT_CAUSE`: Package copy logic only selected `.bin`/`.woc`, omitting the
  runtime-loaded character-art and recovery text `.txt` resources.
* `FIX`: `package_release.py` copies only the bounded runtime text directories
  (`data/characters` and `data/text`) in addition to compiled content. The
  package smoke now requires the two current runtime text assets and rejects
  fallback-art/failed-closed narrative diagnostics.
* `TEST`: Final Windows package build plus `package_smoke.py`; the smoke reports
  `PACKAGE_SECRET_SCAN=PASS`, `PACKAGE_DEV_GARBAGE=0`, resource-root and user
  data separation PASS. The same package smoke is a Windows CI step.
* `RESULT`: PASS.
* `COMMIT`: `2efde27325dd26baeb2fdc33b584acdfa81c6911`
* `REMAINING_RISK`: New runtime-loaded asset classes require an explicit package
  rule and corresponding smoke assertion; no broad asset manifest is claimed.
