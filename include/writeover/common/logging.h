#pragma once
// Thread-safe diagnostic logger. Owned by EngineContext (no global singleton).

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

namespace writeover {

enum class LogLevel : uint8_t {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4,
};

class Logger {
public:
    void SetMinLevel(LogLevel min);
    void SetFile(const std::string& path);

    void Log(LogLevel level, const char* module, std::string_view msg);
    void Info(const char* module, std::string_view msg) { Log(LogLevel::Info, module, msg); }
    void Warn(const char* module, std::string_view msg) { Log(LogLevel::Warning, module, msg); }
    void Error(const char* module, std::string_view msg) { Log(LogLevel::Error, module, msg); }

private:
    std::mutex mutex_;
    LogLevel min_level_ = LogLevel::Info;
    std::string file_path_;
    bool file_open_ = false;
};

} // namespace writeover