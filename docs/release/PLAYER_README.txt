WRITEOVER-07
============
Version: @VERSION@
Platform: @PLATFORM@

You wake in B1 and work your way through a facility of locked access points,
staff and disputed records. Investigate before choosing how to proceed.
Cameras, gunfire and discovered bodies can affect later encounters.
The world, people and weapons are drawn with authored characters.

How to start
------------
@START@
The player entry is @ENTRY@. Game data is bundled with the entry and is
located relative to it; do not move the data directory by itself.
Choose New Game to begin in B1, or Continue to resume the latest recovery file.
This is a complete-campaign playtest candidate with 19 rooms and three endings.
It is not a finished 1.0 release.
The tower directory describes 41 levels; 19 rooms are playable. Unlisted stops
are not extra levels waiting to be unlocked.

Controls (release defaults)
---------------------------
WASD              Move
Shift             Sprint
Space             Jump
Ctrl              Crouch
Z                 Prone
Q / E             Lean left / right
Mouse             Look
Left mouse        Fire
Right mouse       Examine a visible nearby object/person (ADS remains disabled)
F                 Interact
R                 Reload
V                 Stunner shortcut (uses its actual ammunition)
1 / 2 / 3          Select pistol / SMG / stunner
Esc               Pause / back. Select Quit from the pause menu to leave.
F1                Case File: current objective, lead, evidence and access
F5                Manual save
F9                Load latest recovery file
Pause menu        Controls, settings, recent events, dialogue, ending summary
W / S in menus    Select; F confirms; A / D scrolls long text; Esc goes back
The Controls page shows your actual settings.cfg bindings, including remaps.
Sensitivity, volume, subtitles, contrast, reduced motion/flicker and sensory
detail can be changed in the existing Settings page. History stays available
when sensory messages are off. Terminal minimum for menus is 48 x 18 cells.
Frame Limit cycles Auto / 30 / 60 / 120 in Settings. Auto presents at up to the
existing 120 Hz simulation cadence; terminal/device speed can lower actual FPS.
Flashes and narrative effects keep their game-time duration at every frame limit.

Save and settings
-----------------
Save files: @SAVE_DIR@
Settings file (optional): @SETTINGS_DIR@
Saves and settings live in the platform user-data directory, separate from
the installed game. Back up saves you want to keep before changing versions.
Manual, chapter checkpoint, pre-final choice and completion saves are separate.
Continue/F9 loads the most recent successful recovery file. Pause > Restart
Checkpoint and Replay Final Choice choose their dedicated files. A corrupt
current file is rejected, not silently replaced with an unrelated older run.
Loading clears recent perceived history so future events cannot leak backward.
New Game confirms before rebuilding all live progression. Existing save files
remain until a subsequent save replaces that role; preferences are retained.
Death cannot be saved. If no recovery file exists, F9 returns the player to the
room entrance with durable world consequences retained, not a fresh world.

Known platform limitations
---------------------------
Windows uses the native raw-mouse and WinMM procedural audio paths when the
device is available. Linux and macOS use terminal keyboard input and retain
gameplay messages as text. They do not have the same mouse/audio experience
as Windows. A macOS arm64 package requires an arm64 system.
The September 2026 complete-campaign download is Windows x64 only; older
PVS-01 Linux/macOS downloads do not contain this campaign version.
Unsigned packages may prompt an operating-system warning. Check the source
and checksum; do not disable system protection to run the game.

Bug reports
-----------
Please report the platform, version, exact player entry, and the terminal
error text (if any) at:
https://github.com/Rainflowers686/WRITEOVER-07/issues
Include the room and steps to reproduce. Remove private information from
screenshots and paths; do not attach your entire user-data directory.

Credits and distribution notice
--------------------------------
See THIRD_PARTY_NOTICES.txt. This candidate has automated engineering evidence,
but foreground terminal appearance, audio listening and first-time human play
acceptance must be checked separately. No Product Gold status is claimed.
