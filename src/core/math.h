#pragma once
// Small 2D/3D vector math, RNG and noise helpers shared by the renderer, world and examples.

#include <cmath>
#include <cstdint>

constexpr float kPi = 3.14159265358979f;
constexpr float kTau = 6.28318530717959f;

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2 operator-() const { return {-x, -y}; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
};

inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline float cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
inline float length(Vec2 v) { return std::sqrt(dot(v, v)); }
inline Vec2 normalize(Vec2 v) { float l = length(v); return l > 1e-6f ? v / l : Vec2(1, 0); }
inline Vec2 perp(Vec2 v) { return {-v.y, v.x}; }
inline Vec2 fromAngle(float a) { return {std::cos(a), std::sin(a)}; }
inline Vec2 rotate(Vec2 v, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}
inline Vec2 reflect(Vec2 d, Vec2 n) { return d - n * (2.0f * dot(d, n)); }
inline Vec2 lerp(Vec2 a, Vec2 b, float t) { return a + (b - a) * t; }

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator*(Vec3 o) const { return {x * o.x, y * o.y, z * o.z}; }
    Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
};
inline Vec3 lerp(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float saturate(float v) { return clampf(v, 0.0f, 1.0f); }
inline float mixf(float a, float b, float t) { return a + (b - a) * t; }
inline float smoothstepf(float e0, float e1, float x) {
    float t = saturate((x - e0) / (e1 - e0));
    return t * t * (3.0f - 2.0f * t);
}

// Hue in [0,1) to a fully saturated RGB colour.
inline Vec3 hueToRgb(float h) {
    h = h - std::floor(h);
    float r = std::fabs(h * 6.0f - 3.0f) - 1.0f;
    float g = 2.0f - std::fabs(h * 6.0f - 2.0f);
    float b = 2.0f - std::fabs(h * 6.0f - 4.0f);
    return {saturate(r), saturate(g), saturate(b)};
}

// xorshift32: deterministic, fast, good enough for visuals.
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed = 0x9E3779B9u) : s(seed ? seed : 1u) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uniform() { return (next() >> 8) * (1.0f / 16777216.0f); }  // [0,1)
    float range(float lo, float hi) { return lo + (hi - lo) * uniform(); }
};

inline float hash1(int n) {
    uint32_t h = static_cast<uint32_t>(n) * 374761393u + 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return ((h ^ (h >> 16)) & 0xFFFFFF) * (1.0f / 16777215.0f);
}

inline float hash2(int x, int y, int seed = 0) { return hash1(x * 73856093 ^ y * 19349663 ^ seed * 83492791); }

// Smooth 1D value noise in [0,1].
inline float noise1(float x) {
    int i = static_cast<int>(std::floor(x));
    float f = x - static_cast<float>(i);
    float u = f * f * (3.0f - 2.0f * f);
    return mixf(hash1(i), hash1(i + 1), u);
}

// Smooth 2D value noise in [0,1], tileable with the given period.
inline float noise2(float x, float y, int period, int seed = 0) {
    int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
    float fx = x - static_cast<float>(ix), fy = y - static_cast<float>(iy);
    float ux = fx * fx * (3.0f - 2.0f * fx), uy = fy * fy * (3.0f - 2.0f * fy);
    auto w = [period](int v) { return ((v % period) + period) % period; };
    float a = hash2(w(ix), w(iy), seed), b = hash2(w(ix + 1), w(iy), seed);
    float c = hash2(w(ix), w(iy + 1), seed), d = hash2(w(ix + 1), w(iy + 1), seed);
    return mixf(mixf(a, b, ux), mixf(c, d, ux), uy);
}

// Fractal (fBm) tileable noise in [0,1].
inline float fbm2(float x, float y, int period, int octaves, int seed = 0) {
    float sum = 0, amp = 0.5f, norm = 0;
    for (int o = 0; o < octaves; ++o) {
        sum += amp * noise2(x, y, period, seed + o);
        norm += amp;
        x *= 2; y *= 2; period *= 2; amp *= 0.5f;
    }
    return sum / norm;
}
