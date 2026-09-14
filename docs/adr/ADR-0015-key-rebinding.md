# ADR-0015: bounded keyboard rebinding

The product closure authorizes in-game key rebinding through the existing
InputMapper and settings table. A private backend observer records physical
press edges from the same poll stream; it does not consume a second stream or
change gameplay input types. Keyboard/pointer focus and delta handling remain
owned by InputRuntime. Repeated key-down events do not create new capture edges.

Settings opens a binding list. Select an action, press a key, review the pending
value, then confirm with F or cancel with Escape. Used keys are rejected instead
of silently unbinding another action. Arrow keys and Escape remain reserved for
safe menu navigation. The existing supported physical-key vocabulary is retained.
Restore Defaults replaces only binding tables, not other preferences or saves.

Only a confirmed change updates Settings and reapplies the same table to the
live mapper. Capture/pending phases do not load a save on an incidental F9 press.
Loss of focus cancels capture. Physical arrow/F/Escape menu shortcuts remain
available independently of gameplay mappings, including legacy unbound actions.

No public API or save schema changes are required. Menu capture is transient.
The replay counters continue to observe their original backends beneath the
adapter, preserving replay evidence rather than reporting zero consumed events.
