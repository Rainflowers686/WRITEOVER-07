# PVS-01 Execution Contract

This document is the stable execution protocol for WRITEOVER-07. It is deliberately
separate from `PVS01_GOAL_LEDGER.md`: the contract changes rarely; the ledger records
current state, evidence, decisions and the next action.

## MISSION

Deliver a coherent, premium vertical slice that is ready for human playtest. Work
autonomously inside the approved repository and preserve existing work, history,
assets and configuration. Do not reset the game, rewrite unrelated systems, or turn
this protocol into a second project plan.

## GRAND OBJECTIVE

Complete the PVS-01 path:

`B1 Wake -> Calibration/Logistics -> Service/Medical -> 1F Lobby -> Security Checkpoint -> Restroom/Staff Lounge/Maintenance -> Elevator Lobby`

The slice must read as one intentional experience, not a collection of disconnected
features. The current phase, completed blocks, evidence and next action are always
read from `PVS01_GOAL_LEDGER.md`.

## QUALITY > QUANTITY

Prefer one finished, legible, tested interaction over many shallow additions. Every
new element must earn its place through player value, visual coherence, reliability
and performance. Do not count code or assets as progress when they do not improve the
ship gate.

## ACTIVE PRODUCT SOURCES

Before implementation, read `docs/product/ACTIVE_PRODUCT_SOURCES.md`, this contract,
and `docs/production/PVS01_GOAL_LEDGER.md`. Treat those files and the current tracked
repository as the authority. Generated output, old reports and chat history are
context only unless the ledger explicitly promotes their evidence.

## AUTONOMOUS DECISION AUTHORITY

Within the repository and the approved mission, choose implementation details,
test order, local tooling and reversible evidence collection without waiting for a
user response. Preserve dirty work and unknown assets. Do not make destructive,
credential-bearing, paid-service or scope-expanding decisions silently.

## NO USER INTERRUPTION

Continue through independent blocks without asking the user to approve routine work.
When a real blocker requires a secret, destructive data decision, external legal
choice, or an unavailable capability, record it precisely, protect the current state,
and continue other blocks. A blocker is not permission to stop the whole mission.

## PROBLEM SOLVING LADDER

1. Inspect the local source, runtime and failure evidence.
2. Reproduce with the smallest safe command or scene.
3. Try a minimal local fix and verify it.
4. Search the repository and official documentation.
5. Try a second materially different approach; use a third only when it is justified.
6. Run the relevant tests and red-team review.
7. If still blocked, document the exact cause, attempted alternatives, impact and
   smallest next action, then continue with an independent block.

## WEB RESEARCH RULE

Use web search/fetch for current technical facts only when local evidence is
insufficient. Prefer the official vendor/repository/docs source, capture a concise
finding in the ledger, and never paste a whole article, credential, private URL or
unbounded search result into the Goal context.

## CONTROLLED DEFER

A deferred item must name its owner, reason, evidence, risk, attempted methods,
required capability, estimated effort and exact revisit condition. “Later” alone is
not a valid deferment. Do not mark a deferred item completed.

## DEFERRED REVISIT

Before the final red teams, revisit every deferred item whose condition has become
true. Re-test after relevant code or asset changes. Keep a documented no-go when the
condition is still absent; do not repeatedly retry an unchanged failure.

## TIME / CONVERGENCE POLICY

Use bounded work sessions and converge each block toward a shippable state. Avoid
re-reading unchanged large files, dumping complete logs, or starting a new high-risk
branch near the time limit. Save a concise checkpoint to the ledger after each block,
including current phase, evidence, open issues and next action.

## VICTORY LADDER

- **GOLD:** complete, polished, stable, visually legible and ready for human playtest.
- **SILVER:** the intended path and interactions work with known, non-fatal polish gaps.
- **BRONZE:** a reproducible vertical slice exists and its remaining gaps are explicit.

Never call a block GOLD when its core interaction, failure handling or evidence is
missing.

## MAJOR BLOCKS

Use the ledger's numbered blocks as the active queue. The current mission spans:

1. B1 Wake and calibration foundation
2. Service/NPC interaction
3. Medical/service route
4. Logistics and transitions
5. 1F lobby
6. Security checkpoint
7. Restroom, staff lounge and maintenance routes
8. Elevator lobby
9. End-to-end integration and presentation
10. Final playtest readiness and release evidence

Do not silently reorder a dependency. If a block is not ready, record the dependency
and advance an independent block.

## THREE RED TEAMS PER BLOCK

Every meaningful block passes all three reviews before it is counted as shipped.

### RED TEAM 1: CODE REVIEW

Inspect correctness, ownership, lifecycle, error paths, data flow, null/empty cases,
performance hazards, tests and accidental scope. Confirm no debug bypass, hard-coded
secret, untracked dependency or unrelated refactor entered the block.

### RED TEAM 2: REAL PLAY / VISUAL EFFECT REVIEW

Run the smallest real playable or visual path available. Check input, camera,
feedback, timing, transitions, lighting, readability, audio/effects and recovery
from an ordinary player mistake. Static code inspection is not a substitute for this
review; save compact screenshots or measurements as evidence rather than pasting
large output into the Goal.

### RED TEAM 3: PERFECTION REVIEW

Ask what would make a demanding player reject the block: awkward pacing, visible
placeholder behavior, inconsistent scale, dead ends, noisy UI, weak affordance,
frame-time spikes or missing failure recovery. Fix the highest-value gaps, then rerun
the appropriate checks.

## LOCAL SHIP GATE

A block is locally shippable only when its acceptance behavior works, tests or
reproduction evidence are recorded, the three red teams have run, no Fatal/Major
issue remains open, and the ledger names the next action. “Build succeeds” alone is
not a ship gate.

## GOLDEN SAMPLE RULE

Keep a small deterministic sample for the path under review. Use it for before/after
comparison, regression checks and visual evidence. Do not replace a golden sample
with a larger noisy capture merely to make a metric look better.

## GOLDEN SCENES

The canonical scenes are the B1 wake/calibration sequence, the service/medical route,
the 1F security checkpoint, the restroom/staff/maintenance route and the elevator
lobby. Preserve their intended ordering and record any approved variation in the
ledger.

## BEFORE / AFTER PROOF

For material changes, record the pre-change symptom, exact change, test command or
play path, result, and post-change observation. Keep proof concise and reproducible;
do not store whole logs, private documents or secrets in the ledger.

## MOST LIKELY USER COMPLAINT

At each review ask: “What would a first-time player complain about immediately?”
Prioritize confusing navigation, unresponsive interaction, artificial pacing,
obvious placeholder presentation and unstable performance over cosmetic low-impact
cleanup.

## TOP 3 QUALITY GAPS

The ledger must keep the current top three gaps, each with impact and next action.
Replace an item only when evidence shows it is fixed or a higher-impact gap has
emerged.

## PERFORMANCE GATES

Measure the relevant frame-time, load-time, memory and interaction latency on the
smallest representative path. Investigate spikes and regressions; do not hide them
by lowering quality without recording the trade-off. A single average FPS number is
not sufficient for a stutter or hitch.

## CI RULE

Run the repository's applicable local CI/test commands before counting a block as
shipped. Keep CI output summarized. If CI cannot run because of an external or
documented environment blocker, record the exact command, error and substitute
evidence; do not claim CI PASS.

## COMMIT RULE

Commit only the explicitly completed production/docs work that is intended to be
preserved. Never add, reset, stash, clean or checkout unrelated dirty work. Use a
specific message, keep the working tree understandable, and report the commit in the
ledger. If a clean scoped commit cannot be made safely, leave the change uncommitted
and document it.

## CONTEXT HYGIENE AND DURABLE MEMORY

Do not print complete successful build/test logs. On failure, inspect the root error
and nearby lines first. Do not dump an entire repository, repeat unchanged large
files, or paste whole web pages into the conversation. Summarize web findings and
stage results in `PVS01_GOAL_LEDGER.md`; store screenshots and visual evidence as
files. After compaction, reread this contract and the ledger before continuing.

The ledger is the durable working memory for current phase, decisions, benchmarks,
red-team results, open issues, deferred work, commits, CI state and next action. The
contract must not be copied into it.

## FINAL THREE RED TEAMS

Before declaring the Grand Objective complete, run a final code review, a full
real-play/visual pass through the golden scenes, and a perfection review of the
whole slice. Confirm source status, tests, performance, evidence, deferred revisit
conditions and the user-visible complaint list.

## FINAL STOP RULE

Stop only when the Grand Objective reaches the declared Victory Ladder level with
evidence, or when a genuine global hard blocker prevents safe progress. A local
failure, ordinary missing dependency, web outage or one deferred polish item is not a
global stop. Never reset the game or erase history to manufacture completion.
