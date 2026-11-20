#include "materials.h"

#include <vector>

#include "../core/math.h"

namespace {

constexpr int kSize = 512;  // texture resolution (power of two, tileable)

struct Texel {
    float h;     // height 0..1
    Vec3 color;  // linear albedo
};

using Generator = Texel (*)(int x, int y);

// Rounded bevel profile: 0 in the mortar gap, 1 on the face, smooth in between.
float bevel(float edgeDist, float gap, float width) { return smoothstepf(gap, gap + width, edgeDist); }

Texel tiles(int x, int y) {
    const int cell = 128;
    int cx = x / cell, cy = y / cell;
    float lx = static_cast<float>(x % cell), ly = static_cast<float>(y % cell);
    float edge = std::fmin(std::fmin(lx, ly), std::fmin(cell - 1 - lx, cell - 1 - ly));
    float grain = fbm2(x / 32.0f, y / 32.0f, kSize / 32, 4, 11);
    float face = bevel(edge, 3.0f, 9.0f);
    float tint = hash2(cx, cy, 3);
    Vec3 stone = Vec3(0.42f, 0.40f, 0.37f) * (0.75f + 0.35f * tint) * (0.8f + 0.4f * grain);
    Vec3 mortar = Vec3(0.12f, 0.11f, 0.10f) * (0.8f + 0.4f * grain);
    float h = face * (0.82f + 0.18f * grain) + (1 - face) * 0.1f * grain;
    return {h, lerp(mortar, stone, face)};
}

Texel bricks(int x, int y) {
    const int bw = 128, bh = 64;
    int row = y / bh;
    int xo = x + ((row & 1) ? bw / 2 : 0);
    int col = (xo / bw) % (kSize / bw);
    float lx = static_cast<float>(xo % bw), ly = static_cast<float>(y % bh);
    float edge = std::fmin(std::fmin(lx, ly), std::fmin(bw - 1 - lx, bh - 1 - ly));
    float grain = fbm2(x / 16.0f, y / 16.0f, kSize / 16, 4, 21);
    float chips = fbm2(x / 64.0f, y / 64.0f, kSize / 64, 3, 22);
    float face = bevel(edge, 4.0f, 7.0f) * smoothstepf(0.18f, 0.3f, chips + 0.15f);
    float tint = hash2(col, row, 5);
    Vec3 brick = Vec3(0.45f + 0.15f * tint, 0.16f + 0.05f * tint, 0.10f) * (0.75f + 0.45f * grain);
    Vec3 mortar = Vec3(0.30f, 0.28f, 0.25f) * (0.7f + 0.4f * grain);
    float h = face * (0.8f + 0.2f * grain) + (1 - face) * 0.15f * grain;
    return {h, lerp(mortar, brick, face)};
}

Texel metal(int x, int y) {
    const int panel = 256;
    float lx = static_cast<float>(x % panel), ly = static_cast<float>(y % panel);
    float edge = std::fmin(std::fmin(lx, ly), std::fmin(panel - 1 - lx, panel - 1 - ly));
    float face = bevel(edge, 2.0f, 4.0f);
    // Rivets near the panel corners.
    float rivet = 0;
    const float rp[2] = {14.0f, panel - 15.0f};
    for (float rx : rp)
        for (float ry : rp) {
            float d = length(Vec2(lx - rx, ly - ry));
            rivet = std::fmax(rivet, std::sqrt(saturate(1 - (d / 6.0f) * (d / 6.0f))));
        }
    // Diamond tread plate pattern.
    float u = (x + y) / 24.0f, v = (x - y) / 24.0f;
    float fu = u - std::floor(u) - 0.5f, fv = v - std::floor(v) - 0.5f;
    float tread = saturate(1 - length(Vec2(fu * 3.0f, fv * 1.0f)) * 1.6f);
    float brushed = noise2(x / 64.0f, y / 2.0f, kSize / 64, 31) * 0.6f + fbm2(x / 32.0f, y / 32.0f, kSize / 32, 3, 32) * 0.4f;
    float h = face * (0.55f + 0.25f * tread) + 0.45f * rivet;
    Vec3 base = Vec3(0.30f, 0.32f, 0.34f) * (0.75f + 0.35f * brushed);
    Vec3 seam = Vec3(0.06f, 0.06f, 0.07f);
    return {saturate(h), lerp(seam, base * (1.0f + 0.25f * tread + 0.3f * rivet), face)};
}

Texel cobble(int x, int y) {
    // Jittered-grid Voronoi stones (F2 - F1 gives the gap between stones).
    const int cells = 8;
    const float cs = static_cast<float>(kSize) / cells;
    float px = x / cs, py = y / cs;
    int ix = static_cast<int>(px), iy = static_cast<int>(py);
    float f1 = 1e9f, f2 = 1e9f;
    int id = 0;
    for (int oy = -1; oy <= 1; ++oy)
        for (int ox = -1; ox <= 1; ++ox) {
            int gx = ix + ox, gy = iy + oy;
            int wx = (gx + cells) % cells, wy = (gy + cells) % cells;
            Vec2 c(gx + 0.15f + 0.7f * hash2(wx, wy, 41), gy + 0.15f + 0.7f * hash2(wx, wy, 42));
            float d = length(Vec2(px, py) - c);
            if (d < f1) { f2 = f1; f1 = d; id = wy * cells + wx; }
            else if (d < f2) { f2 = d; }
        }
    float gap = f2 - f1;
    float face = smoothstepf(0.04f, 0.22f, gap);
    float grain = fbm2(x / 32.0f, y / 32.0f, kSize / 32, 4, 43);
    float tint = hash1(id * 7 + 1);
    float dome = face * std::sqrt(saturate(gap * 2.2f));
    Vec3 stone = Vec3(0.30f + 0.08f * tint, 0.29f + 0.05f * tint, 0.27f) * (0.75f + 0.4f * grain);
    Vec3 dirt = Vec3(0.10f, 0.09f, 0.06f) * (0.7f + 0.6f * grain);
    return {dome * (0.85f + 0.15f * grain), lerp(dirt, stone, face)};
}

GLuint upload(const std::vector<unsigned char>& rgb) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, kSize, kSize, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
    return tex;
}

unsigned char toByte(float v) { return static_cast<unsigned char>(saturate(v) * 255.0f + 0.5f); }

Material build(Generator gen, float worldSize, float bumpDepth, float specular) {
    std::vector<Texel> texels(kSize * kSize);
    for (int y = 0; y < kSize; ++y)
        for (int x = 0; x < kSize; ++x) texels[y * kSize + x] = gen(x, y);

    std::vector<unsigned char> albedo(kSize * kSize * 3), normal(kSize * kSize * 3);
    auto h = [&](int x, int y) { return texels[((y + kSize) % kSize) * kSize + (x + kSize) % kSize].h; };
    for (int y = 0; y < kSize; ++y)
        for (int x = 0; x < kSize; ++x) {
            int i = (y * kSize + x) * 3;
            const Vec3& c = texels[y * kSize + x].color;
            albedo[i + 0] = toByte(c.x);
            albedo[i + 1] = toByte(c.y);
            albedo[i + 2] = toByte(c.z);
            float dx = (h(x + 1, y) - h(x - 1, y)) * bumpDepth;
            float dy = (h(x, y + 1) - h(x, y - 1)) * bumpDepth;
            float inv = 1.0f / std::sqrt(dx * dx + dy * dy + 1.0f);
            normal[i + 0] = toByte(-dx * inv * 0.5f + 0.5f);
            normal[i + 1] = toByte(-dy * inv * 0.5f + 0.5f);
            normal[i + 2] = toByte(inv * 0.5f + 0.5f);
        }
    Material m;
    m.albedo = upload(albedo);
    m.normal = upload(normal);
    m.worldSize = worldSize;
    m.specular = specular;
    return m;
}

}  // namespace

void Materials::init() {
    mats_[static_cast<int>(FloorType::Tiles)] = build(tiles, 12.0f, 6.0f, 0.25f);
    mats_[static_cast<int>(FloorType::Bricks)] = build(bricks, 10.0f, 7.0f, 0.08f);
    mats_[static_cast<int>(FloorType::Metal)] = build(metal, 10.0f, 5.0f, 0.9f);
    mats_[static_cast<int>(FloorType::Cobble)] = build(cobble, 14.0f, 6.0f, 0.2f);
}

void Materials::destroy() {
    for (Material& m : mats_) {
        if (m.albedo) glDeleteTextures(1, &m.albedo);
        if (m.normal) glDeleteTextures(1, &m.normal);
        m = Material();
    }
}

const char* floorName(FloorType t) {
    switch (t) {
        case FloorType::Tiles: return "Stone tiles";
        case FloorType::Bricks: return "Bricks";
        case FloorType::Metal: return "Metal plates";
        case FloorType::Cobble: return "Cobblestone";
        default: return "?";
    }
}
