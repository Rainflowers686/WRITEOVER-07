#include "writeover/core/save.h"

#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace writeover {

namespace {
constexpr size_t kMaxSaveSizeBytes = 8 * 1024 * 1024;
const char* kSaveExtension = ".wo07";
} // namespace

std::vector<uint8_t> ComposeSaveBuffer(const std::vector<SaveSection>& sections) {
    std::vector<uint8_t> out;
    Serializer s(out);

    SaveFileHeader header;
    header.magic = kSaveMagic;
    header.version = kSaveSchemaVersion;
    header.section_count = static_cast<uint32_t>(sections.size());
    header.reserved1 = 0;
    header.reserved2 = 0;

    // Header is serialized field-by-field (no struct dump, no padding risk).
    s.WriteU32(header.magic);
    s.WriteU32(header.version);
    s.WriteU32(header.section_count);
    s.WriteU32(header.reserved1);
    s.WriteU64(header.reserved2);

    const size_t header_size = 24;  // fixed wire header
    for (const auto& section : sections) {
        const uint32_t crc = Crc32(section.data.data(), section.data.size());
        s.WriteU32(static_cast<uint32_t>(section.id));
        s.WriteU32(static_cast<uint32_t>(section.data.size()));
        s.WriteU32(crc);
        s.WriteBytes(section.data.data(), section.data.size());
    }

    // Footer: overall CRC32 over everything after the header.
    const uint32_t footer_crc = Crc32(out.data() + header_size, out.size() - header_size);
    Serializer footer(out);
    footer.WriteU32(footer_crc);

    return out;
}

Result<std::vector<SaveSection>> ParseSaveBuffer(const uint8_t* data, size_t size) {
    if (size < 24 + 4) {
        return Result<std::vector<SaveSection>>::Err(
            kSaveMagic, "save buffer too small");
    }

    Deserializer d(data, size);
    const uint32_t magic = d.ReadU32();
    const uint32_t version = d.ReadU32();
    const uint32_t section_count = d.ReadU32();
    (void)d.ReadU32();  // reserved1
    (void)d.ReadU64();  // reserved2

    if (magic != kSaveMagic) {
        return Result<std::vector<SaveSection>>::Err(
            kSaveMagic, "not a valid save file (bad magic)");
    }
    if (version != kSaveSchemaVersion) {
        return Result<std::vector<SaveSection>>::Err(
            kSaveMagic + 1, "unsupported save version");
    }
    if (size > kMaxSaveSizeBytes) {
        return Result<std::vector<SaveSection>>::Err(
            kSaveMagic + 2, "save file too large");
    }
    if (section_count >= static_cast<uint32_t>(SaveSectionId::Count)) {
        return Result<std::vector<SaveSection>>::Err(
            kSaveMagic + 3, "save section count out of range");
    }

    const size_t payload_begin = d.Position() + static_cast<size_t>(section_count) * 12;
    if (payload_begin + 4 > size) {
        return Result<std::vector<SaveSection>>::Err(
            kSaveMagic + 4, "save buffer truncated");
    }

    std::vector<SaveSection> sections;
    sections.reserve(section_count);
    for (uint32_t i = 0; i < section_count; ++i) {
        SaveSectionHeader header;
        header.section_id = d.ReadU32();
        header.data_size = d.ReadU32();
        header.crc32 = d.ReadU32();
        if (header.section_id >= static_cast<uint32_t>(SaveSectionId::Count)) {
            return Result<std::vector<SaveSection>>::Err(
                kSaveMagic + 5, "unknown save section id");
        }
        if (header.data_size > d.Remaining() - 4) {
            return Result<std::vector<SaveSection>>::Err(
                kSaveMagic + 6, "save section truncated");
        }
        std::vector<uint8_t> section_data(header.data_size);
        if (header.data_size > 0) {
            d.ReadBytes(section_data.data(), section_data.size());
        }
        const uint32_t actual_crc = Crc32(section_data.data(), section_data.size());
        if (actual_crc != header.crc32) {
            return Result<std::vector<SaveSection>>::Err(
                kSaveMagic + 7, "save section checksum mismatch");
        }
        SaveSection section;
        section.id = static_cast<SaveSectionId>(header.section_id);
        section.data = std::move(section_data);
        sections.push_back(std::move(section));
    }

    const uint32_t footer_crc = d.ReadU32();
    const uint32_t actual_footer = Crc32(data + 24, size - 24 - 4);
    if (footer_crc != actual_footer) {
        return Result<std::vector<SaveSection>>::Err(
            kSaveMagic + 8, "save file checksum mismatch");
    }

    return Result<std::vector<SaveSection>>::Ok(std::move(sections));
}

Result<void> SaveManager::SaveWorld(const std::string& path,
                                    const std::vector<SaveSection>& sections) {
    const std::vector<uint8_t> buffer = ComposeSaveBuffer(sections);
    const std::string tmp = path + kSaveExtension + ".tmp";
    const std::string final = path + kSaveExtension;
    auto written = WriteFileBinary(tmp, buffer);
    if (written.IsError()) {
        return written;
    }
    return ReplaceFileAtomic(tmp, final);
}

Result<std::vector<SaveSection>> SaveManager::LoadWorld(const std::string& path) {
    const std::string final = path + kSaveExtension;
    auto data = ReadFileBinary(final);
    if (data.IsError()) {
        return Result<std::vector<SaveSection>>::Err(
            kSaveMagic + 9, "cannot read save file");
    }
    return ParseSaveBuffer(data.Value().data(), data.Value().size());
}

} // namespace writeover