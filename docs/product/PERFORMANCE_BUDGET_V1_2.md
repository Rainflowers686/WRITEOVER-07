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

| System | Suggested worst1% |
|--------|------------------|
| NPC perception | <= 0.35 ms |
| NPC decision | <= 0.35 ms |
| Memory/social | <= 0.20 ms |
| Evidence/crime | <= 0.15 ms |
| Infrastructure | <= 0.20 ms |
| Quest/narrative | <= 0.20 ms |
| Player/weapon | <= 0.20 ms |
| Misc sim | <= 0.35 ms |

## Stated Bench Metric

The current benchmark API reports `worst_1pct_avg_ms` (average of the slowest
1% samples). It is not called a strict p99 quantile in code or documentation.

## Systemic Kernel Lookup Benchmark

Name: `systemic_kernel_lookup`

This proves the storage/lookup baseline only:

- 25 identity-bearing NPCs
- 500 evidence records
- 500 memory records
- 100 world events
- 30 hideable containers
- 100 items
- 120Hz loop

Budget: worst1% <= 2.0 ms.

## Systemic Update Workload Benchmark

Name: `systemic_update_workload`

This uses only current foundation capabilities:

- relationship lookup/update
- memory read/write
- evidence creation/query
- body drag state update
- discovery observation
- response application
- item transfer
- promise transition
- quest transition
- search outcome creation
- social exchange record
- terminal audit
- observability state updates
- event bridge

Synthetic scale ≈25 NPC, ≈500 memories, ≈500 evidence, ≈100 items,
≈30 containers, ≈100 events. 120Hz loop.

Budget: worst1% <= 2.0 ms.

Future full-M5 AI workload will be added as a separate benchmark when the AI
capabilities exist; it is not claimed by the current benchmark.