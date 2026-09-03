#include "writeover/render/frame_encoder.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace writeover {

std::string BuildSgr(const CharCell& prev, const CharCell& cell) {
    if (prev.fg_r == cell.fg_r && prev.fg_g == cell.fg_g &&
        prev.fg_b == cell.fg_b && prev.bg_r == cell.bg_r &&
        prev.bg_g == cell.bg_g && prev.bg_b == cell.bg_b &&
        (prev.flags & 0x01) == (cell.flags & 0x01)) {
        return {};
    }
    std::string s;
    if (cell.flags & 0x01) {
        s.append("\x1b[1m");
    } else {
        s.append("\x1b[0m");
    }
    s.append("\x1b[38;2;");
    s.append(std::to_string(cell.fg_r));
    s.push_back(';');
    s.append(std::to_string(cell.fg_g));
    s.push_back(';');
    s.append(std::to_string(cell.fg_b));
    s.append("m");
    s.append("\x1b[48;2;");
    s.append(std::to_string(cell.bg_r));
    s.push_back(';');
    s.append(std::to_string(cell.bg_g));
    s.push_back(';');
    s.append(std::to_string(cell.bg_b));
    s.append("m");
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
        out.append("\x1b[H");
        CharCell state{};
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const CharCell& cell = frame[static_cast<size_t>(y) * width + x];
                out.append(BuildSgr(state, cell));
                out.append(CharCellToUtf8(cell.code_point));
                state = cell;
            }
            out.push_back('\n');
            state = CharCell{};
        }
        out.append("\x1b[0m");
        result.payload_bytes = out.size();
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

    // Update snapshot.
    std::memcpy(prev_.data(), frame, cell_count * sizeof(CharCell));
    prev_width_ = width;
    prev_height_ = height;

    result.changed_cells = changed;

    // UNCHANGED: no cells differ.
    if (changed == 0) {
        result.unchanged = true;
        result.payload_bytes = 0;
        return result;
    }

    // If forced full or >50% cells changed, emit FULL.
    const float ratio = static_cast<float>(changed) / static_cast<float>(cell_count);
    if (mode == EncodeMode::ForceFull || ratio > 0.50f) {
        result.full = true;
        out.append("\x1b[H");
        CharCell state{};
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const CharCell& cell = frame[static_cast<size_t>(y) * width + x];
                out.append(BuildSgr(state, cell));
                out.append(CharCellToUtf8(cell.code_point));
                state = cell;
            }
            out.push_back('\n');
            state = CharCell{};
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
                CharCell state{};
                for (int cx = run_start; cx < x; ++cx) {
                    const CharCell& cell = frame[static_cast<size_t>(row_start + cx)];
                    out.append(BuildSgr(state, cell));
                    out.append(CharCellToUtf8(cell.code_point));
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
            CharCell state{};
            for (int cx = run_start; cx < width; ++cx) {
                const CharCell& cell = frame[static_cast<size_t>(row_start + cx)];
                out.append(BuildSgr(state, cell));
                out.append(CharCellToUtf8(cell.code_point));
                state = cell;
            }
        }
    }
    out.append("\x1b[0m");
    result.payload_bytes = out.size();
    return result;
}

} // namespace writeover