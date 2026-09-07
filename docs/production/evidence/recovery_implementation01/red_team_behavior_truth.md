# Recovery Implementation-01 — Red Team 2: Behavior Truth

This review evaluates action-to-consequence behavior rather than variable names
or process exit codes.  The runs below use the Release executable and the
bounded regression tests; they do not claim manual play.

## Counterfactual credential runs

### Run A — credential acquired

`recovery_b1_success.txt`:

- non-lethal hit created a runtime body;
- search revealed and took the badge;
- the player dragged and hid the body;
- the cleaner moved, arrived, inspected, and responded;
- the held valid badge opened the real B1 door;
- the terminal session and audit were created;
- the player crossed into `room_01_calibration`;
- save/load completed.

The output records `ACCESS_DENIED=NO`, `GATE_OPEN=YES`,
`GATE_CROSSED=YES`, and `TERMINAL_SESSION=YES`.

### Run B — credential not acquired

`recovery_b1_denied.txt`:

- the same B1 gate target was reached without acquiring the badge;
- the interaction was denied;
- the gate remained closed and the player remained in B1.

The output records `ACCESS_ATTEMPTED=YES`, `ACCESS_DENIED=YES`,
`GATE_OPEN=NO`, and `GATE_CROSSED=NO`.

### Terminal-only negative run

`recovery_b1_terminal_denied.txt` reaches the terminal without a badge and
records `TERMINAL_ATTEMPTED=YES`, `TERMINAL_DENIED=YES`, with no terminal
session and no gate crossing.

## Counterfactual discovery runs

- Success replay: after body hide, the cleaner's movement, arrival, inspection,
  discovery event, and response are all required by the replay assertion.
- `AutonomousCleanerMustArriveBeforeDiscovery`: before arrival the body remains
  hidden; after sufficient deterministic movement and inspection it is exposed
  and a response exists.
- `AutonomousCleanerBlockedCannotDiscoverOnDueFrame`: a solid wall leaves the
  body hidden through the due frame and produces no response.
- `AutonomousIncapacitatedCleanerCannotWitness`: a stunned cleaner cannot
  discover or report the body.

This is the intended behavior distinction: the same authored body fact does not
produce a consequence merely because a timer elapsed.

## Other negative behavior

- A dropped or transferred badge no longer satisfies the B1 reader seam.
- A revoked badge fails credential validation even if still held.
- A gate denial does not mutate the existing infrastructure door.
- A terminal denial does not create knowledge/session state.
- A continuous observation refreshes one memory; a separate event after the
  refresh window remains independently observable.
- A stunned or dead runtime actor is excluded from autonomous perception.
- Player death removes movement and interaction authority until the saved load
  action restores a live state.

## Red-team conclusion

The bounded B1 behavior chain is supported by state transitions and durable
event records, not subtitles alone.  No open P0/P1 behavior blocker remains in
the exercised slice.  Manual player comprehension and comfort remain a Rain
acceptance question.
