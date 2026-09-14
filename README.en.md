# WRITEOVER-07

[简体中文](README.zh-CN.md) · [Download the playtest](https://github.com/Rainflowers686/WRITEOVER-07/releases/tag/v0.2.0-candidate.1)

A single-player game that puts MUD-style exploration inside a real-time
first-person world. You wake in the facility's B1 with a number, a weapon and a
record that seems less certain about who you are. Investigate, talk to staff,
find a way past access controls, or open fire. Your approach can change how
people respond and what gets you through the next checkpoint.

![Character-built elevator entrance and first-person weapon](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/lift_front.png)

*Preview exported from the game's actual character cells. Fonts, text size and
window dimensions affect how it looks in a native terminal.*

## Awake is not the same as cleared to leave

Medical staff check whether you can stand. The reader checks your credential.
Security wants an answer that fits the procedure. The facility is less helpful
about explaining why you are here.

Starting in B1's revival area, find your way through Calibration, Medical and
Security to reach the floors above. In Records and Dispatch, the problem becomes
less straightforward. A file may open the next door without explaining what
happened. Some staff will help; others would rather finish their shift. The
narrator keeps an account of your progress in language of its own.

You need to understand your situation and decide how to leave. Read the records
you find, compare what people tell you, and notice why a route has become
available. The campaign reaches the roof. What you can choose at the end depends
on the evidence you acquired and the consequences of getting there.

## MUD-style exploration, first-person combat

The view and the writing do different jobs. Look down a corridor to locate a
door, read a guard's facing, or find cover during a fight. Examine objects, talk
to people and read records to understand how this place works and why someone
is refusing to let you through.

Before opening fire, try checking the equipment, reading the notices or
approaching the staff. Case File collects
your current objective, known leads, acquired records and access status. Dialogue
and recent-event history let you return to a line you missed.

The MUD influence is in observing a place, interacting with its people and
working out what to try from their responses. Movement and combat use real-time
keyboard and mouse controls; you do not need to memorize a text-command language.
This is a single-player offline game, not a multiplayer MUD.

## People with shifts to finish

Security patrols, checks disturbances, investigates noise and enters combat when
a threat escalates. A camera report or a discovered body can turn a routine
passage into an inspection. Breaking sight matters. Removing the guard does not
automatically persuade every reader to open.

Pay attention to the cleaner. How you treated him, and what you left in the
corridor, can affect whether he handles the scene, calls Medical or reports it
to Security. Leaving someone unconscious gives the next person who finds them
a different situation from leaving them dead.

Technicians talk about readers, wiring and bypasses. Medical staff look at the
person before dealing with the number. Expect clipped orders, practical advice
and people who would prefer not to explain themselves. They respond through
their jobs, remembered events and limited information rather than all acting
as helpful guides.

NPC dialogue is authored and their behavior follows in-game rules. These are
not language-model chatbots. Their human quality comes from how they respond to
what you did and then get on with their work.

## The narrator has a way of filing things

> You are cleared to leave the bed. The room is a separate authorization.

It knows the facility's vocabulary. You receive clearance, follow procedure and
depart on record. The delivery is calm, sometimes almost helpful.

It also notices deviations. Switch off a camera and it can tell you:

> The camera has stopped supplying answers. That is an answer of its own.

It does not narrate every movement. Quiet progress, noise and changes to a scene
receive different responses when the circumstances call for them. Take the words
as advice, or compare them with what you just saw. An official-sounding account
is not necessarily a complete explanation.

Narration and dialogue are presented as text. The current version does not have
full character voice acting.

## Getting through is only part of the problem

Credentials, staff assistance, terminals and maintenance bypasses can provide
ways through particular checkpoints. Looking at the route before acting often
leaves you more room to maneuver. You can also use force and carry on dealing
with what follows.

The pistol, SMG and stunner give you different combat options. The stunner can
leave someone alive, but it still uses ammunition and can leave a discoverable
scene. After stopping an immediate threat, consider the body, the cameras and
what the next checkpoint may hear about it.

Some consequences travel beyond the room where they started. B1 camera records,
the cleaner's response and a filed route can matter to staff farther up the
facility. Returning to places and rereading records helps connect those events.
The three endings are not all available on a menu from the start; evidence and
earlier actions affect which choices you can make.

## Look closer: the whole place is made of characters

Door frames, consoles, armor, a face in profile and the gun in your hand are
built from characters. People and weapons use authored character assets, with
color separating materials and light. Characters have front, back and side
silhouettes for changes in facing.

The aim is a first-person space you can read: where a doorway sits, how much
space equipment occupies, and who is blocking your route. The glyphs remain
visible within it. Ordinary images are not converted into a screen of colored
pixel blocks.

The current campaign has **19 playable rooms from B1 to the roof, branches and
return visits, and three endings**. The directory's 41 floors belong to the
setting; they are not 41 playable maps.

---

## Download and start

The current playtest is **0.2.0-candidate.1 (Pre-release)**. A Windows x64 package
is available. This is not stable 1.0.

1. Open [this release](https://github.com/Rainflowers686/WRITEOVER-07/releases/tag/v0.2.0-candidate.1)
   and download `WRITEOVER-07-v0.2.0-candidate.1-win-x64.zip` from Assets, not Source code.
2. Extract everything, keep `data` alongside the program, and run `WRITEOVER-07.exe`.
3. Choose English or Simplified Chinese, then New Game. Continue resumes an available recovery save.

Use a Unicode monospace font, with CJK support for Chinese. The interface needs
at least 48×18 character cells; 80×30 or larger is easier to read. A window too
small to play pauses the game. Enlarge it to continue.

The executable is unsigned. Check the release source and attached
`SHA256SUMS.txt` rather than disabling security software.
The [full English guide](docs/release/PLAYER_GUIDE.en.md) covers save locations
and troubleshooting.

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
remaining bindings. Settings supports rebinding with conflict checks and
confirmation. Arrows, F and Esc retain safe menu navigation.

On your first run, follow the opening prompts to inspect the nearby reader and
approach the staff. If access is denied, read the reason and check your objective
in Case File. A refusal does not have to become a fight.

## Language, saves and comfort

Switch languages in Settings. You can also adjust field of view, mouse controls,
subtitle duration, sensory detail, contrast, shake/flicker, volumes and frame
limit. Difficulty affects damage received, not evidence or access routes.
Auto follows at most the 120 Hz simulation cadence; that is not a promise of
measured display FPS.

Manual, checkpoint and pre-final choice saves have separate roles. Pause lets
you return to a checkpoint or the final decision. New Game requires confirmation
and does not immediately delete every old slot. Back up important saves before
changing versions.

## Before you play

Content includes firearms, lethal and non-lethal combat, unconscious/dead bodies,
body hiding and suspense inside a closed institution.

The download is for Windows. Windows supports native mouse input and procedural
audio when a device is available. Linux/macOS build checks do not imply identical
input or audio support, or a downloadable package for those platforms in this
release. There is no full character voice acting or online conversational AI.

Native fonts and window behavior, audio listening, first-time play and measured
completion time still need human acceptance. Preview images and automated tests
do not establish those results. Treat this as a playtest candidate and tell us
where it gets in your way.

## Feedback and project resources

Use [Issues](https://github.com/Rainflowers686/WRITEOVER-07/issues) to describe a
problem. Include the version, system, terminal, room and reproduction steps.
If you could not work out where to go, tell us which objective and prompts you
could see. Remove personal information from screenshots and logs; do not upload
your whole user-data directory.

[What's changed](docs/release/RELEASE_NOTES_v0.2.0-candidate.1.md) ·
[Development and building](docs/DEVELOPMENT.md) ·
[Course materials](docs/course/README.md) ·
[Version provenance](docs/release/VERSIONING.md)

The project uses C++17. The development guide leads into character rendering,
NPC behavior and saves; you do not need to compile it to play.
The repository has no open-source license. Public visibility does not grant
unrestricted redistribution or commercial use.
