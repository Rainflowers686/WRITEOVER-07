#include "writeover/common/logging.h"

#include <cstdio>
#include <ctime>
#include <fstream>

namespace writeover {

namespace {
const char* LevelName(LogLevel level) {
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    }
    return "?";
}
} // namespace

void Logger::SetMinLevel(LogLevel min) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = min;
}

void Logger::SetFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_path_ = path;
    file_open_ = !file_path_.empty();
}

void Logger::Log(LogLevel level, const char* module, std::string_view msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < min_level_) {
        return;
    }
    std::fprintf(stderr, "[%s][%s] %.*s\n", LevelName(level), module,
                 static_cast<int>(msg.size()), msg.data());
    if (file_open_) {
        // Reopen once per call: session logs are low-frequency; keeps the
        // file handle fresh and simplifies crash-evidence gathering.
        std::ofstream file(file_path_, std::ios::app);
        if (file) {
            const std::time_t now = std::time(nullptr);
            std::tm tm_buf{};
            localtime_s(&tm_buf, &now);
            char stamp[32] = {};
            std::strftime(stamp, sizeof(stamp), "%H:%M:%S", &tm_buf);
            file << "[" << stamp << "][" << LevelName(level) << "][" << module << "] "
                 << std::string(msg) << "\n";
        }
    }
}

} // namespace writeover