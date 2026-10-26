# Contributing

Start with [AGENTS.md](AGENTS.md), the public contracts under
`include/writeover/`, the applicable module notes under `codex/`, and the
[engineering constitution](engineering/00_ENGINEERING_CONSTITUTION.md).
Build instructions are in [Development](docs/DEVELOPMENT.md).

## Proposing a change

Describe the player-visible problem, the affected module and a reproducible
example in an issue. The project uses a main-only workflow; agree on a change
with the owner before submitting it for review. Keep each task within its
owned files and the repository's 400-line diff limit.

Public types, save formats, simulation timing, rendering and dependency rules
are frozen. Changes to those contracts require the approval and ADR process
in the engineering documents. Documentation corrections should preserve
version pins, command arguments, release provenance and the distinction
between playable content and engine capabilities.

## Verification

Use the checks required by [AGENTS.md](AGENTS.md) and the
[test strategy](docs/engineering/TEST_STRATEGY.md). Report the exact commands,
exit codes and remaining limitations. Write `NOT_RUN` for a check you did not
execute. Old test counts and benchmarks describe the recorded build; they
do not validate a later change.

For documentation changes, check both languages, relative links, anchors and
image paths. Keep the screenshots and descriptions tied to actual gameplay.

## Feedback and sensitive reports

Ordinary bugs belong in the repository's Issues. Include the build, operating
system, terminal, room and steps to reproduce. Use [SECURITY.md](SECURITY.md)
for sensitive reports, and remove personal paths and data from attachments.

The repository has no open-source license. Discuss rights to any proposed
code, art or text with the owner before adding it.
