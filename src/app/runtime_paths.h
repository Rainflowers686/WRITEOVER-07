#pragma once

// Release/runtime path resolution belongs to the composition root.  It keeps
// packaged content relative to the executable or app bundle while keeping
// mutable saves and settings outside the read-only player package.

#include <filesystem>
#include <string>

namespace writeover {

struct RuntimePaths {
    std::filesystem::path executable_path;
    std::filesystem::path executable_dir;
    std::filesystem::path data_dir;
    std::filesystem::path user_data_dir;
};

RuntimePaths ResolveRuntimePaths(const std::string& argv0,
                                 const std::string& explicit_data_dir,
                                 const std::string& explicit_user_data_dir);

} // namespace writeover
