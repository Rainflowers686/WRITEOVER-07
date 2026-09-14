#pragma once
// Cross-platform file helpers. Atomic replace is provided by a replaceable
// provider so platform code (MoveFileExW) can be wired at the composition
// root without leaking windows.h into semantic modules.

#include "writeover/common/result.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

Result<std::vector<uint8_t>> ReadFileBinary(const std::string& path);
Result<void> WriteFileBinary(const std::string& path, const std::vector<uint8_t>& data);

// Default provider: rename-over-existing, without pre-deleting the destination.
// POSIX replacement is atomic; the Windows runtime installs MoveFileExW-based
// replacement. On failure the caller retains ownership of the temporary file.
using AtomicReplaceFn = Result<void> (*)(const std::string& tmp_path,
                                         const std::string& dest_path,
                                         void* user_data);

struct AtomicReplaceProvider {
    AtomicReplaceFn fn = nullptr;
    void* user_data = nullptr;
};

void SetAtomicReplaceProvider(AtomicReplaceProvider provider);
Result<void> ReplaceFileAtomic(const std::string& tmp_path, const std::string& dest_path);

// True if the two byte buffers are identical (used by save round-trip tests
// on deterministic sections after timestamp/metadata normalization).
bool BytesEqual(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);

} // namespace writeover
