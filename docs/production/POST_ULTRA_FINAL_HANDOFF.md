# WRITEOVER-07 — Final product/director audit handoff

START_HEAD = 43083373a03b5583ae207001067cd866bdb9a2cc

FINAL_IMPLEMENTATION_HEAD = d2654f8974922a1e8db9d23e1ef99819cb5f5524

FINAL_HEAD = the delivery documentation commit containing this file. Resolve
`git rev-parse HEAD`; the exact pushed SHA and matching CI run are in the delivery
receipt. A commit cannot contain its own eventual object hash.

STATUS = SOURCE_CONTENT_FROZEN_FOR_AUDIT_PENDING_PUSH_AND_EXACT_HEAD_CI_RECEIPT

AUTHORITY = this report supersedes the previous director report's Roof gap and
its requirement to complete human acceptance before mechanical audit. Historical
reports and failed evidence remain preserved. Nothing here claims Product Gold.
The later broad optimization instruction is included, not substituted for the
original pass. POST_OPTIMIZATION_REVIEW.md records its measured comparison with
131556e, terminal/cadence fixes, cosmetic timing, menu usability and door polish.

## Product ownership and behavior

CAMPAIGN_INTACT = 19 rooms / 32 transitions / 8 upper lift destinations; no new
floors, regions, acts, NPC population, endings, campaign framework or save schema.

THREE_ENDINGS_VALID = Amend / Disclose / Breach, plus discovery-poor Breach;
full routes and independent completed-save reloads have passed. Final CI repeats
the complete campaign gate against its exact checkout.

PLAYER_PRODUCT_LAYER_STATUS = implemented as private application presentation.

SENSORY_FEED = 48 bounded entries, four categories, at most two live cues;
transition-based visual observations plus direct interaction, dialogue, damage,
access, objective, discovery, save and lift feedback. Dedupe window is 600 game
frames. Threat has live precedence. Long live cues show an ellipsis; history
holds the full bounded text. Messages never drive gameplay.

RECENT_HISTORY = paused, scrollable view of that same stream, not a second log.

DIALOGUE_HISTORY = filtered view of recently perceived speech; not a permanent
journal. Direct-message speaker classification is deliberately bounded to current
authored voices; audit new speaker naming before adding content.

INSPECT = existing reserved AimDownSights binding now explicitly means Examine
(default MouseRight; ADS remains disabled). Nearest focused same-room body/NPC/
prop is described without hidden inventory, internal states or ending facts.
Wall LOS and focus gates remain existing authorities. No cross-room omniscience.

BOOT_MENU = Continue, New Game, Controls, Settings, Quit; no-save default is New Game.

CONTINUE = most recent successful `pvs_resume`; legacy `pvs_manual` fallback only
when the resume file is absent. CRC/section checks determine menu availability;
full staged semantic validation still owns the load. Corruption is not silently
replaced with an unrelated save.

NEW_GAME = confirmation followed by shutdown/destruction and reconstruction of
the whole composition. B1/100 health/zero evidence, no old facts, NPC memory,
objectives, storylet state, inventory or cosmetic history. Preferences and existing
on-disk save files remain; later saves may replace their own roles.

PAUSE_MENU = Resume, Manual Save, Load Last, Restart Checkpoint, Replay Final
Choice, Case File, Recent Events, Dialogue, Controls, Settings, New Game, Ending
Summary, Quit. Dead players cannot Resume or save. Lost focus pauses explicitly.
Held menu actions are fenced until release; backend raw state is not mutated.
Options retain their order, selected items scroll into view at small sizes, stale
notices clear on page changes, and unavailable actions explain the restriction.

CONTROLS_HELP = actual existing binding table; introductory and interaction hints
are translated to those bindings. No new remapping framework.

ACCESSIBILITY = working sensory Off/Important/Detailed; Short/Normal/Long text;
subtitles, high contrast, reduced shake/flicker, sensitivity, master volume and
existing Auto/30/60/120 frame limit. Cosmetic timing uses the paused game clock.
All use existing settings.cfg; two bounded fields are authorized by ADR-0010.
World-save and legacy binary Settings layouts are unchanged.

CASE_FILE = authoritative objective/lead/discovery/access, wrapped and scrollable;
only player-known evidence is counted. Dedicated menu text no longer occupies an
indefinite subtitle override. Minimum supported panel is 48x18; smaller windows
show an explicit resize message rather than corrupting cells.

LOCATION_CONTEXT = authored room/destination names in Pause and Case File.

ACCESS_EVIDENCE = credential status, previously earned discoveries and current
route feedback, not a dump of global knowledge assets or secret eligibility.

SAVE_SEMANTICS = distinct manual, chapter checkpoint, pre-final and completion
files plus latest resume. Role saves and resume are individually atomic, not a
cross-file transaction; partial resume-update failure is reported honestly.
Checkpoint writes follow the three chapter-boundary transitions. Pre-final is
secured before choice; completion persists after Roof transition and ending facts.

DEATH_RECOVERY = load latest / dedicated checkpoint / New Game. If no save exists,
existing F9 entrance recovery retains durable world consequences and says so.
A corrupt file does not trigger that fallback. Saving death is rejected.

ENDING_SUMMARY = persisted chosen ending, known evidence, Network and Operations
consequences, and explicit recovery/New Game controls. Replay Final Choice loads
the separate pre-final state; a later newly committed ending can replace completion.

PERCEPTUAL_CONSISTENCY = successful load clears recent history, observer caches,
ending-summary cache, shot/hit/explosion pulses and subtitle/intrusion overrides before presenting
Loaded. Failed load retains live authority and reports failure. No future menu
text survives as an indefinite subtitle. World F9 stays non-modal so existing
movement replays do not lose a held movement segment.

## Creative freeze

ROOF = completed bounded outdoor-composition correction. Same RoomId, 64x64
authored envelope, high open-distance ceiling, continuous platform, 1.55m parapet,
four distant building groups, lower epilogue workstation and preserved return.
Old saved platform positions remain on valid floor. Production locomotion tests
contain repeated sprint/jump; nonfinite plane rays no longer invent a textured
ceiling at maximum distance. No skybox image or pixel framebuffer.

FULL_HUMAN = retained and production-reviewed previous shortened head, restrained
eyeline, coat/stance silhouette; no emoji face or extra face texture pass.

SECURITY = retained broad shoulder/vest/helmet/visor family and genuine side/back
poses; 36/36 production facing/LOD selections resolve exact authored assets.

PISTOL = retained authored oblique slide, enclosed grip/hand, connected forearm.

STUNNER = retained authored shorter twin-emitter silhouette, distinct negative
space and hand mass. Eight production held-pose renders reviewed; no new animation.

SIGNATURE_ROOMS = B1, Security, Elevator, Arrival, Records, Operations, Network,
Transfer, Executive, Authority, Roof production frames reviewed. Existing interior
zoning/geometry remains frozen. Wall texture is quieter, service-duct contrast is
protected, and steel doors have authored joined jamb/header frames checked from
multiple angles. Some distant people/equipment remain sparse
or abstract; this is not a photorealism or foreground-appearance acceptance claim.

NARRATIVE = existing records-supervisor voice and distinct working NPCs retained;
Dr. Vale opens B1, not an unrelated Security line. Roof has three distinct meanings.
History, duration controls and removal of sticky menu subtitles improve legibility
without adding a speech to every event. No new lore or famous-narrator imitation.

PACING = wake / service obstacle / systemic route / institutional escalation /
records and authority / outdoor release. Existing Arrival backtracking is bounded
route structure, not claimed as newly measured human playtime.

LONG_RANGE_CONSEQUENCES = existing body/Cleaner, noise/camera/security, cooperative
Operations, Network discovery and force traces still feed later access and endings.
Memorable beats remain the witnessed body response, later institutional recognition
of conduct, disputed record, and quiet open Roof after committing a resolution.

## Validation and delivery

LOCAL_REGRESSION = Debug/Release builds; 233/233 unit tests and CTest 2/2 in both;
13 content tests, 10 schema tests, deterministic content, invalid-seed startup,
21 recovery replays, 36 classified scenarios / 35 executed / one rule-invalid,
three Act II routes, four final full-campaign routes plus four independent ending
reloads, integrated product/New Game/eight rollback stages, 36 facing selections,
package positive/negative probes, static COUNT=0 and contract check: see the final
evidence/optimization_local_receipt_20260915.json for each current gate and result.
The final scenario matrix passed: 36 classified / 35 executed / one rule-invalid /
zero valid-state gaps. All listed local gates passed at d2654f8.
Final Release PVS render=0.751-0.944ms; total frame=0.985-1.223ms in two sequential
runs (worst_1pct_avg_ms); total avg_ms=0.662-0.682. Terminal full encoding
0.143-0.145ms, delta 0.046-0.048ms; platform writes excluded. No measured display
FPS claim. Full workload, 1200 samples and 6ms gate are unchanged.

PACKAGE = out/optimized-package-20260915/WRITEOVER-07-audit-candidate.zip;
609629 bytes, Windows x64, version 0.1.0-complete-campaign-candidate,
source d2654f8974922a1e8db9d23e1ef99819cb5f5524. Local candidate, no Release.

PACKAGE_SHA256 = 1901D0FE2889C90818B83935CE40797C5D5914115AC2CD00A3BB9A4891B27799

EXACT_HEAD_CI_RUN = resolve the post-push run whose headSha equals FINAL_HEAD;
the local evidence/optimization_delivery_receipt_20260915.json and final chat
record the exact run ID, jobs, conclusion and benchmark.

EXACT_HEAD_CI = NOT_YET_OBSERVED_AT_DOCUMENT_AUTHORING; do not inherit a green run.

COMPARISON_CI = 131556e/run 34866959456 succeeded in Windows, Linux GCC, Linux
Clang, macOS ARM64 and Linux ARM64 link without retry. Comparison macOS render
1.120ms; total 2.090ms. This is not the final optimization head's CI.

MACOS_BENCHMARK = predecessor 4308337/run 34849065030 failed only macOS Release
benchmark: character_total_runtime_frame_240x67 had worst_1pct_avg_ms=8.895,
max_ms=31.339; PVS_RENDER_TIME_MS=1.158. New per-slow-frame stage wall times and
std::clock diagnostics retain all 1200 samples, 25 NPCs, 240x67 workload and the
6ms total-frame gate. std::clock is process CPU on POSIX, elapsed time on MSVC.
Do not call worst_1pct_avg_ms p99. Host variance remains a hypothesis until evidence.
One justified unchanged-source failed-job rerun is the maximum authorized sample.

OPEN_FATAL = NONE_KNOWN_FROM_AVAILABLE_EVIDENCE

OPEN_P0 = NONE_KNOWN_FROM_AVAILABLE_EVIDENCE

OPEN_NORMAL_PLAYER_P1 = NONE_KNOWN_FROM_AVAILABLE_EVIDENCE; not an exhaustive audit.

FLASH_AUDIT_ITEMS = staged save rollback and partial two-file save failure;
pre-final versus completion ownership; transient presentation anti-leak; modal
held-action/focus/resize combinations; nearest-focus depth ties; bounded message
classification; Roof collision at all older save poses; platform benchmark stages.
Also audit explicit SGR state across rows/runs, frame-limit changes/stalls and
cosmetic-clock/reset ownership. New private helpers do not change public APIs.
Composition root remains a large integration file, not a new refactor assignment.

LUNA_FINAL_CLOSURE_ITEMS = fix audit-confirmed bounded defects, final packaging
metadata/README synchronization, course report/PPT/diagrams/demo assembly and
evidence formatting. Do not redesign art, renderer, menus, campaign, combat,
schema or thresholds. Do not add floors or new objective systems.

HUMAN_ACCEPTANCE_ITEMS = foreground Windows Terminal, Rain visual acceptance,
audio listening, first-time classmate playtest and measured playtime. These remain
open but do not block source/content freeze for mechanical audit.

OUT_OF_SCOPE = 41 physically playable floors, more acts/endings, pixel renderer,
generic UI/quest framework, broad architecture rewrite, public Release or Steam.

TASK_TEMP_CLEANUP = no broad cleanup. Prior untracked evidence/builds/settings
preserved. This pass's named evidence and package retained. Temporary package
smoke extractions are removed only by their existing scoped test helpers.

PRESERVATION_CAVEAT = the original unit executable was once run from repository
root and rewrote its pre-existing untracked settings_test.cfg/settings_ctx.cfg/
settings_legacy.cfg fixtures. Their prior bytes were not frozen; byte-for-byte
preservation is not claimed. Subsequent CTest runs use build working directories.
The user's tests/test_harness.cpp EOF change is excluded from every commit.

FOREGROUND_WINDOWS_TERMINAL = NOT_PERFORMED

RAIN_VISUAL_ACCEPTANCE = NOT_CLAIMED

AUDIO_HUMAN_ACCEPTANCE = NOT_CLAIMED

FIRST_TIME_CLASSMATE_PLAYTEST = NOT_PERFORMED

MEASURED_HUMAN_PLAYTIME = NOT_AVAILABLE

TEACHING_HANDOFF = COMPLETE_GAME_TEACHBACK.md, product and optimization addenda.
