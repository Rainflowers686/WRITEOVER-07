# WRITEOVER-07 — Complete Game Manual Play Checklist

PURPOSE = human acceptance of the complete bounded campaign after the
automated route probes pass

STATUS = OPEN_UNTIL_RAIN_RUNS_FOREGROUND_WINDOWS_TERMINAL_AND_LISTENS_TO_AUDIO

This checklist is deliberately manual. Replay logs, SVG/PNG captures, unit
tests, and package smoke do not satisfy the human visual/audio/first-time-player
items below.

## Run setup

- [ ] Start the real Windows production binary in a foreground terminal.
- [ ] Record terminal host, dimensions, font, color capability, and whether
      the game is visible without a helper window covering it.
- [ ] Use a fresh user-data directory for a clean first-time run.
- [ ] Keep a second run available for save/load and alternate endings.
- [ ] Record the actual binary commit and package filename.

RUN_COMMIT =

TERMINAL_HOST =

TERMINAL_DIMENSIONS =

AUDIO_DEVICE =

## First ten minutes

- [ ] Wake in B1 and understand that the service reader is the first task.
- [ ] Read the opening subtitle; confirm no inappropriate early Security line
      appears before the player has earned the relevant context.
- [ ] Inspect the first room composition: service reader, major equipment,
      walls, door frame, and navigation path read as one place.
- [ ] Fire/handle the first weapon only if desired; verify the body/noise/
      security result is legible.
- [ ] Confirm the Pistol and Stunner have different silhouettes and are not
      merely two thin text lines.
- [ ] Enter Calibration, then Medical or Staff.
- [ ] Understand the first route choice without consulting source code.
- [ ] Reach 1F Security and the elevator without a repetitive terminal-door
      loop.
- [ ] Confirm the HUD shows one clear main objective, a readable interaction
      prompt, and restrained subtitle hierarchy.

FIRST_TIME_PACING = PASS / WARN / OPEN

FIRST_TIME_PLAYER_CONFUSION =

## Chapter One visual acceptance

- [ ] B1 has a focal point and functional equipment grouping.
- [ ] Calibration has a recognizable purpose.
- [ ] Medical reads as intake/triage, not an empty decorated box.
- [ ] Staff reads as a service route and is visibly optional.
- [ ] 1F Security reads as a checkpoint with desk/monitor/equipment logic.
- [ ] Elevator reads as a structural door/shaft/threshold, not a floating
      sprite.
- [ ] Security identity is readable at distance: helmet/visor/mask,
      shoulders/chest equipment, uniform, posture, and silhouette.
- [ ] Full Human/Medical face is restrained and non-uncanny at close range.
- [ ] Cleaner and Technician read as different roles from Security.
- [ ] Front, Back, SideLeft, and SideRight labels agree with actor yaw and
      the visible body orientation where those views are used.

CHAPTER_ONE_VISUAL = PASS / WARN / OPEN

FACING_LABEL_ISSUE =

## Act II-A route acceptance

- [ ] Reach the Service Concourse after the first elevator departure.
- [ ] The Concourse communicates hub/choice rather than a dead corridor.
- [ ] Visit Records Archive and verify its manifest interaction.
- [ ] Optionally visit Power Utility and Observation Gallery.
- [ ] Reach Transit Control and understand its procedural/security pressure.
- [ ] Save and load once before leaving Act II-A.
- [ ] Confirm the loaded route retains the expected door, badge, body,
      security, and objective state.
- [ ] In a quiet run, verify Transit does not immediately behave as if a loud
      action occurred.
- [ ] In an aggressive run, verify the shot/alert consequence is visible and
      durable.

ACT_TWO_A_MANUAL = PASS / WARN / OPEN

QUIET_ROUTE_RESULT =

AGGRESSIVE_ROUTE_RESULT =

## Upper-tower directory acceptance

- [ ] Enter Arrival Lobby and see a lift directory anchored in the room.
- [ ] Confirm the directory shows the current destination and only the
      currently unlocked destinations.
- [ ] Confirm W/S selection, F confirmation, and ESC/Pause close work in the
      foreground terminal.
- [ ] Confirm the Arrival door/lift has a frame, threshold, signage/window or
      equivalent depth relation.
- [ ] Visit Records Core; query the file; return to Arrival.
- [ ] Visit Operations Control; choose cooperation on one run and observe the
      quiet route state.
- [ ] Visit Network Node; discover the observation route; return.
- [ ] Visit Security Transfer; observe the guard/equipment/gate composition.
- [ ] Visit Executive Archive; open the sealed record.
- [ ] Visit Authority Core; verify the pre-final warning explains that the
      decision is durable.
- [ ] Backtrack to at least one earlier upper destination and confirm the
      directory remains coherent.

UPPER_TOWER_MANUAL = PASS / WARN / OPEN

ROOMS_REVISITED =

## Ending acceptance

Run the following from a fresh save or a verified pre-final checkpoint:

### Amend

- [ ] Reach Authority Core.
- [ ] Confirm AMEND is available.
- [ ] Choose AMEND.
- [ ] Confirm the roof epilogue identifies the decision.
- [ ] Quit/reload and verify ending_amend/campaign_completed/roof_reached
      remain durable.

AMEND_RESULT = PASS / WARN / OPEN

### Disclose

- [ ] Use a quiet/cooperative Operations route.
- [ ] Discover the Network observation route.
- [ ] Reach Authority Core.
- [ ] Confirm DISCLOSE is available.
- [ ] Choose DISCLOSE.
- [ ] Confirm the roof epilogue and durable ending fact.

DISCLOSE_RESULT = PASS / WARN / OPEN

### Breach

- [ ] Use the force/alert route or otherwise create the required transfer
      security trace.
- [ ] Reach Authority Core.
- [ ] Confirm BREACH is available.
- [ ] Choose BREACH.
- [ ] Confirm the roof epilogue and durable ending fact.

BREACH_RESULT = PASS / WARN / OPEN

## Audio and presentation acceptance

- [ ] Listen to the opening, first weapon consequence, security escalation,
      Act II-A transit, Authority Core, and roof epilogue on the intended
      output device.
- [ ] Confirm procedural Security voice, practical Cleaner voice, distracted
      Technician voice, and clinical-but-human Medical voice are distinct.
- [ ] Confirm the narrator has space and does not comment on every event.
- [ ] Confirm quiet passages remain quiet enough for tension.
- [ ] Confirm aggressive moments have readable escalation without rainbow
      terminal noise.
- [ ] Confirm glyph structure remains meaningful if viewed with color reduced
      or disabled.

AUDIO_LISTENING = PASS / WARN / OPEN

NARRATOR_VOICE = PASS / WARN / OPEN

## Final human gate

RAIN_VISUAL_ACCEPTANCE = PASS / WARN / OPEN

FOREGROUND_WINDOWS_TERMINAL = PASS / WARN / OPEN

FIRST_TIME_CLASSMATE_PLAYTEST = PASS / WARN / OPEN

MEASURED_HUMAN_PLAYTIME =

UNRESOLVED_HUMAN_FINDINGS =

No automated result should be copied into these fields without the human run.
