#include "writeover/common/io.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace writeover {

Result<std::vector<uint8_t>> ReadFileBinary(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return Result<std::vector<uint8_t>>::Err(1, "cannot open file: " + path.u8string());
    }
    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size < 0) {
        return Result<std::vector<uint8_t>>::Err(2, "cannot size file: " + path.u8string());
    }
    std::vector<uint8_t> data(static_cast<size_t>(size));
    if (size > 0) {
        file.read(reinterpret_cast<char*>(data.data()), size);
        if (!file) {
            return Result<std::vector<uint8_t>>::Err(3, "short read: " + path.u8string());
        }
    }
    return Result<std::vector<uint8_t>>::Ok(std::move(data));
}

Result<void> WriteFileBinary(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return Result<void>::Err(4, "cannot write file: " + path.u8string());
    }
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));
    }
    if (!file) {
        return Result<void>::Err(5, "write failed: " + path.u8string());
    }
    return Result<void>::Ok();
}

namespace {
Result<void> DefaultAtomicReplace(const std::filesystem::path& tmp_path,
                                  const std::filesystem::path& dest_path,
                                  void*) {
    std::error_code ec;
    // POSIX rename replaces an existing file atomically. Never unlink the good
    // destination first: a missing/unusable temp must leave it intact. Windows
    // production uses the installed MoveFileExW provider (including write-through).
    std::filesystem::rename(tmp_path, dest_path, ec);
    if (ec) {
        return Result<void>::Err(6, "atomic replace failed: " + dest_path.u8string() +
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

Result<void> ReplaceFileAtomic(const std::filesystem::path& tmp_path, const std::filesystem::path& dest_path) {
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
