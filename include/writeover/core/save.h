#pragma once
// Save system. Explicit binary wire format; NO wall-clock timestamp in the
// wire header (M-007 closure: byte-level round-trip equality is possible).
// Profile metadata is a SEPARATE file (M-009 closure) so loading an old world
// save never rolls back death/load/ending counters.

#include "writeover/common/io.h"
#include "writeover/common/result.h"
#include "writeover/common/serialize.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace writeover {

inline constexpr uint32_t kSaveMagic = 0x574F3037;  // "WO07"
inline constexpr uint32_t kSaveSchemaVersion = 1;

enum class SaveSectionId : uint32_t {
    Player = 0,
    World = 1,
    Rng = 2,
    Events = 3,
    Ai = 4,
    Narrative = 5,
    SettingsGameplay = 6,
    Count = 7,
};

// Wire header: fixed 24 bytes (magic4 + version4 + sectionCount4 + rsvd4 + rsvd8).
struct SaveFileHeader {
    uint32_t magic = kSaveMagic;
    uint32_t version = kSaveSchemaVersion;
    uint32_t section_count = 0;
    uint32_t reserved1 = 0;
    uint64_t reserved2 = 0;
};

// 12 bytes per section.
struct SaveSectionHeader {
    uint32_t section_id = 0;
    uint32_t data_size = 0;
    uint32_t crc32 = 0;
};

struct SaveSection {
    SaveSectionId id;
    std::vector<uint8_t> data;
};

// Serializes sections to the full .wo07 buffer (header + sections + footer
// CRC over everything after the header).
std::vector<uint8_t> ComposeSaveBuffer(const std::vector<SaveSection>& sections);

// Parses a buffer. Fail-closed on: bad magic, unsupported version, truncated
// buffer, section CRC mismatch.
Result<std::vector<SaveSection>> ParseSaveBuffer(const uint8_t* data, size_t size);

class SaveManager {
public:
    // path without extension; writes <path>.wo07 atomically.
    Result<void> SaveWorld(const std::string& path, const std::vector<SaveSection>& sections);
    Result<std::vector<SaveSection>> LoadWorld(const std::string& path);
};

} // namespace writeover