# ADR-0014: language is a presentation preference

The current closure request authorizes selectable English and Simplified Chinese.
`Settings::language` is empty until the player chooses, then `en` or `zh-CN`.
Only settings.cfg stores this value. The legacy binary Settings representation
and gameplay save sections do not change.

The application loads paired stable-id TAB UTF-8 files alongside the existing
recovery text bank. Events, dialogue queues and perceived history retain their
canonical English payloads; a shared display projection resolves the selected
language at draw time. Switching languages therefore also redraws existing
history without replaying an event or changing storylet state.

Resource pairs must contain identical IDs, and conflicting duplicate canonical
text is rejected. Unknown display strings remain unchanged during migration;
this fallback is not evidence of complete translation. Final acceptance requires
the separate functional-text coverage audit and bilingual production captures.

The supported text preference frame-cap domain is 0/30/60/120. Zero means the
existing scheduler-bounded automatic cadence, not unlimited presentation.

HudFrame adds a presentation-only health-label pointer with the existing English
default. It is consumed synchronously by the compositor and never retained or
serialized. Boot asks for language when the preference is absent; Settings can
switch it without restarting the campaign.
