# ADR-0007: PVS-01 Runtime Seam Closure

- 状态: 已实现
- 日期: 2026-09-04
- 决策者: Rainflowers686
- 受影响模块: M1, M2, M3, M4, M5, M6

## 背景

PVS-01 needs a small amount of public contract surface to expose the
production half-block renderer's authored prop kinds and to audit the
SystemicWorld records that cross the player, world, AI, narrative, and save
boundaries. The existing private implementation could not prove those seams
without either duplicating state or relying on a test-only back door.

## 决策

- Add production sprite kinds for the authored door, medical, crate, elevator,
  camera, and sign presentations.
- Add const inspection accessors for body drag, search, social exchange,
  terminal session/audit, and canonical observation-source records.
- Make body concealment emit a typed `BodyHidden` event after the body/container
  state transition so player, world, AI, narrative, and save seams observe the
  same semantic fact.
- Keep SystemicWorld as the shared semantic kernel. These additions expose
  records for verification; they do not move gameplay ownership into the
  kernel or add a general-purpose gameplay framework.
- Keep `ObservationSource` storage canonical in `SystemicWorld::sources_`; the
  existing observability vector remains a synchronized compatibility mirror.

## 备选方案

Keep the APIs private and validate only by inspecting serialized bytes. This
was rejected because it would hide cross-module semantic transitions and make
the PVS regression test a false-positive again.

## 影响

The public header hashes change and are recorded in the contract baseline.
The additions are const/read-only inspection, enum values, and one typed event
emission; they do not alter the save wire format or the 120 Hz simulation clock.
Existing callers remain source-compatible because the new `frame` argument has
a default; production route content uses the new renderer kinds.

## 回滚/回退

Revert this ADR together with the header changes and their implementation call
sites, then restore the prior contract baseline hashes. Do not revert the
authoritative worktree with a destructive Git operation.
