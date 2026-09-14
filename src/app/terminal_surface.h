#pragma once

#include "writeover/render/terminal_backend.h"
#include <algorithm>

namespace writeover {

// Presentation-only sizing. Unknown redirected surfaces keep the requested
// dimensions; real surfaces may shrink and recover up to the configured cap.
struct TerminalSurface {
    int width = 1;
    int height = 1;
    bool Usable() const { return width >= 48 && height >= 18; }
};

inline TerminalSurface FitTerminalSurface(int requested_width, int requested_height,
                                         const TerminalCaps& caps) {
    return {std::max(1, caps.max_width > 0 ? std::min(requested_width, caps.max_width) : requested_width),
            std::max(1, caps.max_height > 0 ? std::min(requested_height, caps.max_height) : requested_height)};
}

} // namespace writeover
