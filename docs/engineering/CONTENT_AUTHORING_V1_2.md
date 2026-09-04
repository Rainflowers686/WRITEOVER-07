# CONTENT AUTHORING v1.2 — WRITEOVER-07

## 1. Principle

Authors write JSON. A Python compiler produces deterministic binary seed data.
Runtime never parses authoring JSON.

## 2. Seed Pipeline

```powershell
python tools/systemic/systemic_schema_check.py --data-dir data
python tools/systemic/compile_systemic_seed.py --src data/systemic/systemic_seed.json --out data/systemic/systemic_seed.bin
```

The runtime loads `data/systemic/systemic_seed.bin` via `SystemicWorld::LoadSeedBinary`.

## 3. Validator Rules

- The five compiled root arrays (`actors`, `items`, `containers`,
  `evidenceSeeds`, and `promiseSeeds`) are required and must be lists.
- Entries must be objects with required IDs, closed enums, valid types/ranges,
  and finite numeric values.
- IDs are unique within each record domain. Actor identity references, item
  holder references, item container references, and promise participants are
  resolved before the file is accepted.
- Item location is explicit (`Holder`, `Ground`, or `Container`); a ground
  item requires a room and a container item requires an existing container.
- Important items (`Badge`, `Weapon`, `KeyItem`, `Credential`) require an
  owner and non-empty provenance tags. `reportedStolen` and `revoked` are
  independent booleans.
- Evidence requires a closed evidence type, subject, room, and bounded
  visibility. Containers require a room, positive capacity, bounded
  concealment/accessibility, and closed routine tags.
- Promise participants must resolve to an actor or `player`; promise status
  must use the closed promise enum.

The root also reserves `questSeeds`, `knowledgeSeeds`, `terminalSeeds`, and
`observationSeeds` for the future data-driven compiler. If present, they must
be lists and must currently be empty; this prevents a JSON section from being
silently ignored by the compiler. Their runtime contracts already exist in
`SYSTEMIC_CONTRACTS_V1_2.md`, but they are not claimed as compiled content in
this foundation gate.

If `data/systemic` does not exist or has no required seed, the validator fails.
Negative tests are included in the Python test suite, including malformed
structure, unknown references, duplicate IDs, invalid locations, invalid
enums, and reserved non-empty sections.

## 4. Migration

Old seed field `"class": "Guard"` is accepted by the compiler as:
`CognitionTier=SemiHuman`, `Role=Guard`. New content should write `cognition`
and `role` explicitly.
