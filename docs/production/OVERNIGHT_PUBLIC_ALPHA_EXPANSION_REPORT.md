# Overnight Public Alpha Act II-A expansion report

Date: 2026-09-14  
Project: WRITEOVER-07  
Source baseline: `23eaddbe1a2476519fc91dd66d5dfefef069e512`
Status: `LOCAL_AUTHORED_EXPANSION_VERIFIED; RECORDS_ROUTE_GATE_PASS; HANDOFF_AND_PUBLIC_RELEASE_PENDING`

This report describes the overnight public-alpha-sized expansion after the
Chapter One elevator. It is not a product release, a visual-gold claim, a
course submission, or a public announcement.

## Scope decision

The expansion uses the current single-NPC AI architecture. It does not add a
squad/companion/BT/utility framework. The valuable increase is player-facing
choice: five rooms, distinct work identities, recoverable routes and facts
that carry into later spaces.

The design target is:

```text
Chapter One departure
  -> dispatch intrigue
  -> records / power / observation discovery
  -> quiet or noisy systemic consequence
  -> transit pressure
  -> checkpoint payoff and backtracking
```

## Room-by-room design

| Room | Objective | Broad solution styles | Optional branch | Systemic encounter | Social/technical interaction | Delayed consequence | Checkpoint |
|---|---|---|---|---|---|---|---|
| Service Concourse | Review the dispatch board and find a filed transfer | Read, inspect, choose a route, return to Elevator | Select Records, Power or Transit first | Entry fact gates the new storylets | Dispatch terminal and framed door controls | Chosen route changes available transitions | Existing Chapter One checkpoint remains valid; Act II entry is recoverable |
| Records Archive | Obtain or confirm the archive release | Speak with the operator, use credentials, backtrack | Credentialed terminal shortcut | Operator relationship and archive facts | Social approval or procedure-oriented terminal use | Released/consulted facts alter observation access | Return door to Concourse |
| Power Utility | Make the maintenance bypass carry current | Help the Technician, use relay, use backup, force reroute | Quiet assistance versus forced/noisy reroute | Infrastructure change and utility-noise event | Technician dialogue plus two technical controls | Noise raises Transit awareness and changes guard response | Return to Concourse; bypass to Transit when powered |
| Observation Gallery | Learn what the camera cannot see | Ask the analyst, inspect monitors, loop the camera | Create a blind spot before returning to Records | CameraBlindSpot knowledge and vandalism-like action | Analyst conversation plus camera terminal | Later Transit surveillance can be bypassed or interpreted differently | Return to Records |
| Transit Control | Resolve the checkpoint without losing the route | Talk, loop camera, bypass response guard, use control terminal | Quiet control, alert escalation or guard-down route | Guard state, alert level, access denial and checkpoint fact | Security response NPC, camera, monitor bank and control terminal | Checkpoint text and later route state reflect the chosen response | Checkpoint exit/control terminal; backtracking remains available |

The player is not forced into a moral score. “Quiet” means observation,
cooperation and discretion; “aggressive” means pressure, damage/noise and a
less negotiable record. Neither route erases consequences.

## Visual and experience work included

- Security, Full Human and Maintenance authored bodies are now built around
  proportions and mass before clothing marks.
- Four-direction art is authored separately; side views change overlap and
  equipment instead of mirroring the front.
- Pistol and Stunner are synchronized across idle/fire/reload states. Pistol
  reads as a compact service sidearm; Stunner reads as a broad dual-emitter
  tool.
- B1, Security and Elevator received large functional groupings. The door pass
  adds wall-fixed frames and panels with depth-safe layering.
- Opening composition uses a mild downward bias and a continuous service
  ceiling so the first frame favors route/equipment instead of an overhead
  grate artifact.
- HUD remains literal and compact; palette remains semantic and glyph-readable.
- The face pass removed the black eye band and exaggerated facial punctuation.
  It is a stylized authored face, not a photorealistic texture.

## Acceptance evidence

Current production renders:

- `out/creative_review_final_visual01/contact.png`
- `out/creative_review_face12_art/human_sheet.png`
- `out/act2_art_review_weapon07/weapon_sheet.png`

Runtime/content evidence:

- deterministic content compiler check passed;
- Debug and Release builds passed;
- Debug and Release tests passed;
- mandatory Chapter One/recovery replay passed 21/21;
- scenario matrix passed 36 total, 35 executed, one rule-invalid and zero
  valid-state coverage gaps;
- four final room smoke renders exited 0;
- dedicated Act II Records route gate passed: Dispatch -> Records terminal ->
  Records operator -> Concourse backtrack, with save/load and no-death checks;
- the first failed replay attempt is preserved and explained in the audit
  manifest, not counted as a pass.

## What is deliberately not claimed

- no foreground-terminal classmate playtest has been completed;
- only the Records route is covered by a dedicated Act II gate; focused
  Power/Observation/Transit fixtures and a first-time human read remain;
- package smoke and Release benchmark pass locally at the current documentation
  baseline; exact pushed-head CI passed for `a1feb5c160eb91ba0980f5d156ee3cac2550f698`
  in run `34790805997`;
- no exact pushed-head CI result exists at the time of this report;
- no public alpha announcement, release, Steam upload or course final is
  authorized;
- no claim of photorealistic faces or ordinary pixel framebuffer rendering is
  made.

## Remaining high-value polish

1. Play Act II once as a first-time player and record only concrete confusion.
2. Capture close Security/Full Human/Maintenance faces in the real foreground
   terminal; if the face is still uncanny, revise the authored silhouette once,
   not the renderer.
3. Add focused Power/Observation/Transit route fixtures; preserve the passing
   Records route gate and replay the affected set after systemic changes.
4. Run final package, benchmark and exact-head CI gates.

The expansion is useful when a classmate can describe the facility's work and
their consequences after playing, not merely when more rooms or tests exist.
