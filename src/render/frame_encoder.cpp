#include "writeover/render/frame_encoder.h"
#include "utf_append.h"
#include "text_layout.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace writeover {

namespace {

// RGB channels have at most three decimal digits; write directly into the
// persistent payload instead of allocating a string for each color run.
void AppendChannel(std::string& out, uint8_t value) {
    if (value >= 100) out.push_back(static_cast<char>('0' + value / 100));
    if (value >= 10) out.push_back(static_cast<char>('0' + (value / 10) % 10));
    out.push_back(static_cast<char>('0' + value % 10));
}

// After reset or cursor movement the terminal's current colors are not the
// CharCell defaults. Force one complete SGR at the beginning of each run.
CharCell InitialSgrState(const CharCell& first) {
    CharCell state = first;
    state.flags ^= 1;
    return state;
}

void AppendSgr(std::string& s, const CharCell& prev, const CharCell& cell) {
    if (prev.fg_r == cell.fg_r && prev.fg_g == cell.fg_g &&
        prev.fg_b == cell.fg_b && prev.bg_r == cell.bg_r &&
        prev.bg_g == cell.bg_g && prev.bg_b == cell.bg_b &&
        (prev.flags & 0x01) == (cell.flags & 0x01)) {
        return;
    }
    if (cell.flags & 0x01) {
        s.append("\x1b[1m");
    } else {
        s.append("\x1b[0m");
    }
    s.append("\x1b[38;2;");
    AppendChannel(s, cell.fg_r);
    s.push_back(';');
    AppendChannel(s, cell.fg_g);
    s.push_back(';');
    AppendChannel(s, cell.fg_b);
    s.append("m");
    s.append("\x1b[48;2;");
    AppendChannel(s, cell.bg_r);
    s.push_back(';');
    AppendChannel(s, cell.bg_g);
    s.push_back(';');
    AppendChannel(s, cell.bg_b);
    s.append("m");
}

} // namespace

std::string BuildSgr(const CharCell& prev, const CharCell& cell) {
    std::string s;
    AppendSgr(s, prev, cell);
    return s;
}

void AnsiFrameEncoder::Reset() {
    has_prev_ = false;
    prev_.clear();
    prev_width_ = 0;
    prev_height_ = 0;
}

EncodeResult AnsiFrameEncoder::Encode(const CharCell* frame, int width, int height,
                                       std::string& out, EncodeMode mode) {
    EncodeResult result;
    if (frame == nullptr || width <= 0 || height <= 0) {
        return result;
    }
    const size_t cell_count = static_cast<size_t>(width) * height;

    // Resize safety: if the previous frame has different dimensions, treat
    // this as a fresh full frame (never index prev_ with the new layout).
    if (has_prev_ && (prev_width_ != width || prev_height_ != height)) {
        prev_.clear();
        has_prev_ = false;
    }

    // If no previous frame, emit FULL.
    if (!has_prev_) {
        prev_.assign(frame, frame + cell_count);
        has_prev_ = true;
        prev_width_ = width;
        prev_height_ = height;
        result.full = true;
        result.changed_cells = cell_count;

        // Full encode: home + every cell with color-run compression.
        out.append("\x1b[2J\x1b[H");
        CharCell state = InitialSgrState(frame[0]);
        for (int y = 0; y < height; ++y) {
            out.append("\x1b[" + std::to_string(y + 1) + ";1H");
            for (int x = 0; x < width; ++x) {
                const CharCell& cell = frame[static_cast<size_t>(y) * width + x];
                if (cell.flags & text::kWideTail) continue;
                AppendSgr(out, state, cell);
                detail::AppendUtf8(out, cell.code_point);
                state = cell;
            }
        }
        out.append("\x1b[0m");
        result.payload_bytes = out.size();
        return result;
    }

    // Byte equality is sufficient (not necessary) for identical cells. Padding
    // differences merely fall through to the semantic comparison below. Avoid
    // allocating a mask, scanning wide pairs and copying an unchanged frame.
    if (std::memcmp(prev_.data(), frame, cell_count * sizeof(CharCell)) == 0) {
        result.unchanged = true;
        return result;
    }

    // Compare with previous frame.
    size_t changed = 0;
    std::vector<bool> changed_mask(cell_count, false);
    for (size_t i = 0; i < cell_count; ++i) {
        const CharCell& cur = frame[i];
        const CharCell& prv = prev_[i];
        if (cur.code_point != prv.code_point ||
            cur.fg_r != prv.fg_r || cur.fg_g != prv.fg_g ||
            cur.fg_b != prv.fg_b || cur.bg_r != prv.bg_r ||
            cur.bg_g != prv.bg_g || cur.bg_b != prv.bg_b ||
            cur.flags != prv.flags) {
            changed_mask[i] = true;
            ++changed;
        }
    }

    // Equal fields with different padding are also unchanged. No snapshot
    // update is necessary: padding has no terminal meaning.
    if (changed == 0) {
        result.unchanged = true;
        return result;
    }

    // A delta must include both halves of either the old or new wide glyph.
    // Otherwise cursor positioning into a continuation can erase its head.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t i = static_cast<size_t>(y) * width + x;
            if (!changed_mask[i]) continue;
            if (x > 0 && ((frame[i].flags | prev_[i].flags) & text::kWideTail) && !changed_mask[i - 1]) {
                changed_mask[i - 1] = true; ++changed;
            }
            if (x + 1 < width && ((frame[i].flags | prev_[i].flags) & text::kWideHead) && !changed_mask[i + 1]) {
                changed_mask[i + 1] = true; ++changed;
            }
        }
    }

    // Update snapshot.
    std::memcpy(prev_.data(), frame, cell_count * sizeof(CharCell));
    prev_width_ = width;
    prev_height_ = height;

    result.changed_cells = changed;

    // If forced full or >50% cells changed, emit FULL.
    const float ratio = static_cast<float>(changed) / static_cast<float>(cell_count);
    if (mode == EncodeMode::ForceFull || ratio > 0.50f) {
        result.full = true;
        out.append("\x1b[H");
        CharCell state = InitialSgrState(frame[0]);
        for (int y = 0; y < height; ++y) {
            out.append("\x1b[" + std::to_string(y + 1) + ";1H");
            for (int x = 0; x < width; ++x) {
                const CharCell& cell = frame[static_cast<size_t>(y) * width + x];
                if (cell.flags & text::kWideTail) continue;
                AppendSgr(out, state, cell);
                detail::AppendUtf8(out, cell.code_point);
                state = cell;
            }
        }
        out.append("\x1b[0m");
        result.payload_bytes = out.size();
        return result;
    }

    // DELTA: emit cursor-positioned runs for changed rows.
    // For each row, find changed runs and emit cursor move + SGR + glyphs.
    for (int y = 0; y < height; ++y) {
        const int row_start = y * width;
        int run_start = -1;
        for (int x = 0; x < width; ++x) {
            const bool is_changed = changed_mask[static_cast<size_t>(row_start + x)];
            if (is_changed && run_start < 0) {
                run_start = x;
            }
            if (!is_changed && run_start >= 0) {
                // Emit a delta run for [run_start, x).
                out.append("\x1b[");
                out.append(std::to_string(y + 1));
                out.push_back(';');
                out.append(std::to_string(run_start + 1));
                out.append("H");
                CharCell state = InitialSgrState(frame[static_cast<size_t>(row_start + run_start)]);
                for (int cx = run_start; cx < x; ++cx) {
                    const CharCell& cell = frame[static_cast<size_t>(row_start + cx)];
                    if (cell.flags & text::kWideTail) continue;
                    AppendSgr(out, state, cell);
                    detail::AppendUtf8(out, cell.code_point);
                    state = cell;
                }
                run_start = -1;
            }
        }
        if (run_start >= 0) {
            out.append("\x1b[");
            out.append(std::to_string(y + 1));
            out.push_back(';');
            out.append(std::to_string(run_start + 1));
            out.append("H");
            CharCell state = InitialSgrState(frame[static_cast<size_t>(row_start + run_start)]);
            for (int cx = run_start; cx < width; ++cx) {
                const CharCell& cell = frame[static_cast<size_t>(row_start + cx)];
                if (cell.flags & text::kWideTail) continue;
                AppendSgr(out, state, cell);
                detail::AppendUtf8(out, cell.code_point);
                state = cell;
            }
        }
    }
    out.append("\x1b[0m");
    result.payload_bytes = out.size();
    return result;
}

} // namespace writeover
