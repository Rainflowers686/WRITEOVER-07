# Post-audit public contract review

Scope authority is Rain's final post-audit closure request: repair save replacement,
live resizing, bilingual/CJK presentation and functional preferences without a new
renderer or campaign framework. This record reconciles those implemented changes
with the public-header hash gate; it does not disable the gate or certify CI.

| Header | Actual delta | Design record |
|---|---|---|
| common/io.h | Correct replacement guarantee comment; unchanged function signatures | F02 repair in 5f49c29; DefaultAtomicReplace preserves destination on failure |
| core/settings.h | language and invert_y preference fields; frame-cap comment | ADR-0014 and ADR-0016; no binary Settings payload additions |
| render/hud.h | health_label pointer and interaction_highlight boolean with backward defaults | ADR-0014 and ADR-0016; synchronous presentation-only consumers |
| render/terminal_backend.h | Two unused CharCell flag bits assigned to CJK head/tail; GetCaps refresh semantics documented | ADR-0012 live terminal surface and ADR-0013 compositor display columns |

All other public-header hashes remain unchanged. CharCell layout and terminal
method signatures remain unchanged. The Settings C++ object grows, requiring a
normal source rebuild; claiming ABI compatibility would be incorrect. The outer
save schema remains 1, while the private Player payload has an explicit versioned
format and a bounded reader for the previous complete layout.

The first local FAST_REQUIRED run at 73f826f stopped on these four stale hashes
after its gameplay, save, package and benchmark steps passed. Its FAIL receipt is
retained. Updating exactly these reviewed entries requires rerunning the header
gate; it does not turn that historical run into PASS.
