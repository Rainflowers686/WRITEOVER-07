# Product settings and actual consumers

Authority: `SettingsRegistry` reads/writes UTF-8 `settings.cfg`; product rows live
in `src/app/player_product.h`. Current user preferences do not rewind with a world
save. Menu changes call the existing save callback. Windows audio consumers remain
platform-dependent, not a promise of native POSIX audio or voice acting.

| Control | Domain | Runtime consumer |
|---|---|---|
| Language | first-run unset, en, zh-CN | PresentationText at draw time; paired IDs and bounded templates |
| Sensory detail | off, important, detailed | PerceptionFeed::Visible; history remains available |
| Text duration | short, normal, long | transient subtitle/feed presentation duration |
| Subtitles | on/off | RenderModule text presentation |
| High contrast | on/off | HUD/product compositor colors |
| Reduced shake | on/off | camera presentation offset |
| Reduced flicker | on/off | transient visual effects |
| Mouse sensitivity | 0..100 | existing mouse-look scale |
| Frame cap | Auto, 30, 60, 120 | Engine presentation cadence; Auto is bounded by simulation cadence |
| Field of view | 60..120 | projection and focused interaction |
| Master/narrator/SFX volume | 0..100 | installed audio provider and category gains |
| Difficulty | Easy, Normal, Hard | incoming damage 3/4, 1, 5/4; narrative difficulty predicate refresh |
| Interaction emphasis | on/off | HudFrame accent/bold; action text remains visible |
| Invert mouse Y | on/off | PreferenceMouseDelta before mouse look |
| Rebind keys | gameplay action table | InputMapper plus actual-binding control labels |

Difficulty does not change access or ending eligibility. It also retains the
existing difficulty-gated opening storylet (`storylet_r1_02`), so it is not accurate
to claim that no narrative consumer exists. Normal damage preserves the prior
combat baseline. Positive scaled damage is rounded down and bounded to at least 1.

Rebinding captures a fresh press, rejects conflicts/reserved arrows, and requires
confirmation before persistence. Esc cancels. Restore Default Bindings affects
only bindings. The developer action is not offered as a player rebind target.
Menus retain fixed arrows/F/Esc so a remap cannot remove the way back.

`gamepad_sensitivity`, `aim_assist`, and `tactical_focus` are legacy compatibility
fields without verified production consumers. They are not exposed as working
product features. `preset` remains the existing capability/default classification,
not a second page full of invented graphics options.

`language`, `invert_y`, sensory detail and text duration use the text preferences
path without additions to legacy binary Settings serialization. The text frame-cap
reader rejects values outside 0/30/60/120. Legacy `Settings::Load` reads its old raw
byte domain; current product startup uses SettingsRegistry, and world loading does
not use it to replace active preferences. The scheduler also bounds presentation.
No claim is made that arbitrary legacy binary settings are a validated new config
format.

Safe repeat play uses short text duration, history and pre-final recovery. This
closure does not add a dialogue-skip control: advancing queued narrative can alter
scheduling and suppress messages the player has not yet perceived. A presentation
setting must not skip systemic actions or mark a storylet fired.
