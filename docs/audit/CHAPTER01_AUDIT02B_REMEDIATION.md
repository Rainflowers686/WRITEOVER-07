# WRITEOVER-07 — Chapter One Audit-02B remediation ledger

This ledger reconciles every `DS-C1-02B-xxxx` record from the completed
supplemental audit against the current implementation checkpoint
`71d2aa01efea5e6b7f08035bd66146956eb5bcd1`. The audit snapshot was based on
`4c809a8b0340808b1db287db3f71a340bcc28ea4`; it is evidence about that snapshot,
not a substitute for the current-head checks below.

## Coverage receipt

```text
AUDIT_ROOT = D:\AAAbiancheng\00_Projects\_audit_runs\WRITEOVER-07\20260909_chapter01_audit_02B_coverage
AUDIT_COMPLETE.flag = PRESENT
AUDIT_FILE_TOTAL = 430
FILES_REVIEWED = 430
TOTAL_CHUNKS = 432
PASS1_CHUNK_COVERAGE = 100%
PASS2_CODE_CHUNK_COVERAGE = 100%
PASS3_HIGH_RISK_COVERAGE = 100%
UNREVIEWED_FILES = 0
TOTAL_FINDINGS = 29
ACTIVE_FINDINGS = 26
FATAL = 0
P0 = 0
P1 = 5
P2 = 6
P3 = 15
FALSE_POSITIVE = 3
GATE_STATUS = PASS
```

`LIVE_FINDINGS.jsonl`, `FINAL_AUDIT_REPORT.md`, `HANDOFF_TO_CODEX.md`,
`MASTER_FINDING_LEDGER.md`, `STATE_OWNERSHIP_MATRIX.csv`,
`PRODUCTION_REACHABILITY.csv`, and `TEST_BLINDSPOTS.md` were read from that
root. No audit snapshot was recreated or deleted by this remediation.

The `CURRENT_HEAD` value in each entry is the source/test checkpoint at which
the finding was reconciled. The later documentation receipt commit changes
only reports and does not change that implementation evidence.

## Reconciled findings

### DS-C1-02B-0001

```text
ID = DS-C1-02B-0001
ORIGINAL_SEVERITY = P1 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = CONFIRMED_CURRENT
REPRODUCTION = Before the remediation, Run(0) had no production RequestStop caller; the current direct Release normal_quit replay exits through the production input path.
ROOT_CAUSE = Engine stop authority existed but normal player input did not reach it, and module shutdown was not wired after the loop.
FIX = Pause prompt now exposes the existing Q gameplay binding as the intentional quit affordance; the callback calls Engine::RequestStop, Engine shuts down registered modules, and InputRuntime shutdown is idempotent.
TEST = tools/replay/normal_quit.txt; direct Release replay; recovery_replay_gate.ps1; engine.request_stop_terminates_run; engine.shutdowns_modules_once.
RESULT = PASS: REPLAY_PROCESS_EXIT_OK=YES, REPLAY_INPUT_CONSUMED=YES, REPLAY_EXPECTED_STATE_REACHED=YES, NORMAL_QUIT_REQUESTED=YES, exit=0.
COMMIT = 71d2aa0
REMAINING_RISK = Foreground manual console restoration remains a host-specific acceptance item; the non-foreground probe honestly reports its fallback.
```

### DS-C1-02B-0002

```text
ID = DS-C1-02B-0002
ORIGINAL_SEVERITY = P1 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = Baseline disabled-guard entry could reach the elevator lobby without the completion fact; current chapter01_security_bypass was rerun on the Release binary.
ROOT_CAUSE = Security bypass entry and elevator completion used separate predicates.
FIX = First remediation records the durable security checkpoint when the disabled-guard route is legitimately used; the current route reaches the authored elevator checkpoint without an unexplained lobby limbo.
TEST = chapter01_security_bypass replay and 19-case recovery replay gate.
RESULT = PASS: expected state and chapter checkpoint reached; FACT_CHAPTER_SECURITY_CHECKPOINT=YES; no route failure.
COMMIT = 2efde27
REMAINING_RISK = The route remains a bounded Chapter One rule, not a general elevator authorization system.
```

### DS-C1-02B-0003

```text
ID = DS-C1-02B-0003
ORIGINAL_SEVERITY = P1 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = The baseline package copier omitted authored character and narrative text; current package staging includes the bounded runtime text set.
ROOT_CAUSE = copy_runtime_data filtered only compiled binary extensions.
FIX = Package release copies .bin/.woc plus authored .txt under data/characters and data/text.
TEST = Clean package build and package_smoke.py; inventory includes b1_character_art.txt and recovery_text.txt; package launch used an unrelated working directory.
RESULT = PASS: package resource root, executable-relative data, authored art and text resolution, and separated user data.
COMMIT = 2efde27
REMAINING_RISK = Future runtime text directories need an explicit packaging rule or manifest update.
```

### DS-C1-02B-0004

```text
ID = DS-C1-02B-0004
ORIGINAL_SEVERITY = P2 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = Current package smoke checks inventory before launch and validates required runtime character/text resources.
ROOT_CAUSE = The old smoke gate treated process exit as sufficient.
FIX = package_smoke.py requires the authored art/text files, fails on missing-resource inventory, checks no fallback/FAILED_CLOSED output, and runs outside the package directory.
TEST = Normal package smoke PASS; a derived archive with data/characters/b1_character_art.txt removed is an EXPECTED_FAILED_PROBE with exit=1 and the exact missing-resource diagnostic.
RESULT = PASS for the positive package and PASS for the negative gate behavior.
COMMIT = 2efde27
REMAINING_RISK = The negative probe is an operator-side validation of the gate, not a separate committed package fixture.
```

### DS-C1-02B-0005

```text
ID = DS-C1-02B-0005
ORIGINAL_SEVERITY = P1 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = The baseline gate executed nine cases while receipts claimed fifteen; the current committed script enumerates the Recovery, Alpha-01 and Chapter One cases plus the current normal-quit regression.
ROOT_CAUSE = CI invoked a gate whose case list lagged the documented replay set.
FIX = recovery_replay_gate.ps1 now runs 19 named cases, including all current Recovery/Alpha/Chapter cases and normal_quit; every case checks process, input, expected state and result separately.
TEST = Current Release recovery_replay_gate.ps1: all 19 cases PASS.
RESULT = PASS: actual script output and case count agree at 19; the old 15/15 statement remains historical only.
COMMIT = 2efde27 plus 71d2aa0 for normal_quit coverage
REMAINING_RISK = Any future mandatory fixture must be added to both the committed list and its receipt; no claim is made for obsolete PVS01 inputs.
```

### DS-C1-02B-0006

```text
ID = DS-C1-02B-0006
ORIGINAL_SEVERITY = P1 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = Current no-save-death and checkpoint-death replays exercise the dead state and recovery path.
ROOT_CAUSE = Dead-player input accepted only F9 while a missing save had no authored restart behavior.
FIX = A dead player with no valid save can use the existing load/recovery action to restart the currently loaded authored room at its spawn with full health and an explicit player-facing message.
TEST = chapter01_no_save_death and recovery_b1_health_death in the current Release gate; scenario matrix failure-state rows.
RESULT = PASS: PLAYER_RESTARTED=YES and the player is live after recovery; valid-save recovery remains covered separately.
COMMIT = 2efde27
REMAINING_RISK = This is bounded room restart behavior, not a general game-over menu.
```

### DS-C1-02B-0007

```text
ID = DS-C1-02B-0007
ORIGINAL_SEVERITY = P2 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = Badge-only/no-terminal crossing is rerun through chapter01_terminal_skip_denied.
ROOT_CAUSE = A focused B1 gate branch bypassed the authored shared transition predicate.
FIX = B1 crossing now consumes the shared transition policy requiring a valid held badge and terminal session or the explicitly authored loud-action alternative.
TEST = chapter01_terminal_skip_denied and recovery_b1_badge_only; direct Release gate.
RESULT = PASS: denied route reports TRANSITION_DENIED=YES and does not cross; badge-only reports ACCESS_ATTEMPTED=NO where appropriate.
COMMIT = 2efde27
REMAINING_RISK = The loud-action alternative is an intentional bounded route rule, not a bypass for arbitrary rooms.
```

### DS-C1-02B-0008

```text
ID = DS-C1-02B-0008
ORIGINAL_SEVERITY = P2 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = Current aggressive/security and body paths create and retain a body for an incapacitated actor outside the B1-only branch.
ROOT_CAUSE = Body synchronization and rendering were restricted to the primary B1 guard.
FIX = Runtime incapacitated actors receive room-local BodyRecords and the body renderer consumes those records across authored rooms.
TEST = Current Release chapter01_security_bypass plus unit/body and spatial regression coverage.
RESULT = PASS: the security route retains a body state and no invisible authoritative incapacitated actor was observed in the current route receipt.
COMMIT = 2efde27
REMAINING_RISK = Manual foreground visual confirmation of every room remains separate from engineering replay evidence.
```

### DS-C1-02B-0009

```text
ID = DS-C1-02B-0009
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = The current authored bank is within the widened bounded parser budget; no silent overflow fallback was encountered.
ROOT_CAUSE = The old 64-asset/48-row budget left little headroom and overflow surfaced as runtime fallback.
FIX = The bounded art-bank budget was widened in the first remediation for the current authored set; content remains finite and load failure remains explicit.
TEST = Current authored resource load through Release smoke/package smoke and static/content gates.
RESULT = PASS for the current bank; current content is 57 records/43 rows within the widened cap.
COMMIT = 2efde27
REMAINING_RISK = The art bank is not an unlimited asset pipeline; future content must remain within the documented cap or split intentionally.
```

### DS-C1-02B-0010

```text
ID = DS-C1-02B-0010
ORIGINAL_SEVERITY = P2 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = Facility alert is written by the report path and is read by the current security objective source; per-NPC alertness remains a diagnostic/runtime field, not a claimed full AI consumer.
ROOT_CAUSE = The audit snapshot evaluated the field before the bounded objective consumer was present.
FIX = The security objective changes when FacilityAlertLevel reaches Suspicious or higher; the capability matrix explicitly limits the claim and does not advertise a complete alert AI.
TEST = Current Release chapter routes and objective/replay receipt; AlertLevel() consumer at the security objective source.
RESULT = PASS for the bounded facility-alert consumer; no false claim is made for NPC alertness.
COMMIT = 2efde27
REMAINING_RISK = NPC alertness remains foundation/diagnostic state until a future bounded consumer is intentionally added.
```

### DS-C1-02B-0011

```text
ID = DS-C1-02B-0011
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = FUTURE_SCOPE
REPRODUCTION = FindSceneTransitionAt has no production caller; current normal room movement uses focused authored SceneEntity interactions and the shared transition predicate.
ROOT_CAUSE = A bounded coordinate-query helper and authored bounds were retained while current route authority moved to focused interactions.
FIX = No speculative second transition path was introduced; the current route remains explicit and the dead helper is recorded as future cleanup.
TEST = Current transition/replay tests validate the active path; no test claims coordinate crossing through this unused helper.
RESULT = Not a current player-facing defect in the bounded Chapter One route.
COMMIT = none
REMAINING_RISK = If future content wants coordinate crossing, it must either wire this helper with tests or remove the dead data/helper together.
```

### DS-C1-02B-0012

```text
ID = DS-C1-02B-0012
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = FUTURE_SCOPE
REPRODUCTION = The current security route normally requires the badge, so the retained bribe branch is not reached by current authored fixtures; local flags are not treated as durable capability.
ROOT_CAUSE = The branch belongs to an older optional route and its transient flags are not a stable current save contract.
FIX = No new social route was invented merely to satisfy the audit. The current capability/reporting boundary does not claim the branch as normal-play behavior.
TEST = Current denied/security route tests preserve credential truth; no test falsely claims bribe reachability.
RESULT = Deferred as bounded future social content, not an open current regression.
COMMIT = none
REMAINING_RISK = Reviving the branch requires a durable fact and a reachable, authored route plus negative tests.
```

### DS-C1-02B-0013

```text
ID = DS-C1-02B-0013
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = FALSE_POSITIVE
REPRODUCTION = Independent trace shows authored facts load before narrative selection; current chapter replays display the natural-language storylet action.
ROOT_CAUSE = The earlier claim treated the fact-loading order as if narrative tick selection happened first.
FIX = No code change; preserve the corrected classification and current fact-order regression.
TEST = Current Alpha/Chapter narrative replays and authored fact/content checks.
RESULT = FALSE POSITIVE confirmed; no missing storylet action was observed.
COMMIT = none
REMAINING_RISK = Future storylet additions still need authored text and eligibility tests.
```

### DS-C1-02B-0014

```text
ID = DS-C1-02B-0014
ORIGINAL_SEVERITY = P2 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = STALE_AFTER_CHANGE
REPRODUCTION = The old Alpha-02 receipt cites 15/15 while its then-committed gate had nine cases; current code and current receipts now report the actual gate.
ROOT_CAUSE = Historical documentation outlived the replay script it described.
FIX = Historical text remains preserved; current remediation receipts distinguish the old claim from the current 19-case gate and no longer use the old assertion as current evidence.
TEST = Current recovery_replay_gate.ps1 output and this ledger.
RESULT = Stale historical claim reconciled; not a runtime defect at the current head.
COMMIT = a4ad377 plus 71d2aa0
REMAINING_RISK = Future receipts must be generated from the committed script output, not copied from an earlier report.
```

### DS-C1-02B-0015

```text
ID = DS-C1-02B-0015
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = The current lethal primary-guard path updates the authoritative fact when the badge owner dies.
ROOT_CAUSE = The fact was seeded but its runtime writer was absent in the audit snapshot.
FIX = Body synchronization sets fact_r1_guard_dead for the primary guard death, while non-lethal state remains distinct.
TEST = chapter01_security_bypass and aggressive Release replays report FACT_R1_GUARD_DEAD=YES; narrative route remains eligible.
RESULT = PASS.
COMMIT = 2efde27
REMAINING_RISK = The fact is scoped to the authored primary guard and is not a general death-to-fact mapper.
```

### DS-C1-02B-0016

```text
ID = DS-C1-02B-0016
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = ALREADY_FIXED_BY_PARALLEL_WORK
REPRODUCTION = Current objective presentation uses an explicit B1/quest prefix and the replay receipt separates presentation-during-route from active objective at completion.
ROOT_CAUSE = The earlier latch observed any non-empty fallback objective rather than the intended quest presentation.
FIX = Objective presentation is latched from the authored objective source while the quest is active; completion may clear the active objective without erasing the receipt.
TEST = Current B1 and Chapter replay receipts include QUEST_PRESENTED_DURING_ROUTE=YES; objective presentation unit/replay path.
RESULT = PASS; completed quest is not required to retain an active objective.
COMMIT = 2efde27
REMAINING_RISK = Only the current bounded Chapter One objective contract is claimed.
```

### DS-C1-02B-0017

```text
ID = DS-C1-02B-0017
ORIGINAL_SEVERITY = P2 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = CONFIRMED_CURRENT
REPRODUCTION = The baseline header target used header files as OBJECT sources, which MSVC could list without compiling their include contract.
ROOT_CAUSE = The advertised public-header check had no generated translation units.
FIX = CMake now generates one .cpp per public header, includes the header in that TU, builds a real writeover_public_header_check executable, and registers it with CTest.
TEST = Debug/Release configure/build, direct executable, Debug/Release CTest and CI CTest target.
RESULT = PASS: 2/2 CTest including public_header_standalone; direct executable exit=0.
COMMIT = 71d2aa0
REMAINING_RISK = The check is a compile self-containment gate, not a full ABI compatibility or API behavior suite.
```

### DS-C1-02B-0018

```text
ID = DS-C1-02B-0018
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = CONFIRMED_CURRENT
REPRODUCTION = The audit guard could write bad[20] when the buffer had only 17–20 bytes.
ROOT_CAUSE = The size guard and indexed byte disagreed.
FIX = The negative corruption test now checks bad.size() > 20 before writing the byte.
TEST = Release and Debug direct unit suites, including systemic.save_corruption_rejected.
RESULT = PASS: 208 tests, 0 failed.
COMMIT = 71d2aa0
REMAINING_RISK = The test still intentionally mutates a serialized fixture; no production bounds are weakened.
```

### DS-C1-02B-0019

```text
ID = DS-C1-02B-0019
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = CONFIRMED_CURRENT
REPRODUCTION = tests/test_harness.h used int64_t without a direct cstdint include.
ROOT_CAUSE = Transitive includes made the test header compiler-dependent.
FIX = Add the direct <cstdint> include.
TEST = Real generated public-header compilation and Debug/Release test builds.
RESULT = PASS.
COMMIT = 71d2aa0
REMAINING_RISK = Other future test-only types still need their own direct includes.
```

### DS-C1-02B-0020

```text
ID = DS-C1-02B-0020
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = CONFIRMED_CURRENT
REPRODUCTION = README fixed-count claims were stale relative to the direct test executable.
ROOT_CAUSE = Documentation encoded a historical test count.
FIX = Remove fixed test-count claims and describe the regression suite without a brittle number.
TEST = README diff plus current direct suite receipt.
RESULT = PASS: current Release direct unit reports 208 tests, 0 failed without a stale README claim.
COMMIT = 71d2aa0
REMAINING_RISK = Reports still record counts when they are evidence snapshots; those are deliberately time-stamped/current-head claims.
```

### DS-C1-02B-0021

```text
ID = DS-C1-02B-0021
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = CONFIRMED_CURRENT
REPRODUCTION = The player README advertised Right Mouse ADS while active ADS is disabled by design.
ROOT_CAUSE = Release controls text was not reconciled with the capability matrix.
FIX = Qualify ADS as reserved/disabled and document Esc pause plus Q quit while paused.
TEST = Release package includes the corrected README; settings/input tests and smoke remain green.
RESULT = PASS: no active ADS capability is advertised as available.
COMMIT = 71d2aa0
REMAINING_RISK = A future ADS implementation needs a separate feature contract and evidence.
```

### DS-C1-02B-0022

```text
ID = DS-C1-02B-0022
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = STALE_AFTER_CHANGE
REPRODUCTION = BASELINE_MANIFEST.json is not referenced by build, CI, packaging, or runtime and hashes an earlier frozen tree.
ROOT_CAUSE = Historical baseline material was retained as if it were a live manifest.
FIX = Do not regenerate it into a misleading current manifest; preserve it as historical evidence and keep current Git/content/package receipts authoritative.
TEST = Repository/CI reference search and current content/package checks show no active dependency.
RESULT = No current runtime or shipping failure.
COMMIT = none
REMAINING_RISK = If the file is later promoted to an authority, it needs an explicit owner and matching gate.
```

### DS-C1-02B-0023

```text
ID = DS-C1-02B-0023
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = CONFIRMED_CURRENT
REPRODUCTION = Engine::Run previously returned without invoking IEngineModule::Shutdown.
ROOT_CAUSE = The module lifecycle interface had a dead shutdown hook.
FIX = Engine calls each non-null registered module Shutdown exactly once after finite or RequestStop termination; InputRuntime guards against destructor double-shutdown.
TEST = engine.shutdowns_modules_once and engine.request_stop_terminates_run.
RESULT = PASS in Debug/Release direct suites and the normal quit production replay.
COMMIT = 71d2aa0
REMAINING_RISK = Null module registration is guarded at shutdown; broader module ownership remains unchanged.
```

### DS-C1-02B-0024

```text
ID = DS-C1-02B-0024
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = CONFIRMED_CURRENT
REPRODUCTION = Public systemic Save ignored bounded vector write failures and PerformSearch ignored event insertion failure.
ROOT_CAUSE = Serialization/operation APIs discarded failure returns after partial work.
FIX = SystemicWorld::Save now validates and returns bool while propagating all bounded writes; Serialize clears bytes on failure; PerformSearch rolls back its body/search mutation if AddSystemicEvent fails.
TEST = systemic.save_rejects_invalid_live_state, systemic.save_corruption_rejected, systemic runtime save/load tests, and Debug/Release direct suites.
RESULT = PASS: invalid NaN live state yields Save=false and empty Serialize; current systemic suite remains green.
COMMIT = 71d2aa0
REMAINING_RISK = The bounded wire contract remains finite; callers must honor Save=false and future larger vectors need deliberate limits.
```

### DS-C1-02B-0025

```text
ID = DS-C1-02B-0025
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = FUTURE_SCOPE
REPRODUCTION = AutonomousNpcSystem::FindNpcByEntity is private and has no caller.
ROOT_CAUSE = A retained helper is not part of the active runtime path.
FIX = No behavior or API was changed merely for dead-code removal; current navigation/combat paths remain intact.
TEST = Static audit COUNT=0 and current AI/runtime tests cover active paths.
RESULT = No player-facing defect; helper remains a bounded cleanup candidate.
COMMIT = none
REMAINING_RISK = Remove only with a focused dead-code change if repository policy later requires it.
```

### DS-C1-02B-0026

```text
ID = DS-C1-02B-0026
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = FUTURE_SCOPE
REPRODUCTION = staff_service_door is an authored scene record without a current interaction/transition consumer.
ROOT_CAUSE = Historical scene authoring contains a reserved door record.
FIX = Preserve the record as bounded future content; do not delete or wire a new route during audit remediation.
TEST = Current content check accepts the authored scene; active Chapter routes do not claim this door.
RESULT = No current route failure.
COMMIT = none
REMAINING_RISK = If retained for production content, validator/reachability ownership should eventually classify or wire it.
```

### DS-C1-02B-0027

```text
ID = DS-C1-02B-0027
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = FALSE_POSITIVE
REPRODUCTION = PredicateType::Count is intentionally the enum sentinel for the current authored boolean fact registry; FactStore already rejects it and a current test is named fact.predicate_sentinel_rejected.
ROOT_CAUSE = The audit treated a defensive/unreachable sentinel branch as a supported Count predicate.
FIX = No change; preserve sentinel rejection and avoid silently expanding the authored fact contract.
TEST = fact.predicate_sentinel_rejected, content/schema checks, Debug/Release suites.
RESULT = FALSE POSITIVE confirmed under the current contract.
COMMIT = none
REMAINING_RISK = If integer count facts become a product feature, the enum/schema/save contract must be redesigned together.
```

### DS-C1-02B-0028

```text
ID = DS-C1-02B-0028
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = FALSE_POSITIVE
REPRODUCTION = cppcheck's accessMoved warning concerns returning evt.id after moving the event; EventId is a trivially copyable strong ID and the value remains valid.
ROOT_CAUSE = Static analyzer heuristic does not model the strong-ID value semantics precisely.
FIX = No production change; current static audit remains clean.
TEST = Static audit COUNT=0 and event/AI unit coverage.
RESULT = FALSE POSITIVE confirmed.
COMMIT = none
REMAINING_RISK = A future non-trivial EventId would warrant revisiting the warning and storing the ID before move.
```

### DS-C1-02B-0029

```text
ID = DS-C1-02B-0029
ORIGINAL_SEVERITY = P3 / HIGH
CURRENT_HEAD = 71d2aa01efea5e6b7f08035bd66146956eb5bcd1
CURRENT_CLASSIFICATION = FALSE_POSITIVE
REPRODUCTION = The warning is emitted while analyzing the POSIX #else branch where events are not queued; the Windows branch has the active event path.
ROOT_CAUSE = Configuration/preprocessor-specific static analysis path, not a runtime invariant across platforms.
FIX = No change; do not weaken the POSIX input code or claim the warning as a gameplay defect.
TEST = Static audit COUNT=0; Windows Debug/Release and cross-platform CI compile gates remain authoritative.
RESULT = FALSE POSITIVE confirmed.
COMMIT = none
REMAINING_RISK = Platform-specific behavior still requires the corresponding platform build/CI checks.
```

## Classification totals

```text
CONFIRMED_CURRENT = 8
ALREADY_FIXED_BY_PARALLEL_WORK = 11
STALE_AFTER_CHANGE = 2
FUTURE_SCOPE = 4
FALSE_POSITIVE = 4
DUPLICATE = 0
DEFER_ASTRA = 0
UNCLASSIFIED = 0
```

The totals include the current remediation entries and intentionally do not
turn historical PVS01 or future social/content possibilities into current
Chapter One capabilities. There are no open Fatal or P0 findings in the 02B
ledger. Current normal-play P1 closure is separately gated by the fresh
replay, build, package and CI receipts in the Alpha-04 report.
