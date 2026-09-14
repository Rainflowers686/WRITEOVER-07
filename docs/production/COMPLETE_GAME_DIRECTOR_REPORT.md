# WRITEOVER-07 — Complete-game director pass

START_HEAD = a7c6d5bf8bafcf28cd24edf91ff3f1ef207bbf54

FINAL_IMPLEMENTATION_HEAD = 4153275550248ce28b42d95ce136959e9e2b1450

FINAL_HEAD = the documentation/evidence commit containing this report; exact
post-push SHA and matching CI run are reported in the delivery chat. A document
cannot embed its own eventual Git object hash.

STATUS = DIRECTOR_CANDIDATE_WITH_OPEN_VISUAL_AND_MANUAL_ACCEPTANCE

This report supersedes older handoffs only for the changes/evidence described
here. The old full SHA beginning `a509e3f5...` was wrong: the real historical
commit is `a509e3fa6b9edcb0ff1392d4ae7eaa2de2a913d6`. Historical CI numbers and
package hashes in earlier reports must not be relabeled as this source's results.

## Player-experience review

PLAYER_EXPERIENCE_REVIEW = all 19 room-start production captures, authored
character/weapon production fixtures, complete-route traces, menu capture and
independent saved-ending loads. This was not a zero-knowledge playthrough.

The existing complete campaign remains 19 rooms / 32 transitions / 8 upper
destinations / 3 endings, not a physically implemented 41-floor tower. No rooms,
major NPC population, combat model, public interface or save schema were added.

TOP_5_VISUAL_PROBLEMS_FOUND =

1. Full Human / Maintenance front heads occupied too much of the authored body;
   small punctuation eyes made near faces uncanny. Shortened heads, restrained
   eyelines and continuous skin planes; recovered leg/stance area.
2. First-person weapons read as disconnected rectangular diagrams. Reauthored
   oblique slide/emitter, grip, enclosed hand and connected forearm for 8 poses.
3. Arrival's near tall masses hid people and function. Changed to low counters,
   with a bench group and clear central walking lane.
4. Upper rooms shared near pillars and tall main terminals. Lowered selected
   terminals and furnishings; grouped Operations consoles and Records storage;
   differentiated Executive and Authority materials. Repetition is reduced,
   not eliminated.
5. Roof remained an enclosed equipment room. A 12m-wall revision made it worse
   and was rejected. Final low blocks/brighter concrete improve contrast, but
   an outdoor-feeling finale remains a declared visual gap.

TOP_5_GAMEPLAY_PROBLEMS_FOUND =

1. CONFIRMED: completed ending auto-save ran before the Roof switch. Three fresh
   processes loaded the completed flag in Authority, without the Roof end screen.
2. CONFIRMED: revisiting Authority could offer another ending and accumulate
   incompatible ending facts. Completion now closes eligibility and interaction.
3. CONFIRMED: Operations terminal and operator differed on B1 loud-action history
   and whether later Security was alerted. Both now use the same predicate.
4. PACING DEBT, not a new defect: repeated Arrival returns dominate upper routing.
   Kept the bounded graph; improved precise next-floor objectives.
5. REVIEW LIMIT: fresh-seeded direct Transit/Transfer starts receive immediate
   guard damage. They are not equivalent to an earned quiet arrival. Full quiet
   and aggressive routes, not that isolated screenshot, determine route correctness.

TOP_5_NARRATIVE_PROBLEMS_FOUND =

1. Concourse narration said two corridors despite its three-way authored route.
2. Records/Archive revelation was too abstract to explain why the player ascends.
3. All resolutions shared one general Roof line instead of distinct final meaning.
4. Completed case file still offered unresolved leads.
5. Damage feedback literally said health was authoritative — an engineering note
   in the player's fiction. Replaced with concise cover/line-of-sight guidance.

TOP_5_USABILITY_PROBLEMS_FOUND =

1. Lift directory was a long subtitle: selected stop and controls could disappear.
2. Ending menu and case file shared that clipping path, including subtitles-off.
3. Arrival objective did not identify the clerk prerequisite or next unlocked floor.
4. Roof load/quit/return hints were appended to an overlong epilogue; F9 was
   described as replaying a checkpoint although the slot could contain completion.
5. Post-completion Authority still invited a new choice. Prompt, eligibility and
   objective now agree that the decision is recorded.

TOP_3_HARD_ENGINEERING_PROBLEMS_FOUND =

1. Synchronous RequestSave ordering across world transition and fact ownership.
2. Gameplay controls incorrectly depended on a short optional subtitle surface.
3. Two alternative interactions wrote inconsistent long-range route consequences.

FIXED_VISUAL = human/maintenance front proportions, 8 weapon poses, 6 room
compositions, selected terminal heights and Operations monitor placement.

FIXED_GAMEPLAY = ending persistence order and no repeated decision; Operations
route parity; clearer navigation. No tactical AI redesign.

FIXED_NARRATIVE = concrete Subject 07 order/reviewer chain, distinct final meanings,
resolved case file, Concourse topology wording and diegetic damage feedback.

FIXED_USABILITY = private paused campaign panel with wrapping, selected-item-first
layout and reserved footer; gameplay controls visible even with subtitles disabled.

FIXED_ENGINEERING = bounded private app policy/presentation changes only. The
C++ standards skill guided minimal ownership/API changes; frozen public headers
and the existing exception/dependency rules take precedence over generic advice.

## Creative disposition

B1 = protected room layout; updated shared human and weapon assets. Body/camera
discovery remains the first systemic lesson.

SECURITY = protected desk/monitor/gate composition and heavy armour identity;
real moving/multi-angle door acceptance remains open.

ELEVATOR = retained framed departure; Chapter One is a departure, not campaign end.

ARRIVAL = low counters reveal reception; directory shows actual destinations.

RECORDS = low storage bank; release denial precedes revival, review owner is 36F.

OPERATIONS = consoles and monitor grouping; consistent quiet/force history.

SECURITY_TRANSFER = retained consequential guard encounter; lowered main terminal.

EXECUTIVE_ARCHIVE = warmer perimeter and low furnishings; order gives Subject 07
review authority, which explains rather than merely announces the final choice.

AUTHORITY = cleared near masses; selected outcome meaning appears before other
options; completed decision cannot be replaced through normal interaction.

ROOF = ending acknowledgements and controls improved; open-sky visual illusion
not achieved. The current solid-cell ray model uses the full adjacent vertical
span and treats out-of-grid as solid. Raising ceilings was not an acceptable fix.
No new renderer or fake sky framebuffer was introduced to hide this limitation.

FULL_HUMAN = shortened front head/face; coat and stance preserved; four-facing
production fixtures inspected. Side/back silhouettes not falsely claimed rewritten.

SECURITY_ART = existing armour/visor hierarchy retained after fixture review.

PISTOL = oblique slide, trigger-area suggestion, enclosing hand and connected
forearm; final production bounds x=170..196, y=40..65 at 240x67.

STUNNER = paired contacts / broad emitter / battery body, distinct from Pistol;
same hand-authored glyph approach, not an image conversion.

HUD = health/ammo/objective/prompt hierarchy protected. Campaign controls now
have their own surface; quiet epilogue has separate title/identity/text/controls.

DIRECTORY = 80x24 live production dump inspected; 48/80/120/240 unit widths.
At minimum size the selected item and footer are protected, not all informational
rows. A full scrollable journal/list system was deliberately not introduced.

CASE_FILE = one current lead plus evidence/route traces; completed outcome replaces
leads. It is not a new quest or persistent journal subsystem.

NARRATOR = records-supervisor voice preserved; text bank read in full. Existing
busy-presentation gating can suppress a first-look line when the player acts
quickly; critical new revelations also live in direct interaction responses.

NPC_DIALOGUE = short, role-specific Records/Operator/Liaison/Authority revisions.

PACING = clearer next actions and shorter ending presentation; repeated hub travel
remains. No human first-ten-minute or total playtime measurement is claimed.

LONG_RANGE_CONSEQUENCES = B1 loud history now agrees across Operations interactions;
quiet cooperation plus Network permits Disclose; force permits discovery-poor
Breach. Existing Cleaner/body/camera consumers retained.

AMEND = own account becomes official; facility keeps custody of original.

DISCLOSE = record and surviving feed leave the facility's exclusive control.

BREACH = refuse certification and leave through forced access; file stays open.

AUDIO = existing procedural backend initializes; no listening or mix acceptance.

SAVE_LOAD = original bug reproduced by loading all three complete-route saves;
fixed by switching to Roof before setting/saving completion. New second-process
reload assertion requires exactly one ending, live player, Roof fact and end screen.
Old saves are not rewritten/migrated; a historical completed Authority save can
still be taken to Roof through the already unlocked lift.

## Verification ledger

LOCAL_REGRESSION = PASS for the checks listed below. Initial modified-art run had 3 failures, then 1, then
219/219; thresholds were unchanged. A Release relink was blocked by this task's
running executable; that failed relink is not a successful source build.

Final source was successfully rebuilt after replay processes exited. Debug and
Release direct tests: 219/219 each; CTest: 2/2 each; content deterministic check,
13/13 content tests, 10/10 systemic schema tests, invalid-seed rejection,
dependency/forbidden/public-header checks and static audit COUNT=0 passed.
Recovery: 21/21; scenario matrix: 35 executed / 36 classified (one invalid by
game rules); Act II: 3/3. These wider routes preceded the ending-only save-order
change. Final source then passed all four full campaign routes and all four
separate-process completed-save reloads. No claim of repeating unrelated broad
matrices after the ending-only change.

SAVE_FAULT_MATRIX = 8/8 injected final-commit stages restored byte-equivalent
live sections; final probe uses a completed Roof save. Deliberate fault probes
are judged by SAVE_FINAL_COMMIT_FAILURE_STAGE and SAVE_FINAL_COMMIT_ROLLBACK,
not by an unrelated full-route replay predicate.

BENCHMARK = OVERALL_BUDGET=PASS; PVS_RENDER_TIME_MS=0.766;
PVS_TOTAL_FRAME_TIME_MS=1.471; TERMINAL_FULL_TIME_MS=0.264;
TERMINAL_DELTA_TIME_MS=0.065; TERMINAL_UNCHANGED_TIME_MS=0.035;
TERMINAL_WORSTCASE_TIME_MS=0.257. Art is AUTHORED, 240x67; original budgets and
sample counts retained. worst_1pct_avg_ms is not p99; total excludes platform writes.

PACKAGE = out/director-package-20260914/WRITEOVER-07-director-candidate.zip;
source 4153275550248ce28b42d95ce136959e9e2b1450; 570880 bytes;
SHA256 84AB89FCA28D84378971367AC0A40121685D30FCE6D99E82264F92281482EA18.
Positive smoke/resource root/user-data separation passed. Missing content,
character art and text negative probes passed; package secret scan passed,
developer garbage count 0. This is a local candidate, not a published Release.

SELECTED_VISUAL_EVIDENCE = evidence/director_visual_20260914 (20 PNG files:
9 representative rooms, 3 completed endings reloaded, 4 production art sheets,
1 directory and 3 before frames). SVG-to-PNG conversion displays exported
production CharCells, not a native Terminal screenshot. Art sheets call production
rendering on fixture scenes, not normal campaign play. Initial captures covered
all rooms; the selected retained set is intentionally bounded.

EVIDENCE = evidence/director_campaign_verified_20260914;
evidence/director_recovery_20260914; evidence/director_scenarios_20260914;
evidence/director_act2_final_20260914; evidence/director_savefault_20260914.
The misleadingly named director_campaign_final_20260914 is the preserved stale
binary negative run, not the final PASS. director_act2_20260914 failed only its
old damage-text assertion; director_act2_final_20260914 is the corrected result.

REMOTE_CI = pending exact delivery-head check; no inherited green claim.

MACOS_HOSTED_BENCHMARK = starting head run 34840353471 failed macOS benchmark
while Windows/Linux/GCC/Clang/ARM-link jobs passed. Timing variance is not proven
to be infrastructure solely because another attempt passed. No threshold changes.

OPEN_FATAL = none established by this bounded review; not an exhaustive audit.

OPEN_P0 = none established; corrected completion-save regression passed.

OPEN_PLAYER_P1 = visual/first-time-play boundaries remain; do not infer absence
from automated completion alone.

REMAINING_ART_DEBT = Roof enclosure; repeated upper-room terminal dressing;
near door layers/oblique views; subjective faces and weapon foreshortening.

REMAINING_NARRATIVE_DEBT = real pacing/listening acceptance; text review does not
prove the emotional impact of an uninterrupted run.

REMAINING_GAMEPLAY_DEBT = first-time navigation and encounter fairness review;
known-route automation cannot establish either for a new player.

FOREGROUND_WINDOWS_TERMINAL = NOT_PERFORMED; native UI controls unavailable here.

FIRST_TIME_CLASSMATE_PLAYTEST = NOT_PERFORMED.

RAIN_VISUAL_ACCEPTANCE = NOT_CLAIMED.

## Presenter teaching anchors

| Important file / class / function | Problem, design and classroom explanation |
|---|---|
| src/app/composition_root.cpp / PlayerModule::RequestSave and ending input callback | Save is synchronous. First enter the final room, then write completion and serialize. Explain that a consistent snapshot must include both story facts and player location. |
| scripts/complete_game_campaign_gate.ps1 / completed reload subprocess | A live ending screenshot cannot prove persistence. Restart and verify the saved outcome independently. |
| src/app/campaign_panel.h / DrawCampaignPanel | Controls cannot be optional dialogue. Private CharCell panel wraps content and reserves the footer without altering world rendering or input ownership. |
| src/app/tower_campaign_runtime.cpp / Objective, DirectoryRows, CaseFile, EligibleEndings | Read existing facts to derive presentation; no duplicate quest database or save schema. Completion is a terminal decision state, not another choice menu. |
| src/app/composition_root.cpp / Operations interaction callback | Alternative interactions share a policy predicate; otherwise one door into the same system erases a consequence. |
| data/characters/b1_character_art.txt / CharacterArtBank assets | Authored glyph silhouette, occupied blanks, separate LOD/facing. Perspective and anatomy come before decorative texture. |
| data/rooms/*.json and data/scenes/recovery_scene.json / existing contentc pipeline | Lower physical masses and compose equipment to expose focal actors. Authored JSON compiles deterministically; no renderer rewrite needed for these sightlines. |
| data/text/recovery_text.txt / existing NarrativeModule | Stable text IDs separate prose from storylet conditions. Records knowledge is not omniscience; important route information belongs in direct interaction too. |

## Preservation

TASK_SCRATCH_CLEANUP = BLOCKED_BY_TOOL_POLICY. The exact task-owned
out/director-review-20260914 directory was verified (141 files, 94028641 bytes)
and 20 selected PNGs plus relevant logs were retained separately. The delete
command was rejected before execution; nothing was deleted and no alternative
cleanup mechanism was attempted. Candidate package and earlier evidence remain.

Pre-existing tests/test_harness.cpp newline change remains unstaged. Existing
settings, build-campaign trees, historical visual captures, user untracked files,
and commit 5434f8a are not cleaned, restored, rebased or rewritten. No branch,
PR, tag, release, course deck, generic expansion or exhaustive audit was created.

This candidate is not PRODUCT_GOLD, VISUAL_GOLD, PUBLIC_RELEASE_READY or a claim
of Rain's acceptance. Review the explicit open items before declaring audit readiness.
