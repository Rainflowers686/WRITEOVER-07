// Installs a real MoveFileExW-based atomic-replace provider so save/profile
// writes are crash-atomic on Windows (tmp + MoveFileExW REPLACE_EXISTING).

#include "writeover/platform/platform_api.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <filesystem>

namespace writeover {

namespace {

#if defined(_WIN32)
Result<void> AtomicReplaceWin32(const std::filesystem::path& tmp_path,
                                const std::filesystem::path& dest_path, void*) {
    if (tmp_path.empty() || dest_path.empty()) {
        return Result<void>::Err(600, "atomic replace: empty path");
    }
    if (!MoveFileExW(tmp_path.c_str(), dest_path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return Result<void>::Err(601, "atomic replace (MoveFileExW) failed");
    }
    return Result<void>::Ok();
}
#endif

} // namespace

void InstallPlatformAtomicReplace() {
#if defined(_WIN32)
    AtomicReplaceProvider provider;
    provider.fn = &AtomicReplaceWin32;
    provider.user_data = nullptr;
    SetAtomicReplaceProvider(provider);
#endif
}

} // namespace writeover
