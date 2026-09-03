# STATE / ENDING MODEL v1.2 — WRITEOVER-07

## 1. Canonical Ranges

All axes listed below are normalized unless explicitly noted.

| Axis | Range |
|---|---|
| Humanity | [0,1] |
| Violence | [0,1] |
| Reliability | [0,1] |
| Coercion | [0,1] |
| PublicTrust | [0,1] |
| SecurityStanding | [0,1] |
| MedicalResearchStanding | [0,1] |
| MaintenanceStanding | [0,1] |
| NarratorAlignment | [-1,1] |
| NarratorDominance | [0,1] |
| Autonomy | [0,1] |
| TruthExposure | [0,1] |
| SelfKnowledge | [0,1] |
| InfrastructureIntegrity | [0,1] |
| TimelineInstability | [0,1] |
| ResidualMemoryPressure | [0,1] |
| CivilianCasualties | integer >= 0 |
| SecurityCasualties | integer >= 0 |
| PromisesBroken | integer >= 0 |
| EvidenceSet | collection, canonical count = evidence records |
| PreviousCycleEvidenceCount | integer >= 0 |
| OperatorRoomFound | bool |

## 2. Macro Ending Resolution

Four Macro Endings are deterministic, resolved from the final save snapshot.

Fixed bands:

- TruthBand: LOW < 0.60, HIGH >= 0.60
- DominanceBand: LOW < 0.50, HIGH >= 0.50

Quadrants:

- LOW Truth + HIGH Dominance = COMPLIANCE
- HIGH Truth + HIGH Dominance = CURATOR
- LOW Truth + LOW Dominance = ESCAPE
- HIGH Truth + LOW Dominance = OVERWRITE

No random number is used. Build/config differences do not change the result.

## 3. Hidden Meta Ending

`HIDDEN_LOOP / RESIDUAL ENDING` is not a fifth macro quadrant.

Deterministic condition (must all be true):

- SelfKnowledge >= 0.75
- TimelineInstability >= 0.60
- ResidualMemoryPressure >= 0.50
- OperatorRoomFound = true
- PreviousCycleEvidenceCount >= 3
- At least one selected NPC has a ResidualMemory callback
- At least one death/load/rewind history condition

## 4. Non-Macro Axis Use

Other axes do not create additional Macro Endings. They drive:

- NPC epilogue
- building outcome
- social outcome
- presentation
- post-credit variation
- storylet eligibility
- HIDDEN_LOOP / RESIDUAL ENDING preconditions

## 5. Hysteresis

Presentation may use hysteresis. Ending evaluation always resolves from final snapshot deterministically with the bands above. `>=` belongs to the high side.

## 6. Save Fiction

VisibleNarrativeSave and ProtectedRecovery remain separate. The Narrator may manipulate visible save presentation, but never irreversibly deletes last recoverable progress.