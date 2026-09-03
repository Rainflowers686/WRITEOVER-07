# PERFORMANCE BUDGET v1.2 — WRITEOVER-07

Reference: 120Hz, 8.33ms frame budget.

## Hard CPU Targets

| Metric | Target |
|--------|--------|
| TOTAL CPU p99 | <= 6.0 ms |
| Simulation p99 | <= 2.0 ms |
| Render | <= 3.0 ms |
| Terminal encode | <= 1.0 ms |

## Simulation Budget Suggestions

| System | Suggested p99 |
|--------|---------------|
| NPC perception | <= 0.35 ms |
| NPC decision | <= 0.35 ms |
| Memory/social | <= 0.20 ms |
| Evidence/crime | <= 0.15 ms |
| Infrastructure | <= 0.20 ms |
| Quest/narrative | <= 0.20 ms |
| Player/weapon | <= 0.20 ms |
| Misc sim | <= 0.35 ms |

These are initial contracts. If benchmarks prove adjustments are necessary,
the change must be documented and justified.

## Systemic Benchmark Contract

The synthetic benchmark `SYSTEMIC_BENCH` must use real kernel state:

- 25 identity-bearing NPCs
  - 5 Full-style high-frequency nearby
  - 20 Semi-Human/offscreen
- 500 evidence records
- 500 memory records
- 100 world events
- 30 hideable containers
- 100 items
- 120Hz simulation loop

The measured Simulation p99 must be <= 2.0 ms.

No fake benchmark: the loop must perform actual systemic lookups/updates
against the in-memory kernel.
