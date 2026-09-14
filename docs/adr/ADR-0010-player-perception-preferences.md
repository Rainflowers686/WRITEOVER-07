# ADR-0010: presentation preferences and existing contextual inspect input

Status: accepted under Rain's current final product-pass authorization (sections
22, 77, 78). Scope is two Settings fields, not a save or input vocabulary change.

Sensory verbosity and duration belong to the existing SettingsRegistry text
preferences. Add bounded uint8_t fields (0..2), defaults Important and Normal.
They deliberately do not extend Settings::Save/Load's legacy binary layout:
these cosmetic preferences are not world state and loading a world must not
roll them back. Current world saves do not serialize SettingsGameplay.

The reserved, intentionally disabled AimDownSights action becomes Examine in
the current application policy. Its existing binding (default MouseRight) is
reused, and all product help labels call it Examine, not ADS. No GameAction or
PhysicalKey values change, so stored binding tables and replay vocabulary stay
compatible. The gameplay action is observed before fire/movement dispatch.

Recent perceptions are bounded, non-authoritative, and cleared on successful
load or full runtime reconstruction. Gameplay never reads message visibility.

Validation: bounded text preference parsing and round-trip; unchanged
binary settings and world save contracts; dynamic binding label tests; existing
input/replay gates plus product focus/close/held-action coverage.
