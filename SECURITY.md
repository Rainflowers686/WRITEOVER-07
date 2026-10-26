# Security

WRITEOVER-07 is an offline, single-player game. It reads local saves and
compiled content files; it has no runtime language-model or network feature.
Security reports should identify the release or source commit they concern.

## Reporting a vulnerability

If the repository's Security tab offers **Report a vulnerability**, use that
private form. Otherwise, open an issue asking for a private contact without
including the exploit, a sensitive save or personal data. Wait for a private
channel before sharing those details.

Include the version, operating system, terminal, affected input format and
the smallest synthetic file or sequence that reproduces the problem. Do not
attach unrelated user files or absolute private paths. There is no guaranteed
response time or maintenance window for older playtest builds.

## Running and sharing builds

Download published builds from this repository's Releases page and check the
bundled `SHA256SUMS.txt`. Keep the executable and its matching data together.
Back up saves before testing an unfamiliar build or content file. Review
logs and screenshots before attaching them to a public report.

Checksums confirm consistency with the published package; the current
Windows playtest remains unsigned. Build and automated-test coverage on
other platforms does not establish equivalent input or audio behavior.
