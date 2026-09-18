# WRITEOVER-07

[简体中文](README.zh-CN.md) · [Download the playtest](https://github.com/Rainflowers686/WRITEOVER-07/releases/latest)

A first-person immersive sim built out of characters. You wake up in the basement level of a facility with a number, a weapon and a file that cannot agree on who you are.

Read why a door refused you, dig through the records, talk to whoever is on shift, open the next checkpoint with a credential or a terminal, or shoot your way through. A room keeps what you did in it. Cameras, a body somebody finds, a raised alert level and the people who remember it decide how the next door opens.

![Character-built elevator entrance with the first-person weapon in view](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/lift_front.png)

*Rendered from the game's own character cells. Font, text size and window size change how it looks in a real terminal.*

A Windows x64 playtest build is out, with Simplified Chinese and English included. [Download the latest playtest](https://github.com/Rainflowers686/WRITEOVER-07/releases/latest) · [Player guide](docs/release/PLAYER_GUIDE.en.md)

## What is different here

- **The facility is made of characters.** Door frames, consoles, body armor, faces and the weapon in your hands are hand-authored character assets. color separates materials, and figures have front, back and side silhouettes.
- **Read the room before you decide to shoot.** A reader tells you why it refused you, a terminal keeps a record, and the person on duty knows something you do not.
- **The building keeps consequences.** Cameras, bodies, noise, alert state, credentials, NPC memory and route facts can matter after you leave the room.
- **Everyone here has a job.** Security patrols. A cleaner deals with what is left behind. Maintenance technicians know wiring and bypasses. Medical looks at the person before the number.
- **The narrator has its own account.** It files your actions in the facility's language, and its version is worth checking against the room, the records and the people.
- **The map opens in layers.** Records, Operations, Network, Security, Archive and Authority are not all handed to you at the start.
- **There is more than one way through.** Credentials, cooperation, terminals, maintenance bypasses, quiet movement, or force.
- **Three endings turn on what you leave behind.** The evidence you collect and the route you create decide what is available at the end.

## You wake up in B1

Medical staff check whether you can stand. A reader checks your credential. Security waits for an answer that fits procedure. Nobody is in a hurry to explain why you are here.

From the revival area you work upward through Calibration, Medical and Security, then into Records and Dispatch. The questions change on the way. A file can open the next door without explaining what happened. Some staff will help you. Others would rather finish their shift.

The campaign reaches the roof. The harder question is not only how to leave, but which version of you the institution will keep.

## Read the room first

The first-person view handles space. Where the door is, which guard is facing you, where to take cover when a fight starts. Text handles how the place works: what the equipment says, what a record logged, why the person in front of you will not step aside.

Walk into a room and you can check the equipment, read the notices and talk to staff before you draw. The Case File collects your current objective, known leads, acquired records and access state. If you miss a line, recent events and dialogue history keep it.

Controls are real-time keyboard and mouse. There is no command language to memorise. This is a single-player offline game, not a multiplayer MUD.

![Case File screen showing objectives, leads, acquired records and access status](docs/course/assets/case-file-zh.png)

*Objectives, leads, acquired records and access state in the Case File. The interface ships in both languages, and this capture shows the Chinese build.*

## The building remembers what you did

Cameras cover specific areas and can be interrupted. A person left unconscious and a person left dead are two different scenes to walk away from. Once somebody finds a body, the problem stops being about you and one guard.

The game keeps those events. Who saw it, who heard about it, how far the alert level rose and who formed a memory of it can follow you into later rooms. What you did in B1 can decide which department recognizes you, or make a checkpoint behave differently than it did before.

The cleaner is worth watching. How you treated him and what you left in the corridor change whether he covers for you, calls Medical, or reports the scene to Security. Hiding a body does not erase it from the simulation. Someone can arrive later, find it, and start another chain of consequences.

![Facility corridor with security hardware and a patrolling guard](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/security.png)

## The people here have shifts to finish

The content data defines 17 NPCs across 6 factions and 7 roles. Runtime instances depend on the current scene. Some walk patrol routes. Others stay at their post. Each one works with only part of the information.

They respond to sight and sound. A patrolling guard can react to a noise, walk over to inspect it, and move into a combat rhythm when the threat escalates. Memory is limited, but it is used later. The same event can mean something different to Security, Maintenance, Medical or a cleaner.

Dialogue is authored and behavior is driven by game state. The engineering rules exclude runtime language models and network code, so nobody here is a chatbot. The complexity comes from perception, memory, roles and rules interacting with one another.

![NPCs and guards built from hand-authored character assets](docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/human.png)

## The narrator is not neutral

> NARRATOR / Records does not ask who you are. It asks which version will survive.

It speaks the facility's language: permission, procedure, registration, filing. It turns your actions into an account it is willing to keep, and it offers its own kind of advice. Now and then its version does not match what you just saw.

Every line is authored text triggered by the situation and delivered as text. There is no voice acting in this build and no model writing lines while you play. Read the narrator against the records, the room and what people tell you.

The main-menu line **THE RECORD IS NOT THE EVENT** is there for a reason. The whole campaign keeps returning to the gap between what happened and what the institution eventually records.

## The facility does not reveal the whole map at once

The lift directory claims 41 levels, while the current campaign contains 19 playable rooms from B1 to the roof. Eight upper destinations become available as the required facts are earned:

- 1F Arrival / Public Lobby
- 8F Records Core
- 12F Operations Control
- 18F Network Node
- 24F Security Transfer
- 30F Executive Archive
- 36F Authority Core
- Roof / Exit

The directory distinguishes **CURRENT / AVAILABLE / LOCKED / RESTRICTED / SEALED**. Those labels read campaign state rather than acting as decorative menu text.

Service Concourse, Records Archive, Power Utility, Observation Gallery and Transit Control are not one straight corridor either. You can talk, inspect a terminal, backtrack for information, use credentials, create or avoid noise, or turn the situation into a fight. Knowledge from an earlier room can become a usable route condition later.

## More than one way through

Credentials, staff cooperation, terminal work and maintenance bypasses open particular doors. Moving quietly, making noise and changing a scene all draw different responses. Force remains available, and the room continues after you use it.

Three weapon slots are present: pistol, SMG and stunner. The stunner leaves people alive and still leaves a scene behind. The game distinguishes unconscious and dead states, and whether a body remains exposed or is later discovered can matter.

The three campaign endings do not all appear at the start. The evidence you gathered, the route you took and the consequences you left behind decide which options exist at the end. If you would rather work that out yourself, skip the spoiler section below; the ending names and their rough unlock conditions live there.

## Case File reflects what you actually know

The Case File is not a static quest list. It changes with the campaign and can show the current objective, next lead, acquired evidence, route information, a force trace and the amount of filed evidence.

Recent Events records important events the player could perceive. It is not an omniscient log of another room. Dialogue History keeps lines you already saw, so missing one does not automatically mean reloading.

Inspect follows the same information boundary. It describes bodies, equipment and evidence that are actually visible or known. It does not reveal an NPC's hidden internal state or future ending facts just because the examine key was pressed.

## The menus are part of the product, not a separate mock-up

The boot menu includes Continue, New Game, Controls / Help, Accessibility / Settings and Quit. Continue is visibly unavailable when no usable recovery save exists.

The Pause Menu changes with the current state and can expose:

- Resume
- Manual Save
- Load Last Save
- Restart Checkpoint
- Replay Final Choice
- Case File
- Recent Events
- Dialogue History
- Controls / Help
- Accessibility / Settings
- New Game
- Ending Summary
- Quit

Checkpoint, pre-final recovery and ending summary entries show unavailable states until they are real. A dead player cannot simply Resume or overwrite a manual save.

There are smaller protections too. Held input is fenced when a menu closes, so a mouse button pressed in the menu cannot turn into an accidental shot. Losing focus pauses the game. Shrinking the terminal below 48×18 character cells adds a separate resize pause. Enlarging the window removes that reason without cancelling a pause the player chose.

## Saves, recovery and replaying the last decision

Different save purposes remain separate:

- Manual Save
- Chapter Checkpoint
- Pre-Final Save
- Completion / Ending Save
- Latest Resume / Recovery

Near the ending, the game keeps a dedicated pre-final recovery point. After completion, **Replay Final Choice** can return to the decision without asking you to replay the entire campaign just to revisit an ending you already unlocked.

Loading an older save also clears transient presentation from the future timeline. A line that only happened later should not survive in Recent Events after the world has been restored to an earlier state.

Load is staged through temporary objects and validation before live state is committed. Truncated or malformed data is rejected rather than leaving half a world restored. File replacement is designed to preserve the previous good file if a new write fails.

New Game is not a teleport back to B1. It reconstructs the runtime world, clearing old facts, NPC memories, objectives, storylet state and inventory while leaving preferences and existing files on disk.

## Things the facility will not volunteer

The section below contains mild spoilers. None of these are required just to understand the controls, but players who pay attention to times, floor labels, cameras and records can find more.

<details>
<summary>Open: hidden routes, delayed consequences and small secrets</summary>

### The 02:10 staff route

B1 contains a specific maintenance detail: **maintenance shift change at 02:10**. It can enter player knowledge and feed a staff route. A line that looks like background scheduling can become access information.

### An unlisted observation route

At 18F Network Node, one objective is to find an **unlisted observation route**. It is not presented as a normal destination on the public lift directory.

### A camera blind spot pays off later

Observation Gallery can expose a camera blind spot. That knowledge matters later because Transit surveillance reads the corresponding state rather than treating it as flavour text.

### Power noise travels to Transit

Power Utility can be handled quietly with the technician or forced through a reroute. The loud solution produces utility noise. Transit later reads that consequence and Security can respond differently.

### A hidden body can still be found

Hide Body does not delete an entity. The project has routes in which a cleaner later reaches a hidden body, discovers it, and responds. Non-lethal and lethal outcomes also leave different scenes behind.

### Floors that never open

The directory lists **B4 SEALED / B3 NO-STOP / B2 RESTRICTED / 04 NO-STOP**. They are not playable maps in the current campaign. They are part of the facility's implied scale and deliberately leave some space unexplained.

### One more thing on the roof

The Roof contains a final-status terminal with the prompt **READ FINAL STATUS**. Completing an ending does not immediately throw the player back to the main menu.

### SUBJECT 07 keeps changing

The number follows the player from the opening record through Records, Executive Archive, Authority Core and Ending Summary. The mystery is not only who Subject 07 is. It is who gets to decide which version becomes official.

### Secret-class knowledge

The knowledge system includes Secret-class records. Some upper-route information enters the player's known state as secret knowledge instead of being treated like an ordinary hint.

### Rough unlock conditions for the endings

- **AMEND** is the baseline resolution once the final authority state is ready.
- **DISCLOSE** also requires the Network discovery and cooperation through Operations.
- **BREACH** depends on upper Security progress and force / alert facts.

The scenes themselves are not described here. The game decides which options exist from the facts you created.

</details>

## The whole facility is made of characters

Up close, the door frames, consoles, body armor, faces and weapons are all characters. People and weapons use hand-authored character assets, with color separating materials and different silhouettes for facing. The renderer aims for a first-person space you can read while the characters stay visible as characters.

The renderer outputs character cells directly. color and shading come from the palette, and depth influences glyph choice. Characters also use facing and distance variants, while doors, weapons and important props keep their own authored silhouettes.

![The campaign roofline, reached from B1](docs/production/evidence/ultra_visual_20260914/roof-skyline.png)

## At a glance

- 19 playable rooms, 32 authored scene transitions, 3 integrated campaign endings
- A 41-level diegetic directory with 8 major upper destinations unlocked over time
- Content data defines 17 NPCs across 7 roles and 6 factions
- Stateful NPC behavior using sight, sound, patrol, investigation, limited memory and combat escalation
- Cameras, bodies, noise, alert, credentials, relationships and route facts that can outlive a room
- Case File, Recent Events, Dialogue History and Ending Summary
- Manual / Checkpoint / Pre-Final / Completion / Resume save roles
- Replay Final Choice
- Simplified Chinese and English, switchable in-game
- Sensory detail, text duration, contrast, shake/flicker reduction, separate volume controls, FOV, difficulty and frame-rate settings
- Rebindable controls with conflict checks, confirmation and restore defaults
- Pistol, SMG and stunner weapon slots

## Download and play

The current playtest is on the [Releases page](https://github.com/Rainflowers686/WRITEOVER-07/releases/latest), under the `v0.2.0-course` tag.

1. In Assets, download `WRITEOVER-07-v0.2.0-candidate.1-win-x64.zip`. Do not take Source code.
2. Extract it fully, keep the `data` folder next to the executable, and run `WRITEOVER-07.exe`.
3. Pick Simplified Chinese or English at first launch, then start a new game. If a recovery save exists, Continue appears.

Platform: 64-bit Windows. The package is unsigned. Check where you downloaded it and verify it against the bundled `SHA256SUMS.txt`.

<details>
<summary>Version and provenance</summary>

- The source `PRODUCT_VERSION` is `0.2.0-candidate.1`.
- The latest release tag is `v0.2.0-course` (2026-09-18), pointing at commit `d5017c172dcd97aa36d27e7cf75282ed925611ab`; `version.json` inside the package records the same commit, the `windows-x64` platform and CI run `35290223738`.
- The package name keeps the candidate prefix because the release pipeline generates package names from the product version.
- `main` can carry documentation or compatibility commits made after the public package. To reproduce the course submission, use the exact delivery commit, source archive and checksums recorded in the receipt.

</details>

## Controls

| Action | Windows default |
|---|---|
| Move / look | WASD / mouse |
| Interact / examine | F / right mouse button |
| Fire / reload | Left mouse button / R |
| Select weapon | 1 pistol, 2 SMG, 3 stunner |
| Case File | F1 |
| Save / load recovery save | F5 / F9 |
| Pause / back | Esc |

On-screen control text reads the current bindings. If you remap an action, interaction, fire, Case File and load prompts follow the new key instead of continuing to display the default.

## Language, settings and comfort

Simplified Chinese and English can be switched in settings. Language is a presentation preference, so changing it does not reset campaign facts or replay the story.

The current product menu exposes:

- Sensory detail: Off / Important / Detailed
- Text duration: Short / Normal / Long
- Subtitles
- High Contrast
- Reduce Camera Shake
- Reduce Flicker
- Mouse Sensitivity
- Master Volume
- Narrator Volume
- SFX Volume
- Frame Limit: Auto / 30 / 60 / 120
- Language
- Field of View
- Difficulty: Easy / Normal / Hard
- Interaction Emphasis
- Invert Mouse Y
- Rebind Keys

Difficulty mainly changes damage taken rather than evidence or access routes. Auto can follow the fixed simulation cadence up to 120 Hz. That is not a promise about measured visible terminal frame rate.

## Before you play

The game contains firearms, lethal and non-lethal combat, unconscious and dead characters, moving and hiding bodies, and suspense set inside a closed institution.

Only Windows x64 is downloadable right now. Linux and macOS have build and automated checks, but no matching download, and those checks do not stand in for input or audio on those platforms. There is no full voice acting, no online AI and no multiplayer.

Automated tests do not cover how the game feels. First-session playtesting, audio impressions and completion time still need people.

## Feedback

Report problems in [Issues](https://github.com/Rainflowers686/WRITEOVER-07/issues) with the version, your system, the terminal, the room you were in and the steps to reproduce. If the problem was simply not knowing where to go, say what objective and hint you had at the time. Strip personal information out of screenshots and logs before posting.

## For developers

WRITEOVER-07 uses C++17. The running game combines a fixed-step simulation, DDA raycasting, native CharCell rendering, event-driven systemic gameplay, stateful NPCs, data-driven facts/storylets and transactional saves.

<details>
<summary>Open: engineering and product details</summary>

### State and authored content

- Current complete-campaign documentation records 19 playable rooms and 32 authored scene transitions (31 are wired into the campaign), 69 facts and 27 storylets.
- JSON / text remains reviewable authoring input and compiles into deterministic runtime content.
- Package validation fails closed when required content, Character-Art or critical text is missing.

### Input and menus

- Gameplay actions can be rebound by action. Conflicts are reported instead of silently overwritten.
- Arrow keys remain reserved for safe menu navigation, and Esc can cancel a binding capture.
- UI control text projects the player's actual bindings.
- Held menu input is fenced so closing a menu does not turn Mouse1 into an accidental shot.
- Focus loss and undersized-terminal pauses remain independent reasons.

### Bilingual terminal presentation

- Chinese and English present the same underlying game events.
- CJK layout uses displayed columns rather than UTF-8 byte counts.
- The 3D layer keeps single-width glyphs while double-width CJK belongs to the compositor layer.

### Save pipeline

- Manual, Checkpoint, Pre-Final, Completion and Resume roles remain separate.
- Load parses and validates staged objects before committing live state, with rollback on failure.
- Atomic-style replacement avoids deleting the known-good file first.
- Successful Load clears transient presentation that belongs to a later timeline.
- New Game reconstructs runtime ownership so old NPC memory and facts do not leak into a fresh run.

### Rendering and performance

- Fixed simulation and presentation cadence are separate.
- DDA supplies first-person geometry and the Character renderer outputs the final cell grid.
- The ANSI frame encoder supports delta output, including a zero-output fast path for fully unchanged frames.
- Current performance records report about 924 FPS equivalent CPU throughput for the internal Character-Art render, about 697 FPS equivalent for the internal full pipeline, and about 113.5 FPS for a redirected terminal-submit proxy. None of those is measured visible terminal FPS. Simulation runs at 120 Hz.

### Validation

- The final Release unit suite reports 247 / 247 passing tests.
- CI builds / tests Windows, Linux GCC, Linux Clang and macOS ARM64 paths.
- Linux AArch64 is a link / ELF architecture check, not a physical Kunpeng runtime test.
- Deterministic routes also cover complete campaign paths, cleaner history, hidden-body discovery, non-lethal handling, security bypass, save/load and ending conditions.

### About the foundation Hidden Loop

The narrative foundation also contains a more general **TruthBand × DominanceBand four-quadrant ending resolver** plus a separate **HIDDEN_LOOP / RESIDUAL** meta condition, with unit coverage.

Those are engine-level capabilities, not extra integrated endings in the current campaign. The playable campaign still resolves to **AMEND / DISCLOSE / BREACH**. This distinction matters because foundation code and shipped campaign content are not the same promise.

</details>

[Development and building](docs/DEVELOPMENT.md) · [Course materials](docs/course/README.md) · [Version and provenance](docs/release/VERSIONING.md) · [Release notes](docs/release/RELEASE_NOTES_v0.2.0-candidate.1.md)

The repository has no open-source license. Public visibility does not grant redistribution or commercial use.
