# Bilingual coverage receipt

Version: 0.2.0-candidate.1. Scope: existing production campaign and player UI.

`scripts/check_localization.py` reports 507/507 selected concrete display literals
covered, 81 paired narrative entries and 564 paired interface entries. Both
`--smoke` startup modes passed. Missing concrete entries: zero. Dynamic fragments
are explicitly emitted in the checker receipt and are not counted as exhaustive
coverage of every composed sentence.

The implementation translates final presentation strings and known templates,
including objectives, access feedback, inspection, acquired records, settings,
history and ending feedback. Tests caught and corrected an overly broad CLOSE
template before delivery. Language choice persists independently of world saves;
switching language does not create knowledge or alter route facts.

Actual production-cell renders inspected during this pass include Chinese boot,
B1 onboarding, case file and acquired-record pages, with English comparison
captures. `docs/course/assets/case-file-zh.png` is a retained production example.
The custom records capture's legacy replay predicate reports FAIL; that capture
is layout evidence only, not a successful gameplay-route gate.

Deliberately retained Latin text includes WRITEOVER-07, B1/floor identifiers,
SMG, FPS, key names and first-launch bilingual language-selection labels. Those
identify the product, physical directory or input rather than untranslated
functional prose. Catalog IDs and diagnostic trace strings are not player UI.

Han characters occupy two display columns in layout/backend handling. This is
not a promise of arbitrary Unicode shaping, emoji width, font availability or
identical glyph rasterization in all terminals. Foreground Chinese/English reading,
audio listening and human translation approval remain open acceptance items.

The two full READMEs agree on version, Windows startup, controls, 19 rooms/three
endings, diegetic 41 floors, save roles, language support, unsigned candidate
status, platform limitations and absence of an open-source license. They link to
the same release listing so only actually uploaded platforms are advertised.
