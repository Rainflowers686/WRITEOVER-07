# WRITEOVER-07 player guide

For `0.2.0-candidate.1`, a playtest candidate rather than a stable 1.0 release.

## Download and start

Open [Releases](https://github.com/Rainflowers686/WRITEOVER-07/releases), select
this version and download a game package for your system from Assets.
GitHub's Source code archives are not ready-to-play builds. The Windows package
is `WRITEOVER-07-v0.2.0-candidate.1-win-x64.zip`. Extract the whole archive and
run `WRITEOVER-07.exe`. Keep its `data` directory beside it.

If the game closes immediately, open PowerShell in the extracted directory:

```powershell
.\WRITEOVER-07.exe
```

This keeps error text visible. Use a monospace terminal font with Unicode and,
for Chinese, CJK support. The interface needs at least 48×18 character cells;
80×30 or larger is easier to read. A window too small to play pauses the game
until enlarged. Terminal font behavior still needs human verification.

On first launch choose English or 简体中文, then New Game. Continue is available
when a recovery save exists. You can change language later in Settings without
starting over.

## Your first steps in B1

Read the current objective, approach equipment or staff, and aim at the object
you want to use. Follow the interaction prompt: F is the default. Right mouse
examines rather than aiming down sights. If there is no prompt, try moving closer,
changing your angle or checking whether a wall blocks the target.

F1 opens Case File: your objective, known leads, access status and records you
actually acquired. It does not reveal every ending condition ahead of time.
Pause to revisit Dialogue or Recent Events. Brief opening hints appear when
needed; they are not an automatic walkthrough.

Investigating records, speaking to people, using equipment and resorting to force
can leave different consequences. Cameras, shots and discovered bodies matter.
The stunner uses real ammunition; non-lethal does not mean undetectable.
When a damage source is known, feedback can name a rough direction, not an enemy
coordinate or a view through walls.

## Default controls and rebinding

| Action | Windows default |
|---|---|
| Move / look | WASD / mouse |
| Interact / examine | F / right mouse |
| Fire / reload | Left mouse / R |
| Pistol / SMG / stunner | 1 / 2 / 3 |
| Stunner shortcut | V, consumes ammunition |
| Sprint / jump | Shift / Space |
| Crouch / prone | Ctrl / Z |
| Lean left / right | Q / E |
| Case File | F1 |
| Manual save / load recovery | F5 / F9 |
| Pause / back | Esc |

Ordinary menus use movement keys for selection/scrolling and also retain arrows,
F to confirm and Esc to return as safe navigation. Follow each page's footer.
In Settings, choose Rebind Keys, select an action, press a new key, then confirm.
Conflicts are rejected. Esc cancels capture or confirmation. Arrow keys remain
reserved for menus. Restore Default Bindings changes bindings, not other settings.
Controls and gameplay prompts show your current keys.

## Saving, death and starting over

Manual, chapter checkpoint, pre-final choice, completion and latest recovery files
have distinct roles. Continue and F9 use the latest successfully written recovery
file. Pause offers Restart Checkpoint and Replay Final Choice. Replaying a choice
does not grant evidence you never found.

A primary slot and the recovery file are written separately. A partial failure
is reported as such, not as success for every slot. Invalid saves are rejected
without silently loading another run; a failed load should preserve the running
state. Back up important saves before upgrading.

New Game asks for confirmation, rebuilds live progression and keeps preferences.
It does not immediately delete all existing save files, though later saves replace
the corresponding slots. Death cannot be saved. With no recovery file, F9's room
entrance recovery retains durable world consequences; it is not a fresh world.

Windows uses `%LOCALAPPDATA%\WRITEOVER-07\`, with saves in `saves` and preferences
in `settings.cfg`. Linux uses `$XDG_DATA_HOME/WRITEOVER-07` or, when unset,
`~/.local/share/WRITEOVER-07`. macOS uses
`~/Library/Application Support/WRITEOVER-07`. User data is separate from the
installed game.

## Picture, sound and reading

Settings include language, subtitles, text duration, sensory detail, high contrast,
reduced shake/flicker, mouse sensitivity, field of view, inverted mouse Y,
interaction emphasis, master and separate audio volumes. Current audio is
procedural sound, not full voice acting. Device/platform support still applies;
volume controls do not make Linux/macOS audio equivalent to Windows.

Difficulty changes only damage received: Easy is approximately 75% of Normal,
Hard approximately 125%, with integer rounding. It does not change access,
evidence or endings. Frame Limit offers Auto, 30, 60 and 120. Auto follows at most
the existing 120 Hz simulation cadence; actual displayed FPS depends on your
machine and terminal.

## Scope and cautions

There are 19 playable rooms from B1 to the roof, with branches, return visits and
three endings. The directory's 41 floors describe the setting, not 41 playable
levels. Only platforms actually attached to the release are downloadable for
this version. Older PVS packages do not contain this campaign version.

Content includes firearms, lethal/non-lethal combat, unconscious/dead bodies,
body hiding and institutional suspense. Packages are unsigned. Check their source
and SHA-256 rather than disabling system protection. Foreground terminal behavior,
audio listening and first-time play still need human acceptance. No measured
completion time is promised.

## Report a problem

Use [Issues](https://github.com/Rainflowers686/WRITEOVER-07/issues) with the version,
system, terminal, room, reproduction steps and expected/actual outcome. Remove
personal information from screenshots and logs. Do not upload your whole user
data directory. For save issues, describe the symptom first and share only a
minimal sample if needed.
