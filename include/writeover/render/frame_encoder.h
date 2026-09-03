#pragma once
// ANSI frame encoder (HK-5 closure): previous-frame delta encoding.
//  - First frame           -> FULL payload
//  - Identical frame       -> no payload (unchanged fast path)
//  - Small change          -> DELTA payload (cursor move + changed runs)
//  - Large change (>50%)   -> degrade to FULL
// Produces ANSI VT escapes only; no Win32 dependency, so it is unit-testable
// on any host. The Win32 terminal backend consumes this encoder.
//
// Per-cell truecolor SGR is emitted only when the color state changes
// (color-run compression).

#include "writeover/render/terminal_backend.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

enum class EncodeMode : uint8_t {
    Auto = 0,      // choose full vs delta by changed-cell ratio
    ForceFull = 1, // always full payload
};

struct EncodeResult {
    size_t payload_bytes = 0;  // bytes appended to out (0 == unchanged)
    size_t changed_cells = 0;  // cells that differed from previous frame
    bool full = false;         // this encode was a full-frame payload
    bool unchanged = false;    // no payload emitted
};

class AnsiFrameEncoder {
public:
    // Encodes `frame` relative to the previous frame into `out` (appends).
    // Updates the internal previous-frame snapshot.
    EncodeResult Encode(const CharCell* frame, int width, int height,
                        std::string& out, EncodeMode mode = EncodeMode::Auto);

    // Clears the previous-frame snapshot; the next Encode is a full frame.
    void Reset();

    bool HasPrevious() const { return has_prev_; }
    size_t PreviousCellCount() const { return prev_.size(); }
    int PreviousWidth() const { return prev_width_; }
    int PreviousHeight() const { return prev_height_; }

private:
    std::vector<CharCell> prev_;
    bool has_prev_ = false;
    int prev_width_ = 0;
    int prev_height_ = 0;
};

// Builds an ANSI TrueColor SGR prefix for a cell; returns empty when the
// cell color state is identical to the previous cell (color-run compression).
std::string BuildSgr(const CharCell& prev, const CharCell& cell);

} // namespace writeover