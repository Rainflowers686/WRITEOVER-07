# Recovery Implementation-01 — Red Team 3: Player Experience

## Actual runtime observation

The Windows Release executable was started through the default player entry in
a maximized Windows Terminal.  The captured startup line selected:

- backend: `ansi-truecolor`
- dimensions: `240x67`
- preset: `ULTRA120`
- color capability: `TRUECOLOR`
- probe: `windows-terminal`

The real capture shows the Character-Art world path, authored NPC character
strokes, pistol viewmodel, and the player HUD.  The developer preset/grid
information is not present in the normal HUD.  The earlier pre-fix capture
showed raw CSI and UTF-8 corruption; the current capture does not.

Evidence files:

- `real_windows_terminal_c01.png` — real Windows Terminal desktop screenshot.
- `real_windows_terminal_c01.mp4` — real 8-second continuous desktop capture.
- `replay_b1_success.txt` — Release deterministic success route.

## Player-facing questions

| Question | Current evidence | Status |
| --- | --- | --- |
| Is the first-person space readable? | Real Release screenshot/video | OBSERVED; Rain acceptance pending |
| Are NPCs character strokes rather than rectangles? | Real capture and committed Character-Art path | OBSERVED; Rain acceptance pending |
| Is the pistol recognizable? | Real capture plus runtime fire feedback | OBSERVED; Rain acceptance pending |
| Is the normal HUD free of developer grid/preset leakage? | Real capture | VERIFIED |
| Can the player understand and complete the B1 loop? | Deterministic replay and state assertions | AUTOMATED EVIDENCE; manual play pending |
| Is the full 60–90 second experience comfortable? | No manual session was performed | NOT VERIFIED |

## Red-team conclusion

The runtime integration issue is corrected and real evidence exists.  This
record deliberately does not self-certify visual quality, Product Gold, or
public-release readiness.  Rain must open the screenshot/video and run the
provided entry/replay commands before visual acceptance.
