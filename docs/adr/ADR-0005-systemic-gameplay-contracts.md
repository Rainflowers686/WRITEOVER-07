# ADR-0005: Systemic Gameplay Contracts v1.2

Status: Accepted

## Context

WRITEOVER-07 moved from a technically runnable immersive FPS framework to a
Systemic Gameplay Foundation. Multiple modules need shared contracts for body
state, evidence, items/provenance, promises, relationships, alerts, narrator
observability, and world persistence.

## Decision

Add a small dependency-light `writeover_systemic` library with POD/value
records and one in-memory `SystemicWorld` kernel. Add `SaveSectionId::Systemic`
as a forward-compatible save section. Add three vertical kernel tests and a
real synthetic benchmark.

The public headers affected:

- `include/writeover/common/ids.h` (adds ItemId, EvidenceId, PromiseId,
  ContainerId, MemoryId)
- `include/writeover/core/save.h` (adds Systemic save section)
- `include/writeover/systemic/systemic.h` (new systemic contracts)

## Consequences

- Modules can share systemic state without duplicate ownership.
- Save format is extended without rewriting existing sections or the parser.
- The kernel is intentionally minimal: no full AI, shop UI, hacking UI, or
  complete content.
- Public header baseline updated to account for the new/changed headers.
