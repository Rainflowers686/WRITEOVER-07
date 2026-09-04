#include "src/app/runtime_paths.h"

#include <cstdlib>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <stdlib.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace writeover {

namespace {

std::filesystem::path AbsolutePath(std::filesystem::path value) {
    if (value.empty()) return value;
    std::error_code ec;
    if (value.is_relative()) {
        value = std::filesystem::absolute(value, ec);
    }
    if (ec) return value.lexically_normal();
    return value.lexically_normal();
}

std::filesystem::path ExecutablePathFromOs() {
#if defined(_WIN32)
#if defined(_MSC_VER)
    char* program_path = nullptr;
    if (_get_pgmptr(&program_path) == 0 && program_path != nullptr &&
        program_path[0] != '\0') {
        return std::filesystem::path(program_path);
    }
#endif
    return {};
#elif defined(__APPLE__)
    uint32_t size = 0;
    if (_NSGetExecutablePath(nullptr, &size) != -1 || size == 0) return {};
    std::vector<char> buffer(size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) return {};
    return std::filesystem::path(buffer.data());
#elif defined(__linux__)
    std::vector<char> buffer(PATH_MAX + 1, '\0');
    const ssize_t length = ::readlink("/proc/self/exe", buffer.data(),
                                      buffer.size() - 1);
    if (length <= 0) return {};
    buffer[static_cast<size_t>(length)] = '\0';
    return std::filesystem::path(buffer.data());
#else
    return {};
#endif
}

std::filesystem::path EnvironmentPath(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') return {};
    return std::filesystem::path(value);
}

bool LooksLikeRuntimeData(const std::filesystem::path& data_dir) {
    std::error_code ec;
    return std::filesystem::is_directory(data_dir / "rooms", ec) &&
           std::filesystem::is_regular_file(
               data_dir / "systemic" / "systemic_seed.bin", ec);
}

std::filesystem::path ResolveUserDataDir(
    const std::filesystem::path& executable_dir,
    const std::string& explicit_user_data_dir) {
    if (!explicit_user_data_dir.empty()) {
        return AbsolutePath(std::filesystem::path(explicit_user_data_dir));
    }

    std::filesystem::path base;
#if defined(_WIN32)
    base = EnvironmentPath("LOCALAPPDATA");
    if (base.empty()) {
        const auto profile = EnvironmentPath("USERPROFILE");
        if (!profile.empty()) base = profile / "AppData" / "Local";
    }
#elif defined(__APPLE__)
    const auto home = EnvironmentPath("HOME");
    if (!home.empty()) base = home / "Library" / "Application Support";
#else
    base = EnvironmentPath("XDG_DATA_HOME");
    if (base.empty()) {
        const auto home = EnvironmentPath("HOME");
        if (!home.empty()) base = home / ".local" / "share";
    }
#endif
    if (!base.empty()) return AbsolutePath(base / "WRITEOVER-07");

    // This is only a last-resort environment failure path.  Normal desktop
    // sessions provide one of the platform variables above.
    return AbsolutePath(executable_dir / "user-data");
}

} // namespace

RuntimePaths ResolveRuntimePaths(const std::string& argv0,
                                 const std::string& explicit_data_dir,
                                 const std::string& explicit_user_data_dir) {
    RuntimePaths paths;
    paths.executable_path = AbsolutePath(ExecutablePathFromOs());
    if (paths.executable_path.empty() && !argv0.empty()) {
        paths.executable_path = AbsolutePath(std::filesystem::path(argv0));
    }
    if (!paths.executable_path.empty() && paths.executable_path.has_parent_path()) {
        paths.executable_dir = paths.executable_path.parent_path();
    } else {
        std::error_code ec;
        paths.executable_dir = std::filesystem::current_path(ec);
    }

    if (!explicit_data_dir.empty()) {
        paths.data_dir = AbsolutePath(std::filesystem::path(explicit_data_dir));
    } else {
        const std::filesystem::path executable_data =
            paths.executable_dir / "data";
        const std::filesystem::path bundle_data =
            paths.executable_dir.parent_path() / "Resources" / "data";
        std::error_code ec;
        const std::filesystem::path current_data =
            std::filesystem::current_path(ec) / "data";
        if (LooksLikeRuntimeData(executable_data)) {
            paths.data_dir = executable_data;
        } else if (LooksLikeRuntimeData(bundle_data)) {
            paths.data_dir = bundle_data;
        } else if (LooksLikeRuntimeData(current_data)) {
            // Backward-compatible source-tree development fallback.  A
            // package never reaches this branch because its executable data
            // candidate is checked first.
            paths.data_dir = current_data;
        } else {
            // Keep the diagnostic path deterministic even when content is
            // missing; the caller emits the actionable missing-data error.
            paths.data_dir = executable_data;
        }
    }
    paths.data_dir = AbsolutePath(paths.data_dir);
    paths.user_data_dir = ResolveUserDataDir(paths.executable_dir,
                                             explicit_user_data_dir);
    return paths;
}

} // namespace writeover
