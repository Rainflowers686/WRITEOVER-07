# ADR-0017: show acquired records without a second journal store

Case File now includes a bounded Key Records section. Its seven named campaign
entries use the existing KnowledgeAssetId writers (9301, 9302, 9401–9405), and
each entry requires the current player in known_by. A ShiftSchedule known to the
player adds the staff note. The existing pre-final interaction fact supplies the
authority review note. Unknown IDs, another actor's knowledge and unacquired
entries reveal nothing. The section has at most nine entries.

The projection lives in src/app/product_records.h: PlayerKeyRecords reads the
SystemicWorld; it does not write a fact, duplicate a transcript or change save
format. Existing Systemic serialization already preserves acquired knowledge.
The composition root appends these rows to Case File and adds existing force
and upper-alert facts to the ending summary. A route trace no longer names a
potential ending before the player sees the decision terminal.

In class: the stored model is knowledge plus ownership. The UI is a view of that
model, rebuilt after loading, rather than another database that can disagree.
Product tests cover empty knowledge, another actor's secret, acquired Subject 07,
Systemic serialization roundtrip and the authority review gate.

English/Chinese boot and Controls pages also state the actual content categories
without assigning an age rating. Long pages retain existing display-column
wrapping and scrolling. Production captures are layout evidence, not human
acceptance or a replacement for the mandatory route gate.
