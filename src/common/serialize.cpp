#include "writeover/common/serialize.h"

#include <cstring>

namespace writeover {

namespace {
inline void Require(Deserializer& d, size_t n) {
    if (d.Remaining() < n) {
        d.MarkError();
    }
}
} // namespace

void Serializer::WriteU8(uint8_t v) { out_->push_back(v); }
void Serializer::WriteU16(uint16_t v) {
    out_->push_back(static_cast<uint8_t>(v & 0xFF));
    out_->push_back(static_cast<uint8_t>(v >> 8));
}
void Serializer::WriteU32(uint32_t v) {
    out_->push_back(static_cast<uint8_t>(v & 0xFF));
    out_->push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out_->push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out_->push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}
void Serializer::WriteU64(uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out_->push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
    }
}
void Serializer::WriteF32(float v) {
    uint32_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    WriteU32(bits);
}
void Serializer::WriteF64(double v) {
    uint64_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    WriteU64(bits);
}
void Serializer::WriteBytes(const void* data, size_t n) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    out_->insert(out_->end(), bytes, bytes + n);
}
void Serializer::WriteString(std::string_view s) {
    WriteU32(static_cast<uint32_t>(s.size()));
    WriteBytes(s.data(), s.size());
}

uint8_t Deserializer::ReadU8() {
    if (Remaining() == 0) {
        error_ = true;
        return 0;
    }
    return data_[pos_++];
}
uint16_t Deserializer::ReadU16() {
    Require(*this, 2);
    uint16_t v = 0;
    v |= static_cast<uint16_t>(ReadU8());
    v |= static_cast<uint16_t>(ReadU8()) << 8;
    return v;
}
uint32_t Deserializer::ReadU32() {
    Require(*this, 4);
    uint32_t v = 0;
    v |= static_cast<uint32_t>(ReadU8());
    v |= static_cast<uint32_t>(ReadU8()) << 8;
    v |= static_cast<uint32_t>(ReadU8()) << 16;
    v |= static_cast<uint32_t>(ReadU8()) << 24;
    return v;
}
uint64_t Deserializer::ReadU64() {
    Require(*this, 8);
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<uint64_t>(ReadU8()) << (i * 8);
    }
    return v;
}
float Deserializer::ReadF32() {
    const uint32_t bits = ReadU32();
    float v = 0.0f;
    std::memcpy(&v, &bits, sizeof(v));
    return v;
}
double Deserializer::ReadF64() {
    const uint64_t bits = ReadU64();
    double v = 0.0;
    std::memcpy(&v, &bits, sizeof(v));
    return v;
}
void Deserializer::ReadBytes(void* out, size_t n) {
    Require(*this, n);
    if (Remaining() < n) {
        return;
    }
    std::memcpy(out, data_ + pos_, n);
    pos_ += n;
}
std::string Deserializer::ReadString() {
    const uint32_t len = ReadU32();
    if (len > Remaining()) {
        error_ = true;
        return {};
    }
    std::string s(reinterpret_cast<const char*>(data_ + pos_), len);
    pos_ += len;
    return s;
}

uint32_t Crc32(const uint8_t* data, size_t n) {
    static uint32_t table[256] = {};
    static bool initialized = false;
    if (!initialized) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        initialized = true;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

} // namespace writeover