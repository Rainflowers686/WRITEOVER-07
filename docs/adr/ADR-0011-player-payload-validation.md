# ADR-0011: explicit Player payload and bounded combat restoration

Status: accepted within Rain's 2026-09-15 post-audit closure authorization.

The global save section table, CRC envelope and public gameplay types are unchanged.
The private application Player payload now starts with PLY2, a U16 version (1)
and zero reserved U16. Its room, locomotion, combat, health/dead and jump fields
are all required. Unknown versions, truncation and trailing bytes are rejected.

The previous complete-campaign candidate's unmarked, complete layout remains
readable. Pre-health or pre-jump experimental payloads are no longer accepted:
their absent tails are indistinguishable from a truncated later payload. A save
that lacks those fields fails closed instead of becoming full health. Existing
files are never rewritten merely by trying to load them.

Version 1 stores bounded remaining fire cooldown in the existing combat wire
fields (next_fire_frame = remaining, last_shot_frame = 0). The loader rebases it
to the current game frame. This avoids carrying an old process's absolute
deadline into a fresh process. The old unmarked layout has no saved clock; its
fire interval is conservatively preserved for at most one authored cooldown.

Combat parsing stages values and validates slot, all magazine/reserve bounds,
boolean encoding, finite spread, reload duration/owner viability and fire interval
before assignment. A weapon switch already cancels reload, so the selected slot
is its owner; no second reload-slot state is introduced.

Role and resume saves remain individually atomic. The extracted private write
helper preserves primary-first order and reports both results. A failed resume
update leaves a good primary and pre-final save intact; Continue deterministically
keeps the existing resume, and the player receives the existing partial-failure
message. No cross-file transaction or global save architecture is added.

Focused evidence: invalid combat mutations; every truncated Player prefix;
valid-CRC envelope with truncated legacy tail; version/trailing-byte rejection;
relative-timer canonical round trip; role/secondary-write faults; actual previous
candidate completion save loaded in the new Debug runtime. Full final validation
and cross-platform results belong to the final closure report, not this decision.
