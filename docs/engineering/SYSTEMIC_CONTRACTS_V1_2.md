# SYSTEMIC CONTRACTS v1.2 — WRITEOVER-07

## 1. Runtime Ownership

- M1/Core runtime state owns the single `SystemicWorld`.
- `GameServices` in the composition root exposes it.
- No module owns a second `SystemicWorld`.

## 2. Headers / Library

- `include/writeover/systemic/systemic.h`
- `src/systemic/systemic.cpp`
- Library: `writeover_systemic`, depends only on `writeover_common`.

## 3. Cognition vs Role

`ActorClass` is removed. Use:

- `CognitionTier`: `Full` or `SemiHuman`
- `Faction`: Security, Medical, etc.
- `Role`: Guard, Cleaner, Doctor, Researcher, Technician, etc.

Old `Guard` actor class migrates to `CognitionTier=SemiHuman`, `Role=Guard`.

## 4. Relationships

- Relationships are directed: A -> B.
- `SetRelationship` upserts exact directed pair.
- `GetRelationship(a,b)` never returns `(b,a)`.
- Ranges: Trust/Fear/Respect/Suspicion/Debt/Attachment [0,1]; IdeologicalAlignment [-1,1].
- `debt` means A feels indebted to B.

## 5. Promise FSM

Legal transitions:

- Offered -> Accepted
- Offered -> Cancelled
- Accepted -> Fulfilled
- Accepted -> Broken
- Accepted -> Expired
- Accepted -> Cancelled

All others rejected. Expired is not recorded as Broken.

## 6. Alert / Evidence Canonical Owners

- `AlertState` is canonical alert truth.
- `DerivedAlertScalar()` is read-only.
- Evidence collection is canonical; `GetEvidenceCount()` returns `evidence_.size()`.
- No independent mutable `facility_alert` or `evidence_count`.

## 7. Body / Discovery

- Drag lifecycle: BeginDrag/UpdateDrag/EndDrag.
- HideBody rejects Alive bodies, invalid containers, hidden duplicates, over-capacity.
- `DiscoverBody` only creates observation/evidence/memory/event; it never auto-escalates alert.
- Response is applied through `ApplyDiscoveryResponse`.

## 8. Cross-Module Types

- SearchAction/SearchOutcome
- SocialExchangeRecord
- KnowledgeAssetRecord
- QuestRecord
- TerminalRecord / TerminalSession / TerminalAuditLog
- ObservationSource
- NarratorAuthorityState / NarratorObservabilityState

## 9. WorldEvent Bridge

- `WorldEvent` is runtime transport.
- `SystemicEvent` is durable semantic ledger.
- `BridgeWorldEventOnce` preserves `source_world_event_id` and prevents double-recording.

## 10. Save

- `SaveSectionId::Systemic` is part of the runtime save.
- `SystemicWorld::Deserialize` is fail-closed and returns `Result<SystemicWorld>`.
- Seed binary is compiled from `data/systemic/systemic_seed.json` by Python and loaded by `LoadSeedBinary`.
## 11. Item Ownership Semantics

- owner: original asset owner, does not change on loan/transfer.
- legal_holder: current authorized holder.
- current_holder: actual physical holder.
- reported_stolen: only set by explicit `ReportItemStolen`.
- revoked: blocks reader acceptance.

`TransferItem` changes current_holder only.
`LoanItem` changes current_holder, preserves legal_holder.
`AuthorizedTransferItem` changes current_holder and legal_holder.
`TheftItem` changes current_holder and records theft, but does not auto-report.
`ReportItemStolen` sets reported_stolen without revoking.
`RevokeCredential` blocks reader.
`ReturnItem` returns to current legal_holder.
