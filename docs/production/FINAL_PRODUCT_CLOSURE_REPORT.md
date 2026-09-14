# Post-audit product closure — 0.2.0-candidate.1

This is the current closure record, superseding POST_ULTRA_FINAL_HANDOFF.md and
the older audit manifest for post-audit work. Historical evidence remains intact.
Local validation is complete. Publication is a separate gate: the release's
`RELEASE_VERIFICATION.json` will carry the resolved delivery SHA, exact-head CI run
and archive digest. Referencing that receipt avoids a self-referential commit/hash
in this tracked report. Until it exists and is verified, publication is unconfirmed.

## Delivery identity and acceptance

```text
START_HEAD = ec0a8a92765918a6e6fd0da952aa588b069c342b
FINAL_IMPLEMENTATION_HEAD = release tag's resolved commit (includes terminal unchanged-frame optimization); earlier runtime milestone d0c67db
FINAL_HEAD = resolved commit of refs/tags/v0.2.0-candidate.1; recorded in release RELEASE_VERIFICATION.json
VERSION = 0.2.0-candidate.1
LANGUAGES = zh-CN / en
CHINESE_RUNTIME_STATUS = IMPLEMENTED; automated catalogs and representative production renders checked; native human review pending
ENGLISH_RUNTIME_STATUS = IMPLEMENTED; automated startup and routes checked; human review pending
CJK_WIDTH_STATUS = IMPLEMENTED; display-column layout and wide-cell backend output; not general Unicode shaping
SAVE_FIXES = atomic replacement; complete Player payload validation; finite/domain checks; legacy complete-payload compatibility
RESIZE_STATUS = live capability refresh and minimum-size pause implemented; foreground acceptance pending
ONBOARDING_STATUS = bounded contextual first-run hints; Continue suppression; New Game reset
SETTINGS_STATUS = production preferences wired and inventoried; reserved legacy fields explicitly not advertised
KEY_REBINDING_STATUS = conflict checks, confirmation, persistence and menu escape path implemented
AUDIO_ACCESSIBILITY_STATUS = subtitles, bounded sensory/history and threat direction implemented; listening pending
TEST_FAST_REQUIRED = PASS; post_audit_fast_final_20260915/qa-result.json
TEST_EXTENDED = PASS across retained recovery/scenario receipts and corrected Act II/full-campaign reruns; original failed tier receipt preserved
TESTS_REMOVED_OR_MOVED = duplicate invocations removed; extended route matrix moved to explicit tier; assertions retained
FINAL_LOCAL_VALIDATION = Debug/Release builds, full FAST, EXTENDED constituent gates, bilingual and package preflight PASS
FINAL_CI_RUN = release RELEASE_VERIFICATION.json exact-head run; not yet established by this source document
FINAL_CI_RESULT = publication requires five jobs success; consult the actual run, not this gate definition
PACKAGE = WRITEOVER-07-v0.2.0-candidate.1-win-x64.zip
PACKAGE_SHA256 = release SHA256SUMS.txt and RELEASE_VERIFICATION.json; verify against GitHub asset digest
PRE_RELEASE_TAG = v0.2.0-candidate.1
PRE_RELEASE_URL = https://github.com/Rainflowers686/WRITEOVER-07/releases/tag/v0.2.0-candidate.1 (publication conditional on CI)
README_ZH = README.zh-CN.md
README_EN = README.en.md
PLAYER_GUIDE_ZH = docs/release/PLAYER_GUIDE.zh-CN.md
PLAYER_GUIDE_EN = docs/release/PLAYER_GUIDE.en.md
COURSE_REPORT = docs/course/COURSE_REPORT.md
WBS = docs/course/diagrams/wbs.mmd
USE_CASE = docs/course/diagrams/use_cases.mmd
UML = docs/course/diagrams/architecture.mmd
ALGORITHM_DIAGRAMS = docs/course/diagrams/raycast.mmd; save_transaction.mmd
TEST_REPORT = docs/course/TEST_REPORT.md
PPT = docs/course/output/WRITEOVER07_Course_Presentation_v2.pptx
DEMO_SCRIPT = docs/course/DEMO_5_MINUTES.md
OPEN_FATAL = none identified in scoped closure; not an exhaustive new audit
OPEN_P0 = none identified in scoped closure
OPEN_P1 = none known in scoped runtime; exact-head CI/publication remain separate delivery gates
HUMAN_ACCEPTANCE_ITEMS = native terminals, fonts/resize, audio, first-time play, visual acceptance, measured playtime, member identity and classroom rehearsal
SAFE_CLEANUP = NO_DELETION_PERFORMED; task evidence retained
```

## Finding closure

The statuses describe the addressed finding, not overall release readiness.

| Finding | Final status | Verified root cause and fix | Regression / evidence | Commit |
|---|---|---|---|---|
| F-01 | FIXED | Release workflow hardcoded an earlier product identity. One PRODUCT_VERSION now determines tag/archive/notes; publication is manual and refuses replacement. | Metadata tests and read-only dry run; actual publication still pending. | 73f826f |
| F-02 | FIXED | POSIX replacement unlinked the old destination before rename. Replace directly so failed replacement preserves the old file. | Failure-preserves-old-file test; native POSIX CI remains a final gate. | 5f49c29 |
| F-03 | FIXED | Optional Player tail accepted incomplete serialized state. Require complete supported layouts and reject truncated payloads before applying. | Player/save tests, legacy complete save, integrated staged faults. | b32fc38 |
| F-04 | FIXED | Cached terminal dimensions could remain stale. Refresh live capabilities and pause below playable dimensions. | Layout/capability tests; native resize is human-only acceptance, not inferred. | 2a0d9ab |
| F-05 | FIXED | Combat fields were not fully domain-validated at load. Reject invalid ammo, cooldown and non-finite values transactionally. | Player invalid-state regressions and load rollback gate. | b32fc38 |
| F-06 | FIXED | Runtime was English-first without a persistent locale or CJK column model. Add paired authored catalogs, locale preference and shared display-width handling. | Catalog coverage, bilingual startup and actual cell renders; not every human-visible sentence certified. | 602232f, 9475aac, 49cb58b, 17ce8f8 |
| F-07 | FIXED | Several preferences lacked production consumers or clear UI. Wire supported settings and rebinding; document reserved fields instead of advertising them. | Rebind replay, settings tests and SETTINGS_INVENTORY.md. | 2d05a6b, eed8f17 |
| F-08 | FIXED | Production frame-limit text accepted an unrestricted byte. Restrict preference values to Auto/30/60/120. Legacy raw Settings binary compatibility is not a new validated preferences format. | Config-domain tests and settings inventory. | 9475aac, 17ce8f8 |
| F-09 | FIXED | Historical manifest mixed local-only package status with later publication. Mark it historical and correct its chronology. | Read-back against recorded release provenance; this report owns current status. | 17ce8f8 |
| F-10 | FALSE_POSITIVE | The claim of no writer for fact_r1_checkpoint_reached is contradicted by the b1_to_calibration writer in composition_root.cpp. It remains legacy residue absent from the registry and without a meaningful later consumer. | Live writer/reader search; no gameplay change justified. | No code change |
| F-11 | INTENTIONAL_ACCEPTED | Role save and resume pointer are independently atomic, not one multi-file transaction. A failed second write can preserve a valid role save while resume stays older. | Explicit failure behavior and staged save regression; no promise of global multi-file atomicity. | b32fc38 |
| F-12 | USER_STATE_NO_ACTION | Pre-existing tests/test_harness.cpp newline-only change belongs to the user. | SHA256 D197B0DC8DDB26C60F09F18D6633E3071A0A70BF50240F27DBA1EF08F2E5EC31; not staged. | None |

## Product and implementation boundaries

The campaign remains 19 playable rooms with three endings. Forty-one floors are
diegetic context, not forty-one maps. Character art, weapons and Roof composition
remain frozen. No economy, inventory framework, multiplayer or new ending was added.

The sensory feed is bounded presentation, never a source of gameplay facts.
Acquired records read existing player-known knowledge assets; they do not create a
second journal database. Onboarding uses actual bindings and bounded context hints.
Language changes presentation, not ending eligibility. Chinese uses display columns,
not byte counts; complex script shaping and arbitrary emoji are not promised.

See [settings inventory](../engineering/SETTINGS_INVENTORY.md),
[contract review](../engineering/POST_AUDIT_CONTRACT_REVIEW.md) and
[teachback](COMPLETE_GAME_TEACHBACK.md) for actual files, owners, design tradeoffs,
and classroom explanations. The four public-header hash changes were reviewed;
the baseline was not indiscriminately regenerated.

## Evidence and performance

The first FAST receipt lives in `evidence/post_audit_fast_20260915/`: 245 unit
tests, CTest 2/2, content/schema negatives, bilingual catalogs, product/save,
rebind, 36 facing selections, package smoke and five package-negative probes
passed before the stale public-header hash gate failed. That FAIL is preserved.
After the reviewed correction, the contract checker passed and static audit
reported COUNT=0. The complete final FAST run also passed, in
`evidence/post_audit_fast_final_20260915/`. It began at 4936304 and ended at
d753174; intervening commits changed documentation and the separate Act II
assertion, not runtime code or FAST logic. The receipt records the ending HEAD.

EXTENDED's initial receipt is also preserved as FAIL: recovery 21 and the
scenario matrix passed (35 executed / 36 definitions; one invalid by game rules),
but Act II expected the obsolete non-directional threat sentence. Commit 436cc1b
requires the actual transit-room, from-ahead warning instead, preserving all route,
target-down, checkpoint, survival and access assertions. All three Act II routes
passed in `evidence/post_audit_act2_final_20260915/`. All four complete campaign
routes and four independent completed-save reloads passed in
`evidence/post_audit_campaign_final_20260915/`. The already-passing recovery and
scenario gates were not redundantly repeated after this assertion-only repair.

The first exact-head CI (34894714725, 59ff797) passed Linux GCC, Clang, macOS
ARM64 and Linux ARM64 link. Windows failed while printing the localization JSON:
redirected Python stdout used cp1252 and could not encode Chinese fragments.
The checker now emits ASCII-escaped JSON without changing coverage or assertions.
A forced-cp1252 local run verifies the receipt remains valid. That failed run and
its downloaded log are retained; publication requires the successor exact-head CI.

The successor 34895171162 exposed a macOS unchanged-frame timing failure:
TERMINAL_UNCHANGED_TIME_MS=0.369. The encoder was allocating a mask, expanding
wide-cell checks and copying a snapshot even for identical frames. An early
byte-equality check now skips that work, with a field-wise fallback when padding
differs. No threshold, sample count or scene was changed. Extended existing
render assertions cover separate allocations, padding-only differences, a real
color change and repeated unchanged output; render 59/59 passed. The first local
post-fix benchmark measured unchanged=0.010 ms with zero bytes and overall PASS.
Exact-head CI must verify the fix on macOS; host noise alone is not called a bug
in gameplay and a single timing comparison is not a general FPS claim.

That run measured PVS_RENDER_TIME_MS=0.711, PVS_TOTAL_FRAME_TIME_MS=0.984 and
SYSTEMIC_UPDATE_TIME_MS=0.128, with OVERALL_BUDGET=PASS. These are the benchmark's
worst_1pct_avg_ms statistics, not p99, terminal-write timing or display FPS.
The final FAST measured render 0.714, total frame 1.002 and systemic update 0.255
under those same named metrics, also OVERALL_BUDGET=PASS. Local values fluctuate;
this is not evidence of a statistically significant speedup or slowdown.
No benchmark threshold was relaxed. Localization menus/history are bounded;
no claim of a universal frame-rate improvement is made from this one run.

Ten final PPT slides were re-imported and individually rendered for inspection.
Text and diagrams remain editable. This is not a native PowerPoint rehearsal.
Mermaid diagrams are editable source; standalone image rendering is not claimed.
The course report is Markdown, not a fabricated completed institutional form.

## Remaining human acceptance

Use `docs/course/HUMAN_PLAYTEST_CHECKLIST.md`. Rain must check a foreground native
terminal in both languages, resizing, reading comfort, audio/subtitle balance,
quiet and combat routes, Roof/ending, restart and rebinding. A first-time classmate
must supply actual playtime and usability feedback. Members supply real names,
student IDs and contributions. No automated result supplies peer scores or visual
approval. No license grant was invented.

## Artifact preservation

Historical builds, candidate packages, settings, saves and failed evidence remain.
Task-created `out/course-presentation/`, `out/post-audit-package-preflight-01/` and
the untracked earlier course PPT are potential later cleanup candidates, not
automatically disposable while they support this audit. No deletion was performed.
