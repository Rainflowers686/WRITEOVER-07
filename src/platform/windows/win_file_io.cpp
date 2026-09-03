// Installs a real MoveFileExW-based atomic-replace provider so save/profile
// writes are crash-atomic on Windows (tmp + MoveFileExW REPLACE_EXISTING).

#include "src/platform/windows/platform_api.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <string>
#include <vector>

namespace writeover {

namespace {

#if defined(_WIN32)
std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                        static_cast<int>(utf8.size()), nullptr, 0);
    if (len <= 0) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                        &wide[0], len);
    return wide;
}

Result<void> AtomicReplaceWin32(const std::string& tmp_path,
                                const std::string& dest_path, void*) {
    const std::wstring tmp = Utf8ToWide(tmp_path);
    const std::wstring dest = Utf8ToWide(dest_path);
    if (tmp.empty() || dest.empty()) {
        return Result<void>::Err(600, "atomic replace: path conversion failed");
    }
    if (!MoveFileExW(tmp.c_str(), dest.c_str(),
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