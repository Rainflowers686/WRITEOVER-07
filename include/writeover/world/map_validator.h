#pragma once
// Runtime map/content validator (used by mapc tool and tests).
// Validation is deterministic (sorted iterations, stable messages).

#include "writeover/world/room.h"

#include <string>
#include <vector>

namespace writeover {

struct ValidationIssue {
    enum class Severity : uint8_t {
        Warning = 0,
        Error = 1,
    };
    Severity severity = Severity::Error;
    std::string message;
    std::string location;  // e.g. "room_01.col,row" or "npc_ref"
};

class MapValidator {
public:
    std::vector<ValidationIssue> Validate(const Room& room) const;

    bool HasErrors(const std::vector<ValidationIssue>& issues) const;
};

} // namespace writeover