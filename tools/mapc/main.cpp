// mapc: content validator over compiled room (.woc) files.
// Exit codes: 0 = all valid, 1 = validation errors, 2 = cannot read.

#include "writeover/world/map_validator.h"
#include "writeover/world/room.h"

#include <cstdio>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: mapc <room.woc> [<room2.woc> ...]\n");
        return 2;
    }
    writeover::MapValidator validator;
    int failed = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string path = argv[i];
        auto room = writeover::LoadRoomFile(path);
        if (room.IsError()) {
            std::fprintf(stderr, "mapc: %s: %s\n", path.c_str(),
                         room.Error().message.c_str());
            ++failed;
            continue;
        }
        const auto issues = validator.Validate(room.Value());
        bool has_error = false;
        for (const auto& issue : issues) {
            const char* severity =
                issue.severity == writeover::ValidationIssue::Severity::Error
                    ? "ERROR"
                    : "WARN";
            std::fprintf(stderr, "mapc: [%s] %s (%s)\n", severity,
                         issue.message.c_str(), issue.location.c_str());
            if (issue.severity == writeover::ValidationIssue::Severity::Error) {
                has_error = true;
            }
        }
        if (has_error) {
            ++failed;
            std::fprintf(stderr, "mapc: FAIL %s\n", path.c_str());
        } else {
            std::fprintf(stderr, "mapc: OK %s\n", path.c_str());
        }
    }
    return failed == 0 ? 0 : 1;
}