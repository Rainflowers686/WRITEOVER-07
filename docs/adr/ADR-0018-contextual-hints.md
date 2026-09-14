# ADR-0018: contextual hints are not gameplay progression

ProductOnboarding owns six bounded, one-time presentation hints: move/look,
center-focus interaction, Case File, an empty magazine with reserve ammunition,
first acquired records and a saved checkpoint. It uses the current bindings.
Movement and interaction demonstrations dismiss their hints. Opening Case File
also dismisses its record-navigation hint. An empty magazine can preempt a lower
value hint. Expiration and cooldown use the existing paused game-frame clock.

The compositor requests a hint only when no real interaction prompt is present.
Menus, campaign panels, death and pause suppress it. It neither steals input nor
queues narration, writes facts, advances quests or changes the save schema.
The opening no longer advertises firing before combat is relevant.

A successful load suppresses the hint sequence for that continued run. New Game
reconstructs ProductOnboarding with the rest of the composition. This deliberately
avoids adding tutorial bookkeeping to campaign saves; it is not a persistent
tutorial-completion system. Existing reader/terminal prompts, lift controls and
checkpoint feedback remain the local instructions for those interactions.

Product tests cover demonstrated actions, custom interaction bindings, Chinese
projection, reload priority, no repeated reminders, Continue suppression and
New Game reset. The production B1 movement capture shows the focus hint; it is
layout evidence rather than a first-time human playtest.
