# ADR-0006: Systemic Final Correction

Status: Accepted

## Context

The independent review found major blockers in the systemic foundation:
mixed cognition/role identity, undirected relationships, non-fail-closed
deserialization, missing runtime integration, incomplete product truth
sources, and insufficient benchmark naming.

## Decision

- Separate `CognitionTier`, `Faction`, and `Role`.
- Make relationships directed and range-validated.
- Add explicit Promise/Quest FSM transitions.
- Make `SystemicWorld::Deserialize` fail-closed and return `Result`.
- Integrate one `SystemicWorld` into the composition root and include
  `SaveSectionId::Systemic` in the real runtime save path.
- Compile seed JSON to binary and load it at runtime.
- Split NarratorAuthorityState and per-source NarratorObservabilityState.
- Add search, item lifecycle, social exchange, knowledge asset, quest, and
  terminal typed seams without implementing full gameplay systems.
- Add truthful kernel lookup and current-foundation update benchmarks.
- Freeze full floor directory, ending model, visual production contract, and
  active product sources.

## Consequences

- Foundation can be handed to M1–M6 without unresolved major blockers.
- The new contracts are typed and testable but intentionally do not implement
  full NPC AI, quest engine, shop, hacking UI, or full 41-level maps.
