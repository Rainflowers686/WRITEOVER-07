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

- required fields
- closed enums
- types and ranges
- finite values
- unique IDs
- cross-file refs
- actor known identities
- item owner/issuer/legal/current holder
- promise giver/receiver
- relationship participants
- container refs
- evidence refs
- narrator source refs
- quest refs
- terminal refs

If `data/systemic` does not exist or has no required seed, the validator fails.
Negative tests are included in the Python test suite.

## 4. Migration

Old seed field `"class": "Guard"` is accepted by the compiler as:
`CognitionTier=SemiHuman`, `Role=Guard`. New content should write `cognition`
and `role` explicitly.