# ADR-0016: functional product preferences

The current closure requires settings to have observable consumers. The existing
FOV, narrator/SFX volume, difficulty and interaction-highlight fields are exposed
alongside the previously functional options. Pointer-Y inversion adds one
settings.cfg-only boolean; the binary Settings layout remains unchanged.

Normal difficulty retains the established damage values. Easy receives 75% and
Hard 125% of incoming player damage, rounded down with positive damage clamped
to at least one. This bounded scalar does not change weapons, AI sensing, route
eligibility, or timing. The existing narrative difficulty consumer also refreshes
immediately rather than retaining its startup value.

Interaction emphasis changes the prompt's accent and bold style; disabling it
does not hide action text. HudFrame adds that boolean with its prior true default.
FOV is already read by render and interaction projection. Audio changes call the
existing master/SFX/narrator volume interface. Inversion applies before the
existing ApplyMouseLook function and never changes the mouse backend.

Legacy gamepad sensitivity, aim assist and tactical focus have no verified
production consumer in this build. They remain compatibility fields, are not
advertised as active settings, and must be listed as reserved in the final
settings inventory. No fake controls or new gamepad framework are added.
