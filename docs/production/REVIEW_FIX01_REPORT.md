# WRITEOVER-07 REVIEW-FIX-01

## Scope

This checkpoint fixes only the B1 telemetry and Full-NPC speech transition
issues, records the existing Character Renderer near-projection measurement,
and clarifies the bounded NPC damage proof boundary. It does not add a map,
weapon, story, renderer, release, tag, branch, pull request, or Steam package.

## IMPLEMENTED

- Badge acquisition through body search no longer sets `access_attempted`.
  That flag is written only by the B1 gate/reader interaction path.
- Added a named Release replay counterfactual that acquires the badge and stops
  before any reader/gate interaction. Its expected state requires
  `BADGE_HELD_BY_PLAYER=YES` and `ACCESS_ATTEMPTED=NO`.
- Full-NPC speech now uses the existing `player_observed` field as the previous
  decision interval's visibility state. A speech event is emitted on sight
  entry, not on every 10-Hz decision tick; a real visibility gap permits a new
  sight-entry event.
- Added a regression test covering five seconds of continuous sight and a
  separate visibility-gap/re-entry counterfactual.
- Added a Release unit measurement at 0.5m/1m/2m/3m/4m using the existing
  `DrawCharacterSprites` path. No projection cap or art was changed.
- Documentation now labels guard line-of-sight `EventPlayerDamage` as a
  bounded health/death proof only, not complete enemy combat or combat AI.

## VERIFIED

- Release configure: PASS
- Release build: PASS
- Release direct unit executable: PASS, 174 tests, 0 failed
- Full-NPC speech transition regression: PASS
- Badge-only Release replay assertion: PASS; badge held, access attempted NO
- Character projection measurement: PASS; see
  `evidence/review_fix01/character_projection_measurements.md`

## PROJECTION RESULT

The current Near cap remains `96` cells. At 240x67 cells, the measured
projected heights are 96/96/52/35/26 at 0.5m/1m/2m/3m/4m respectively, with
LOD Near/Near/Near/Near/Mid. The 0.5m and 1m cases demonstrate a genuine
close-range scaling/clipping risk. The provisional review recommendation is a
future Near cap of 48 cells; Rain must decide after viewing real screenshots.
This task does not change the cap or redesign the Character Renderer.

## NOT VERIFIED / PENDING

- No claim is made that the bounded guard health path is complete enemy combat.
- Rain's visual decision about the existing Character Renderer and the
  provisional cap remains pending.
- This report does not certify Product Gold or public-release readiness.

CHARACTER_VISUAL_ACCEPTANCE = PENDING

FINAL_STATUS = READY_FOR_RAIN_VISUAL_DECISION
