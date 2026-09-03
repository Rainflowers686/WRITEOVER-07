#include "writeover/common/io.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace writeover {

Result<std::vector<uint8_t>> ReadFileBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return Result<std::vector<uint8_t>>::Err(1, "cannot open file: " + path);
    }
    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size < 0) {
        return Result<std::vector<uint8_t>>::Err(2, "cannot size file: " + path);
    }
    std::vector<uint8_t> data(static_cast<size_t>(size));
    if (size > 0) {
        file.read(reinterpret_cast<char*>(data.data()), size);
        if (!file) {
            return Result<std::vector<uint8_t>>::Err(3, "short read: " + path);
        }
    }
    return Result<std::vector<uint8_t>>::Ok(std::move(data));
}

Result<void> WriteFileBinary(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return Result<void>::Err(4, "cannot write file: " + path);
    }
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));
    }
    if (!file) {
        return Result<void>::Err(5, "write failed: " + path);
    }
    return Result<void>::Ok();
}

namespace {
Result<void> DefaultAtomicReplace(const std::string& tmp_path,
                                  const std::string& dest_path,
                                  void*) {
    std::error_code ec;
    // Best-effort: remove destination first, then rename. On Windows this is
    // not crash-atomic; the platform layer replaces this provider with
    // MoveFileExW (see src/platform/windows/win_file_io.cpp).
    std::filesystem::remove(dest_path, ec);
    std::filesystem::rename(tmp_path, dest_path, ec);
    if (ec) {
        return Result<void>::Err(6, "atomic replace failed: " + dest_path +
                                        " (" + ec.message() + ")");
    }
    return Result<void>::Ok();
}
} // namespace

namespace {
AtomicReplaceProvider g_atomic_replace = {&DefaultAtomicReplace, nullptr};
} // namespace

void SetAtomicReplaceProvider(AtomicReplaceProvider provider) {
    g_atomic_replace = provider.fn ? provider : AtomicReplaceProvider{&DefaultAtomicReplace, nullptr};
}

Result<void> ReplaceFileAtomic(const std::string& tmp_path, const std::string& dest_path) {
    if (g_atomic_replace.fn == nullptr) {
        return Result<void>::Err(7, "no atomic replace provider installed");
    }
    return g_atomic_replace.fn(tmp_path, dest_path, g_atomic_replace.user_data);
}

bool BytesEqual(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin());
}

} // namespace writeover