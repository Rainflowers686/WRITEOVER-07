# ADR-0012: live terminal surface

The post-audit closure authorizes runtime resizing without changing gameplay,
save schema, or the Character-Art renderer.

`ITerminalBackend::GetCaps()` keeps its signature and now refreshes the current
visible surface using GetConsoleScreenBufferInfo or TIOCGWINSZ. Unknown redirected
surfaces retain their previous dimensions. The application fits the canvas to
that surface, bounded by the requested maximum, and updates both projection and
interaction consumers through their existing shared dimensions.

Below 48 x 18 the compositor draws only a resize notice. An independent transient
surface-pause reason freezes the existing game clock and suppresses held input.
Recovery removes only that reason: existing menus and manual pause remain intact.
No surface state is saved and no quest, room, health, or NPC state is changed.

ANSI full frames address each row explicitly, without a bottom-margin newline
that would scroll and invalidate delta history. A dimension change clears the
old canvas and forces a full frame. Win32 output uses the visible viewport origin.

Focused tests cover shrink/minimum/recovery, redirected dimensions, panel bounds,
full-frame invalidation and unchanged-frame recovery. Real foreground host resize
acceptance remains a separate human check, not proven by these unit tests.
