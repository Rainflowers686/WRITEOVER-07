# ADR 0019 Native paths across save IO

Status: accepted for the course-submission save-path repair requested on 2026-09-16.

## Problem

On Windows with code page 936, a Chinese user-data directory produced a temporary
save but no final save. File streams received native-code-page strings, while the
replacement provider decoded those bytes as UTF-8 before calling MoveFileExW.
ASCII directories worked because both encodings use the same ASCII bytes.

## Decision

SaveManager, binary IO and the replacement-provider callback take
`const std::filesystem::path&`. Product save roles and load call sites retain the
path object. Windows replacement passes its native wide path to MoveFileExW.
POSIX keeps filesystem rename. Error messages use UTF-8 only for display.

This is a bounded public signature change authorized by the requested native-path
repair. String and literal callers still convert to filesystem paths, but custom
replacement callbacks must update their parameter types. Callers that already
own a path must pass it directly instead of serializing it with `string()`.

## Unchanged behavior

The save schema, section data, role order and rollback logic are unchanged.
Each file is replaced individually. Failed replacement preserves the existing
destination and leaves the temporary file with its caller. This change does not
redesign command-line encoding or other content loaders.

## Validation

Regression covers ASCII and Chinese paths, repeated manual/resume writes,
production Windows replacement, failure preservation and load after restarting
the game. The normal unit, header and FAST_REQUIRED gates remain required.
