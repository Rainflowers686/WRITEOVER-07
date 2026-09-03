#pragma once
// Small math helpers. Single math module; a second Vec/math library is a
// forbidden pattern.

#include "writeover/common/types.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace writeover {

inline Vec3 operator+(const Vec3& a, const Vec3& b) {
    return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}
inline Vec3 operator-(const Vec3& a, const Vec3& b) {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}
inline Vec3 operator*(const Vec3& a, float s) {
    return Vec3{a.x * s, a.y * s, a.z * s};
}
inline Vec2 operator+(const Vec2& a, const Vec2& b) {
    return Vec2{a.x + b.x, a.y + b.y};
}
inline Vec2 operator*(const Vec2& a, float s) {
    return Vec2{a.x * s, a.y * s};
}

inline float LengthSq(const Vec3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
inline float Length(const Vec3& v) { return std::sqrt(LengthSq(v)); }
inline float LengthSq(const Vec2& v) { return v.x * v.x + v.y * v.y; }
inline float Length(const Vec2& v) { return std::sqrt(LengthSq(v)); }

inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 Normalize(const Vec3& v) {
    const float len = Length(v);
    if (len <= kEpsVelocity) {
        return Vec3{1.0f, 0.0f, 0.0f};
    }
    return v * (1.0f / len);
}

inline float Clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}
inline int32_t ClampI(int32_t v, int32_t lo, int32_t hi) {
    return std::max(lo, std::min(v, hi));
}
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

inline bool NearEq(float a, float b, float eps) {
    return std::fabs(a - b) <= eps;
}

constexpr float kPi = 3.14159265358979323846f;

inline float DegToRad(float deg) { return deg * (kPi / 180.0f); }
inline float RadToDeg(float rad) { return rad * (180.0f / kPi); }

} // namespace writeover