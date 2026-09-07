# Recovery Implementation-01 — Red Team 1: Code Truth

Scope: the bounded B1 recovery slice only.  This review attacks identity,
state ownership, event provenance, timing, persistence, and replay assertions.

## Findings

### Credential and door authority — VERIFIED for the B1 slice

`ReaderAcceptsItem()` remains a credential-validity predicate.  The B1
interaction resolver additionally requires `ItemHeldBy(badge, player)` before
checking clearance and invoking the existing `InfrastructureSystem` door
state.  The systemic counterfactual test covers the badge on the body, held by
another entity, held by the player, transferred/dropped, and revoked cases.
The B1 denied replay proves the negative route, while the success replay proves
unlock, open, and crossing.

### Terminal authority — VERIFIED for the B1 slice

The terminal interaction is selected by bounded target identity and requires
the player-held, non-revoked badge before creating the terminal session and
audit record.  The terminal-denied replay reaches the terminal and proves that
no session is created and no gate opens without the credential.

### Body and memory state — VERIFIED with bounded limitations

Runtime body creation follows the non-lethal hit feedback, transfers the held
badge to the created body, and uses the existing systemic body/search/drag/hide
seams.  Repeated observation and gunshot tests refresh a semantic memory rather
than appending an identical record every decision interval.  A silent interval
or a separate meaningful event creates a new record.

### Witness provenance — VERIFIED for the corrected runtime path

The autonomous runtime skips stunned and dead actors before perception and
before cleaner discovery.  The incapacitated-cleaner regression proves that a
stunned cleaner cannot create a discovery memory or response.  Body discovery
itself is emitted by an executing cleaner after arrival and inspection; it is
not a due-frame-only DirectWitness.

### Cleaner execution and timing — VERIFIED for the bounded route

The cleaner moves toward the cart in deterministic increments, is blocked by
the active world query when an AABB blocks the route, must arrive within the
arrival radius, and then spends a separate inspection interval before
discovery.  A hidden body in another room is rejected.  The blocked-route test
proves that the due frame alone cannot expose the body.

### Player health and persistence — VERIFIED for the bounded route

Guard line-of-sight posts `EventPlayerDamage`; the player module applies the
authoritative health mutation, the renderer reads that state, and the player
enters a dead state at zero.  Health and dead state are serialized with bounded
validation.  The health replay proves damage, death, save, load, recovery, and
positive live health after recovery.

This is only a bounded health/death proof seam for the recovery slice.  It does
not claim complete enemy combat, enemy attack selection, tactical behavior,
weapon behavior, or general combat coverage.

### Replay truth — VERIFIED

Replay output reports process exit, consumed keyboard/mouse events, expected
state, and chapter checkpoint separately.  The success route requires the
non-lethal/body/search/hide/discovery/response/terminal-session/gate/save-load
facts.  Denied routes require denial and absence of the corresponding success
facts.  `REPLAY_PROCESS_EXIT_OK` is therefore not the gameplay assertion.

## Known bounded limitations

- Cleaner movement is deterministic direct movement with collision rejection;
  it is not general pathfinding.
- B1 target interaction is bounded to the authored B1 object set.  Legacy
  room transitions outside that slice still contain older proximity seams.
- The captured executable evidence is real, but a manual 60–90 second play
  session has not been independently performed in this automation run.

These limitations are not silently converted into PASS claims.  They are
outside the current recovery slice and remain visible to Rain during review.

## Red-team conclusion

No open Fatal or Major code-truth blocker was found in the bounded B1 slice.
The implementation is ready for Rain review, not for a Gold or public-release
claim.
