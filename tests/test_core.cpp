#include "tests/test_harness.h"

#include "writeover/core/profile.h"
#include "writeover/core/save.h"
#include "writeover/core/settings.h"

namespace writeover {

namespace {

bool ComposeParseRoundTrip() {
    std::vector<SaveSection> sections;
    sections.push_back({SaveSectionId::Rng, std::vector<uint8_t>{1, 2, 3, 4}});
    sections.push_back({SaveSectionId::World, std::vector<uint8_t>{9, 9}});
    const std::vector<uint8_t> buffer = ComposeSaveBuffer(sections);
    const auto parsed = ParseSaveBuffer(buffer.data(), buffer.size());
    WO_CHECK(parsed.IsOk());
    if (!parsed.IsOk()) {
        return false;
    }
    WO_CHECK_EQ(static_cast<int64_t>(parsed.Value().size()), 2);
    return parsed.Value()[0].id == SaveSectionId::Rng &&
           parsed.Value()[0].data == std::vector<uint8_t>({1, 2, 3, 4}) &&
           parsed.Value()[1].id == SaveSectionId::World;
}

bool CorruptRejected() {
    std::vector<SaveSection> sections;
    sections.push_back({SaveSectionId::World, std::vector<uint8_t>{5, 5, 5}});
    std::vector<uint8_t> buffer = ComposeSaveBuffer(sections);
    buffer[30] ^= 0xFF;  // corrupt a payload byte -> CRC mismatch
    const auto parsed = ParseSaveBuffer(buffer.data(), buffer.size());
    return parsed.IsError();  // fail closed, never crash
}

bool TruncatedRejected() {
    std::vector<SaveSection> sections;
    sections.push_back({SaveSectionId::World, std::vector<uint8_t>{0, 1, 2, 3}});
    const std::vector<uint8_t> buffer = ComposeSaveBuffer(sections);
    const auto parsed = ParseSaveBuffer(buffer.data(), buffer.size() - 2);
    return parsed.IsError();
}

bool SettingsKeyValueRoundTrip() {
    SettingsRegistry registry;
    Settings s = Settings::Defaults();
    s.fov = 100;
    s.difficulty = 2;
    s.subtitles = false;
    s.key_bindings[static_cast<size_t>(GameAction::Jump)] = PhysicalKey::Space;

    if (registry.Save("settings_test.cfg", s).IsError()) {
        return false;
    }
    const auto loaded = registry.Load("settings_test.cfg");
    WO_CHECK(loaded.IsOk());
    if (!loaded.IsOk()) {
        return false;
    }
    const Settings& t = loaded.Value();
    return t.fov == 100 && t.difficulty == 2 && !t.subtitles &&
           t.key_bindings[static_cast<size_t>(GameAction::Jump)] == PhysicalKey::Space;
}

bool SettingsEncodeDecode() {
    Settings s = Settings::Defaults();
    s.fov = 110;
    s.difficulty = 2;
    s.key_bindings[static_cast<size_t>(GameAction::Fire)] = PhysicalKey::MouseLeft;
    std::vector<uint8_t> bytes;
    Serializer ser(bytes);
    s.Save(ser);
    Settings t = Settings::Defaults();
    Deserializer d(bytes.data(), bytes.size());
    t.Load(d);
    WO_CHECK(!d.HasError());
    return t.fov == 110 && t.difficulty == 2 &&
           t.key_bindings[static_cast<size_t>(GameAction::Fire)] == PhysicalKey::MouseLeft;
}

bool ProfileRoundTrip() {
    ProfileMeta meta;
    meta.death_count = 3;
    meta.load_count = 1;
    meta.endings_seen = 2;
    std::vector<uint8_t> bytes;
    Serializer ser(bytes);
    meta.Save(ser);
    ProfileMeta out;
    Deserializer d(bytes.data(), bytes.size());
    out.Load(d);
    return out.death_count == 3 && out.load_count == 1 && out.endings_seen == 2;
}

} // namespace

void RegisterCoreTests(TestHarness& test) {
    test.Add("save.compose_parse_round_trip", &ComposeParseRoundTrip);
    test.Add("save.corrupt_rejected", &CorruptRejected);
    test.Add("save.truncated_rejected", &TruncatedRejected);
    test.Add("settings.encode_decode", &SettingsEncodeDecode);
    test.Add("settings.key_value_disk", &SettingsKeyValueRoundTrip);
    test.Add("profile.round_trip", &ProfileRoundTrip);
}

} // namespace writeover