# WRITEOVER-07

[简体中文](README.zh-CN.md) · [Language gateway](README.md)

A single-player first-person exploration and combat game that runs in a terminal.
You wake in B1 and follow access permissions, staff accounts and disputed records
up through a facility. Follow procedure, find another way in, or use force.
Cameras, gunfire and discovered bodies can affect what happens later.

From a distance it reads as a low-resolution first-person space. Up close you can
see the characters that form a door frame, armor or a weapon. The people and guns
are authored character art, not ordinary pictures converted into colored pixels.

[Playtest downloads](https://github.com/Rainflowers686/WRITEOVER-07/releases) ·
[Player guide](docs/release/PLAYER_GUIDE.en.md) ·
[What's changed](docs/release/RELEASE_NOTES_v0.2.0-candidate.1.md)

![Elevator entrance](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/lift_front.png)

*Preview exported from the game's actual character cells. It is not a native
terminal screenshot or proof of foreground font/window acceptance.*

## Start playing

The current source candidate is `0.2.0-candidate.1`. Only packages actually
attached to its release are available for download.

1. Choose a game package in Assets, not Source code. Windows archives end in `win-x64.zip`.
2. Extract everything, keep the `data` directory, and run `WRITEOVER-07.exe`.
3. Choose English or Simplified Chinese, then New Game. Continue resumes an available recovery save.

Use a Unicode monospace font, with CJK support for Chinese. The interface needs
at least 48×18 character cells; 80×30 or larger is easier to read. A window too
small to play pauses the game. Packages are unsigned: verify their source and
checksum rather than disabling system protection.

## Ways through the facility

Examine equipment, read records and talk to staff before deciding how to cross
the next access point. The stunner offers a non-lethal option, but consumes
ammunition and may still leave a discoverable scene. Quiet and forced routes
can both leave consequences.

Case File collects your current objective, known leads, acquired records and
access status. Pause to review dialogue or recent events. These help you organize
what you know rather than reveal the ending answers.

The campaign contains 19 playable rooms from B1 to the roof, with branches,
return visits and three endings. The directory's 41 floors are part of the
setting, **not 41 playable levels**.

## Common controls

| Action | Windows default |
|---|---|
| Move / look | WASD / mouse |
| Interact / examine | F / right mouse |
| Fire / reload | Left mouse / R |
| Select weapon | 1 pistol, 2 SMG, 3 stunner |
| Case File | F1 |
| Save / load latest recovery | F5 / F9 |
| Pause / back | Esc |

Right mouse examines; it is not aim-down-sights. See Controls in-game for the
full list. Settings supports rebinding with conflict checks and confirmation.
Arrows, F and Esc retain safe menu navigation.

## Reading, recovery and comfort

Change language in Settings. Adjust field of view, mouse controls, subtitle
duration, sensory detail, contrast, shake/flicker, volumes and frame limit.
Difficulty affects damage received, not evidence or route conditions.
Auto follows at most the 120 Hz simulation cadence; it is not measured display FPS.

Manual, checkpoint and pre-final choice saves have separate roles. Pause lets
you return to a checkpoint or the final decision. New Game asks for confirmation
and does not immediately delete all old slots. Back up important saves before
upgrading. The [guide](docs/release/PLAYER_GUIDE.en.md) explains storage locations
and failure behavior.

## Before you play

Content includes firearms, lethal/non-lethal combat, unconscious/dead bodies,
body hiding and institutional suspense. Windows supports native mouse input and
procedural audio when a device is available. Linux/macOS do not offer the same
input/audio experience. Foreground terminal checks, audio listening, first-time
play and measured completion time still need human acceptance.

Report issues with the version, system, terminal, room and reproduction steps in
[Issues](https://github.com/Rainflowers686/WRITEOVER-07/issues). Remove personal
information from screenshots and errors; do not upload your whole user-data
directory. The repository has no open-source license: public visibility does
not grant unrestricted redistribution or commercial use.

For C++17 code, content tools and validation, see
[Development](docs/DEVELOPMENT.md) and [Version provenance](docs/release/VERSIONING.md).
You do not need to read engineering reports before playing.
