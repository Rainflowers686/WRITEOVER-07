# ADR-0013: compositor display columns

The bilingual product closure authorizes a bounded Unicode presentation seam.
World geometry and authored character sprites remain single-column CharCells.

English/Simplified Chinese overlays share UTF-8 decoding, clipping, wrapping,
and drawing by terminal columns. Han and fullwidth punctuation occupy two cells;
ASCII and structural box drawing occupy one. Invalid UTF-8 is replaced, not read
past the buffer. This is not a general grapheme/emoji shaping engine.

CharCell retains its layout. Two previously unused flag bits identify a wide
head (0x08) and continuation (0x10), only in the text compositor. ANSI emits the
head once and skips the reserved tail, expanding delta damage across both cells.
Win32 uses leading/trailing CHAR_INFO attributes for the same pair. These flags
are never gameplay state, serialized state, or authored world raster pixels.

The existing public terminal contract documents the assigned flag semantics;
there are no new public types or dependencies. Tests must cover mixed text,
right-edge clipping, wrapped controls, resize, and wide-to-narrow delta updates.
