#pragma once
// Explicit little-endian field-by-field serialization. No reinterpret_cast on
// structs, no compiler-padding dependence (M-002 closure, save contract).

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace writeover {

class Serializer {
public:
    explicit Serializer(std::vector<uint8_t>& out) : out_(&out) {}

    void WriteU8(uint8_t v);
    void WriteU16(uint16_t v);
    void WriteU32(uint32_t v);
    void WriteU64(uint64_t v);
    void WriteI32(int32_t v) { WriteU32(static_cast<uint32_t>(v)); }
    void WriteF32(float v);
    void WriteF64(double v);
    void WriteBytes(const void* data, size_t n);
    void WriteString(std::string_view s);

    size_t Size() const { return out_->size(); }

private:
    std::vector<uint8_t>* out_;
};

class Deserializer {
public:
    Deserializer(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    bool HasError() const { return error_; }
    bool AtEnd() const { return pos_ >= size_; }
    void MarkError() { error_ = true; }

    uint8_t ReadU8();
    uint16_t ReadU16();
    uint32_t ReadU32();
    uint64_t ReadU64();
    int32_t ReadI32() { return static_cast<int32_t>(ReadU32()); }
    float ReadF32();
    double ReadF64();
    void ReadBytes(void* out, size_t n);
    std::string ReadString();

    size_t Position() const { return pos_; }
    size_t Remaining() const { return size_ - pos_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_ = 0;
    bool error_ = false;
};

// CRC32 (IEEE 802.3, reflected), used for save section integrity.
uint32_t Crc32(const uint8_t* data, size_t n);

// Convenience template used by strong-id serialization.
template <typename IdT>
inline void WriteId(Serializer& s, IdT id) {
    s.WriteU64(id.GetValue());
}
template <typename IdT>
inline IdT ReadId(Deserializer& d) {
    return IdT::New(d.ReadU64());
}

} // namespace writeover