#include "renderer.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <initializer_list>

// ============================================================================ shaders

namespace {

const char* kVertex = R"(#version 120
varying vec2 vWorld;
varying vec4 vTex0;
varying vec4 vTex1;
void main() {
    vWorld = gl_Vertex.xy;
    vTex0 = gl_MultiTexCoord0;
    vTex1 = gl_MultiTexCoord1;
    gl_FrontColor = gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
)";

// Shared by point and line lights: distance falloff and bump/specular shading.
const char* kLightCommon = R"(#version 120
uniform sampler2D uNormal;
uniform vec3 uColor;      // colour * intensity * flicker level
uniform float uRadius;
uniform float uHeight;
uniform float uBump;
uniform float uSpec;
uniform float uMatScale;
varying vec2 vWorld;

float attenuation(float r) {
    float q = r / uRadius;
    float win = clamp(1.0 - q * q, 0.0, 1.0);
    return win * win / (1.0 + 6.0 * q * q);
}

// d = receiver - light (world units). Returns rgb diffuse, a = specular.
vec4 shade(vec2 d, float energy) {
    vec3 n = texture2D(uNormal, vWorld / uMatScale).xyz * 2.0 - 1.0;
    n = normalize(vec3(n.xy * uBump, max(n.z, 0.05)));
    vec3 l = normalize(vec3(-d, uHeight));
    // Normalised so a flat floor gets exactly the unbumped falloff.
    float diffuse = clamp(dot(n, l) / max(l.z, 0.2), 0.0, 3.0);
    vec3 h = normalize(l + vec3(0.0, 0.0, 1.0));
    float spec = pow(max(dot(n, h), 0.0), 48.0) * uSpec;
    float peak = max(max(uColor.r, uColor.g), uColor.b);
    return vec4(uColor * (energy * diffuse), energy * spec * peak);
}
)";

const char* kPointLight = R"(
uniform sampler2D uShadow;     // polar distance map, one row per light
uniform sampler2D uShadowMin;  // min-pyramid of the distance map, uMinLevels rows per light
uniform vec2 uPos;
uniform float uSize;           // light source radius (penumbra)
uniform float uRow;            // shadow row v-coordinate, < 0 = no shadows
uniform float uShadowIndex;    // shadow row index (for the min-pyramid)
uniform float uMinRows;        // total rows in uShadowMin
uniform float uMinLevels;      // pyramid levels per light
uniform float uShadowRes;      // angular bins per row
uniform int uSamples;       // area-light samples, 0 = hard shadows
uniform int uSteps;         // march steps per sample
uniform vec3 uCone;         // spotlight direction xy, z > 0.5 enables
uniform vec2 uConeCos;      // cos(outer), cos(inner)

const float TAU = 6.2831853;
const float RIM = 0.3;      // light bleeds this far into occluders: lit rims

float mapDist(float angle) { return texture2D(uShadow, vec2(angle / TAU + 0.5, uRow)).r; }

// Nearest occluder distance anywhere within +-radiusBins of the angle (level L covers 2^L - 1).
float windowMin(float angle, float radiusBins) {
    float level = clamp(ceil(log2(radiusBins + 1.0)), 0.0, uMinLevels - 1.0);
    float v = (uShadowIndex * uMinLevels + level + 0.5) / uMinRows;
    return texture2D(uShadowMin, vec2(angle / TAU + 0.5, v)).r;
}

// Interleaved gradient noise: decorrelates area-light samples between pixels.
float ign(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }

// Fraction of a disc light (radius uSize) visible from the receiver. Each sample is a point
// across the disc; the ray from it to the receiver is marched in the light's polar map and is
// blocked where it crosses an occluder front face. Blockers near the light widen the penumbra,
// blockers near the receiver keep it sharp (contact hardening).
float softVisibility(float angle, float r) {
    float jitter = ign(gl_FragCoord.xy) - 0.5;
    float stepJitter = ign(gl_FragCoord.yx + 17.0);
    float tMax = r - RIM;
    if (tMax <= 0.0) return 1.0;  // receiver within the rim of the light itself
    // The last march position is clamped to tMax, so every sample ray is tested right up to the
    // receiver and a blocker just in front of it can't be skipped.
    float stepLen = tMax / float(uSteps);
    // Early out: no occluder closer than tMax anywhere the samples can look -> fully lit.
    float tMin = stepLen * 0.5;
    float phiMax = min(uSize * (r - tMin) / (r * tMin), 1.2);
    if (windowMin(angle, phiMax / TAU * uShadowRes + 1.0) > tMax) return 1.0;
    float lit = 0.0;
    for (int k = 0; k < 32; k++) {
        if (k >= uSamples) break;
        float x = uSize * (((float(k) + 0.5 + jitter) / float(uSamples)) * 2.0 - 1.0);
        // Per-sample offset of the march positions turns step aliasing into fine noise.
        float offset = fract(stepJitter + float(k) * 0.618034) - 0.5;
        float blocked = 0.0;
        for (int j = 1; j <= 17; j++) {
            if (j > uSteps + 1) break;
            float t = min(stepLen * (float(j) + offset), tMax);
            float phi = clamp(x * (r - t) / (r * t), -1.2, 1.2);
            float d = mapDist(angle + phi);
            blocked = max(blocked, step(d, t) * step(t, d + stepLen * 2.0));
        }
        lit += 1.0 - blocked;
    }
    return lit / float(uSamples);
}

void main() {
    vec2 d = vWorld - uPos;
    float r = length(d);
    if (r >= uRadius) discard;
    float energy = attenuation(r);
    if (uCone.z > 0.5) {
        float c = dot(d / max(r, 1e-4), uCone.xy);
        energy *= smoothstep(uConeCos.x, uConeCos.y, c);
    }
    if (energy <= 0.0) discard;
    if (uRow >= 0.0) {
        float angle = atan(d.y, d.x);
        energy *= (uSamples > 0 && uSize > 0.0) ? softVisibility(angle, r)
                                                 : step(r, mapDist(angle) + RIM);
    }
    gl_FragColor = shade(d, energy);
}
)";

const char* kLineLight = R"(
uniform vec2 uA;
uniform vec2 uB;
void main() {
    vec2 ab = uB - uA;
    float t = clamp(dot(vWorld - uA, ab) / max(dot(ab, ab), 1e-6), 0.0, 1.0);
    vec2 d = vWorld - (uA + ab * t);
    float r = length(d);
    if (r >= uRadius) discard;
    gl_FragColor = shade(d, attenuation(r));
}
)";

const char* kFloor = R"(#version 120
uniform sampler2D uAlbedo;
uniform sampler2D uLight;
uniform float uMatScale;
uniform vec2 uScreen;
varying vec2 vWorld;
void main() {
    vec3 albedo = texture2D(uAlbedo, vWorld / uMatScale).rgb;
    vec4 L = texture2D(uLight, gl_FragCoord.xy / uScreen);
    vec3 specTint = L.rgb / max(max(L.r, max(L.g, L.b)), 1e-3);
    gl_FragColor = vec4(albedo * L.rgb + specTint * L.a, 1.0);
}
)";

// Tops of occluders: lit directly by every light (no floor bump), and occluded only when another
// occluder is closer to the light. vTex0 = (emissive glow, mirror, occluder id).
const char* kOccluder = R"(#version 120
uniform sampler2D uShadow;
uniform vec3 uAmbient;
uniform int uLightCount;
uniform vec4 uLightPos[32];    // x, y, radius, shadow row (< 0 = unshadowed)
uniform vec3 uLightColor[32];  // colour * intensity * level
uniform vec4 uLightCone[32];   // direction xy, cos(outer), cos(inner); cos(outer) < -1.5 = omni
varying vec2 vWorld;
varying vec4 vTex0;
const float TAU = 6.2831853;
void main() {
    vec3 light = uAmbient;
    for (int i = 0; i < 32; i++) {
        if (i >= uLightCount) break;
        vec2 d = vWorld - uLightPos[i].xy;
        float r = length(d);
        float q = r / uLightPos[i].z;
        if (q >= 1.0) continue;
        float win = 1.0 - q * q;
        float e = win * win / (1.0 + 6.0 * q * q);
        vec4 cone = uLightCone[i];
        if (cone.z > -1.5) e *= smoothstep(cone.z, cone.w, dot(d / max(r, 1e-4), cone.xy));
        if (uLightPos[i].w >= 0.0) {
            vec4 m = texture2D(uShadow, vec2(atan(d.y, d.x) / TAU + 0.5, uLightPos[i].w));
            if (abs(m.a - vTex0.z) > 0.5 && r > m.r + 0.05) e = 0.0;
        }
        light += uLightColor[i] * e;
    }
    vec3 c = gl_Color.rgb;
    vec3 col = c * light + c * vTex0.x;
    col += vTex0.y * (light * 0.35 + vec3(0.02, 0.03, 0.05));
    gl_FragColor = vec4(col, 1.0);
}
)";

// Capsule glow: vTex0 = (along, across) relative to the segment centre,
// vTex1 = (half length, width, intensity).
const char* kGlow = R"(#version 120
varying vec4 vTex0;
varying vec4 vTex1;
void main() {
    float dx = max(abs(vTex0.x) - vTex1.x, 0.0);
    float q = length(vec2(dx, vTex0.y)) / vTex1.y;
    float g = exp(-q * q * 3.0) + 0.12 * exp(-q * 1.5);
    g *= 1.0 - smoothstep(2.0, 3.0, q);
    gl_FragColor = vec4(gl_Color.rgb * (g * vTex1.z), 1.0);
}
)";

const char* kFullscreenVertex = R"(#version 120
varying vec2 vUv;
void main() {
    vUv = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
}
)";

// 4-tap box downsample with optional soft-knee bright pass (uThreshold < 0 disables).
const char* kDown = R"(#version 120
uniform sampler2D uSrc;
uniform vec2 uTexel;
uniform float uThreshold;
varying vec2 vUv;
void main() {
    vec3 c = (texture2D(uSrc, vUv + uTexel * vec2(-1.0, -1.0)).rgb + texture2D(uSrc, vUv + uTexel * vec2(1.0, -1.0)).rgb +
              texture2D(uSrc, vUv + uTexel * vec2(-1.0, 1.0)).rgb + texture2D(uSrc, vUv + uTexel * vec2(1.0, 1.0)).rgb) * 0.25;
    if (uThreshold >= 0.0) {
        float br = max(c.r, max(c.g, c.b));
        float knee = 0.5 * uThreshold + 1e-4;
        float soft = clamp(br - uThreshold + knee, 0.0, 2.0 * knee);
        soft = soft * soft / (4.0 * knee);
        c *= max(soft, br - uThreshold) / max(br, 1e-4);
    }
    gl_FragColor = vec4(c, 1.0);
}
)";

// 9-tap Gaussian using 5 bilinear fetches.
const char* kBlur = R"(#version 120
uniform sampler2D uSrc;
uniform vec2 uDir;
varying vec2 vUv;
void main() {
    vec3 c = texture2D(uSrc, vUv).rgb * 0.2270270270;
    c += (texture2D(uSrc, vUv + uDir * 1.3846153846).rgb + texture2D(uSrc, vUv - uDir * 1.3846153846).rgb) * 0.3162162162;
    c += (texture2D(uSrc, vUv + uDir * 3.2307692308).rgb + texture2D(uSrc, vUv - uDir * 3.2307692308).rgb) * 0.0702702703;
    gl_FragColor = vec4(c, 1.0);
}
)";

const char* kComposite = R"(#version 120
uniform sampler2D uScene;
uniform sampler2D uBloomA;
uniform sampler2D uBloomB;
uniform float uExposure;
uniform float uBloom;
uniform float uVignette;
varying vec2 vUv;
vec3 aces(vec3 x) { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }
void main() {
    vec3 c = texture2D(uScene, vUv).rgb;
    c += (texture2D(uBloomA, vUv).rgb * 0.6 + texture2D(uBloomB, vUv).rgb * 0.8) * uBloom;
    c = aces(c * uExposure);
    vec2 p = vUv - 0.5;
    c *= 1.0 - uVignette * dot(p, p) * 1.1;
    c = pow(c, vec3(1.0 / 2.2));
    float n = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    gl_FragColor = vec4(c + (n - 0.5) / 255.0, 1.0);
}
)";

void fullscreenQuad() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(-1, 1);
    glEnd();
}

void bindTexture(int unit, GLuint tex) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex);
}

void worldQuad(Vec2 lo, Vec2 hi) {
    glBegin(GL_QUADS);
    glVertex2f(lo.x, lo.y);
    glVertex2f(hi.x, lo.y);
    glVertex2f(hi.x, hi.y);
    glVertex2f(lo.x, hi.y);
    glEnd();
}

// One capsule-shaped glow quad (call between glBegin(GL_QUADS)/glEnd()).
void glowQuad(Vec2 a, Vec2 b, Vec3 color, float width, float intensity) {
    Vec2 d = b - a;
    float len = length(d);
    Vec2 t = len > 1e-5f ? d / len : Vec2(1, 0);
    Vec2 n = perp(t);
    float half = len * 0.5f, ext = width * 3.0f;
    Vec2 c = (a + b) * 0.5f;
    glColor3f(color.x, color.y, color.z);
    glMultiTexCoord4f(GL_TEXTURE1, half, width, intensity, 0);
    const float s[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    for (const auto& k : s) {
        float along = k[0] * (half + ext), across = k[1] * ext;
        glMultiTexCoord2f(GL_TEXTURE0, along, across);
        Vec2 p = c + t * along + n * across;
        glVertex2f(p.x, p.y);
    }
}

std::string join(const char* a, const char* b) { return std::string(a) + b; }

}  // namespace

// ============================================================================ View

View View::fit(int fbW, int fbH, Vec2 ws) {
    View v;
    v.fbW = std::max(fbW, 1);
    v.fbH = std::max(fbH, 1);
    float aspect = static_cast<float>(v.fbW) / static_cast<float>(v.fbH);
    Vec2 size = aspect > ws.x / ws.y ? Vec2(ws.y * aspect, ws.y) : Vec2(ws.x, ws.x / aspect);
    Vec2 c = ws * 0.5f;
    v.min = c - size * 0.5f;
    v.max = c + size * 0.5f;
    return v;
}

void View::apply() const {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(min.x, max.x, min.y, max.y, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

Vec2 View::toWorld(float px, float py) const {
    return {min.x + (max.x - min.x) * px / fbW, max.y - (max.y - min.y) * py / fbH};
}

// ============================================================================ Renderer

bool Renderer::init() {
    bool ok = true;
    ok &= light_.load("point light", kVertex, join(kLightCommon, kPointLight).c_str());
    ok &= lineLight_.load("line light", kVertex, join(kLightCommon, kLineLight).c_str());
    ok &= floor_.load("floor", kVertex, kFloor);
    ok &= occluder_.load("occluder", kVertex, kOccluder);
    ok &= glow_.load("glow", kVertex, kGlow);
    ok &= down_.load("downsample", kFullscreenVertex, kDown);
    ok &= blur_.load("blur", kFullscreenVertex, kBlur);
    ok &= composite_.load("composite", kFullscreenVertex, kComposite);
    if (!ok) return false;

    for (int i = 0; i < kShadowRes; ++i) {
        float a = -kPi + (i + 0.5f) * kTau / kShadowRes;  // bin centre angle, matches the shader
        cos_[i] = std::cos(a);
        sin_[i] = std::sin(a);
    }
    auto makeMap = [](GLuint& tex, GLint internalFormat, GLenum format, int rows, const float* data) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // angles wrap around
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, kShadowRes, rows, 0, format, GL_FLOAT, data);
    };
    // Two channels per bin: nearest distance and the id (index + 1) of the occluder hit there.
    shadowData_.assign(static_cast<size_t>(kShadowRes) * kMaxShadowLights * 2, 0.0f);
    shadowMin_.assign(static_cast<size_t>(kShadowRes) * kMaxShadowLights * kMinLevels, 0.0f);
    makeMap(shadowTex_, GL_LUMINANCE_ALPHA32F_ARB, GL_LUMINANCE_ALPHA, kMaxShadowLights, shadowData_.data());
    makeMap(shadowMinTex_, GL_LUMINANCE32F_ARB, GL_LUMINANCE, kMaxShadowLights * kMinLevels, shadowMin_.data());

    materials_.init();
    return true;
}

void Renderer::destroy() {
    for (Shader* s : {&light_, &lineLight_, &floor_, &occluder_, &glow_, &down_, &blur_, &composite_}) s->destroy();
    lightBuf_.destroy();
    sceneBuf_.destroy();
    for (RenderTarget& t : bloom_) t.destroy();
    for (GLuint* t : {&shadowTex_, &shadowMinTex_}) {
        if (*t) glDeleteTextures(1, t);
        *t = 0;
    }
    materials_.destroy();
}

void Renderer::ensureTargets(int w, int h, int lightW, int lightH) {
    if (w == fbW_ && h == fbH_ && lightW == lightW_ && lightH == lightH_) return;
    fbW_ = w;
    fbH_ = h;
    lightW_ = lightW;
    lightH_ = lightH;
    lightBuf_.create(lightW, lightH, GL_RGBA16F_ARB);
    sceneBuf_.create(w, h, GL_RGBA16F_ARB);
    const int div[5] = {2, 4, 4, 8, 8};
    for (int i = 0; i < 5; ++i)
        bloom_[i].create(std::max(1, w / div[i]), std::max(1, h / div[i]), GL_RGBA16F_ARB);
}

// Rasterises every occluder edge into each light's polar map: for every angular bin the edge
// spans, store the distance at which the bin's ray meets the edge (keeping the nearest).
void Renderer::buildShadowMaps(const World& world, const std::vector<const Light*>& lights) {
    int row = 0;
    shadowRow_.assign(lights.size(), -1);
    for (size_t li = 0; li < lights.size() && row < kMaxShadowLights; ++li) {
        const Light& l = *lights[li];
        if (!l.shadows) continue;
        shadowRow_[li] = row;
        float* bins = &shadowData_[static_cast<size_t>(row) * kShadowRes * 2];  // (distance, id) pairs
        for (int k = 0; k < kShadowRes; ++k) {
            bins[2 * k] = 1e6f;
            bins[2 * k + 1] = 0.0f;
        }
        const float r2 = l.radius * l.radius;
        for (size_t oi = 0; oi < world.occluders.size(); ++oi) {
            const Occluder& o = world.occluders[oi];
            const float id = static_cast<float>(oi + 1);
            const size_t n = o.pts.size();
            for (size_t i = 0; i < n; ++i) {
                Vec2 a = o.pts[i] - l.pos, b = o.pts[(i + 1) % n] - l.pos;
                Vec2 e = b - a;
                // Skip edges entirely outside the light's reach.
                float t = clampf(-dot(a, e) / std::max(dot(e, e), 1e-9f), 0.0f, 1.0f);
                Vec2 closest = a + e * t;
                if (dot(closest, closest) > r2) continue;

                float ta = std::atan2(a.y, a.x), span = std::atan2(b.y, b.x) - ta;
                if (span > kPi) span -= kTau;
                if (span < -kPi) span += kTau;
                if (span < 0) { ta += span; span = -span; }
                float f0 = (ta + kPi) / kTau * kShadowRes - 0.5f;
                int k0 = static_cast<int>(std::ceil(f0));
                int k1 = static_cast<int>(std::floor(f0 + span / kTau * kShadowRes));
                float ca = cross(a, e);
                for (int k = k0; k <= k1; ++k) {
                    int bin = ((k % kShadowRes) + kShadowRes) % kShadowRes;
                    float denom = cos_[bin] * e.y - sin_[bin] * e.x;  // cross(dir, e)
                    if (std::fabs(denom) < 1e-9f) continue;
                    float s = ca / denom;
                    if (s > 0 && s < bins[2 * bin]) {
                        bins[2 * bin] = s;
                        bins[2 * bin + 1] = id;
                    }
                }
            }
        }
        // Min-pyramid: level L holds the minimum over a window of radius 2^L - 1 bins.
        float* m = &shadowMin_[static_cast<size_t>(row) * kMinLevels * kShadowRes];
        for (int k = 0; k < kShadowRes; ++k) m[k] = bins[2 * k];
        for (int L = 1; L < kMinLevels; ++L) {
            const float* prev = m + (L - 1) * kShadowRes;
            float* cur = m + L * kShadowRes;
            const int off = 1 << (L - 1), mask = kShadowRes - 1;
            for (int k = 0; k < kShadowRes; ++k)
                cur[k] = std::min(prev[k], std::min(prev[(k - off) & mask], prev[(k + off) & mask]));
        }
        ++row;
    }
    if (row > 0) {
        glBindTexture(GL_TEXTURE_2D, shadowTex_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kShadowRes, row, GL_LUMINANCE_ALPHA, GL_FLOAT, shadowData_.data());
        glBindTexture(GL_TEXTURE_2D, shadowMinTex_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kShadowRes, row * kMinLevels, GL_LUMINANCE, GL_FLOAT, shadowMin_.data());
    }
}

void Renderer::lightPass(const World& world, const View& view, const RenderSettings& s,
                         const std::vector<const Light*>& lights) {
    const Material& mat = materials_.get(world.floor);
    lightBuf_.bind();
    view.apply();
    Vec3 amb = world.ambient * s.ambient + world.flash;
    glClearColor(amb.x, amb.y, amb.z, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    static const int kSamples[3] = {6, 12, 20};
    static const int kSteps[3] = {5, 8, 12};
    const int q = std::max(0, std::min(2, s.penumbraQuality));

    bindTexture(0, shadowTex_);
    bindTexture(1, mat.normal);
    bindTexture(2, shadowMinTex_);
    light_.use();
    light_.set("uShadow", 0);
    light_.set("uNormal", 1);
    light_.set("uShadowMin", 2);
    light_.set("uMinRows", static_cast<float>(kMaxShadowLights * kMinLevels));
    light_.set("uMinLevels", static_cast<float>(kMinLevels));
    light_.set("uShadowRes", static_cast<float>(kShadowRes));
    light_.set("uBump", s.bump);
    light_.set("uSpec", mat.specular * s.specular);
    light_.set("uMatScale", mat.worldSize);
    light_.set("uSamples", s.softShadows ? kSamples[q] : 0);
    light_.set("uSteps", kSteps[q]);
    for (size_t i = 0; i < lights.size(); ++i) {
        const Light& l = *lights[i];
        float e = l.intensity * l.level;
        int row = s.shadows ? shadowRow_[i] : -1;
        light_.set("uPos", l.pos.x, l.pos.y);
        light_.set("uColor", l.color.x * e, l.color.y * e, l.color.z * e);
        light_.set("uRadius", l.radius);
        light_.set("uHeight", l.height);
        light_.set("uSize", l.size * s.penumbraScale);
        light_.set("uRow", row < 0 ? -1.0f : (row + 0.5f) / kMaxShadowLights);
        light_.set("uShadowIndex", static_cast<float>(row));
        Vec2 dir = fromAngle(l.dir);
        light_.set("uCone", dir.x, dir.y, l.cone ? 1.0f : 0.0f);
        light_.set("uConeCos", std::cos(l.coneAngle + l.coneSoftness), std::cos(std::max(0.0f, l.coneAngle - l.coneSoftness)));
        worldQuad(l.pos - Vec2(l.radius, l.radius), l.pos + Vec2(l.radius, l.radius));
    }

    lineLight_.use();
    lineLight_.set("uNormal", 1);
    lineLight_.set("uBump", s.bump);
    lineLight_.set("uSpec", mat.specular * s.specular);
    lineLight_.set("uMatScale", mat.worldSize);
    for (const LineLight& ll : world.lineLights) {
        lineLight_.set("uA", ll.a.x, ll.a.y);
        lineLight_.set("uB", ll.b.x, ll.b.y);
        lineLight_.set("uColor", ll.color.x * ll.intensity, ll.color.y * ll.intensity, ll.color.z * ll.intensity);
        lineLight_.set("uRadius", ll.radius);
        lineLight_.set("uHeight", 1.5f);
        Vec2 lo(std::min(ll.a.x, ll.b.x) - ll.radius, std::min(ll.a.y, ll.b.y) - ll.radius);
        Vec2 hi(std::max(ll.a.x, ll.b.x) + ll.radius, std::max(ll.a.y, ll.b.y) + ll.radius);
        worldQuad(lo, hi);
    }
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
}

void Renderer::scenePass(const World& world, const View& view, const RenderSettings& s,
                         const std::vector<const Light*>& lights) {
    const Material& mat = materials_.get(world.floor);
    sceneBuf_.bind();
    view.apply();

    bindTexture(0, mat.albedo);
    bindTexture(1, lightBuf_.tex);
    floor_.use();
    floor_.set("uAlbedo", 0);
    floor_.set("uLight", 1);
    floor_.set("uMatScale", mat.worldSize);
    floor_.set("uScreen", static_cast<float>(sceneBuf_.w), static_cast<float>(sceneBuf_.h));
    worldQuad(view.min, view.max);

    // Per-light uniforms for occluder tops (first 32 lights).
    float pos[32 * 4], color[32 * 3], cone[32 * 4];
    int n = 0;
    for (size_t i = 0; i < lights.size() && n < 32; ++i, ++n) {
        const Light& l = *lights[i];
        float e = l.intensity * l.level;
        int row = s.shadows ? shadowRow_[i] : -1;
        Vec2 dir = fromAngle(l.dir);
        const float p[4] = {l.pos.x, l.pos.y, l.radius, row < 0 ? -1.0f : (row + 0.5f) / kMaxShadowLights};
        const float c[4] = {dir.x, dir.y, l.cone ? std::cos(l.coneAngle + l.coneSoftness) : -2.0f,
                            std::cos(std::max(0.0f, l.coneAngle - l.coneSoftness))};
        std::copy(p, p + 4, pos + n * 4);
        std::copy(c, c + 4, cone + n * 4);
        color[n * 3 + 0] = l.color.x * e;
        color[n * 3 + 1] = l.color.y * e;
        color[n * 3 + 2] = l.color.z * e;
    }
    Vec3 amb = (world.ambient * s.ambient + world.flash) * 1.5f;
    bindTexture(0, shadowTex_);
    occluder_.use();
    occluder_.set("uShadow", 0);
    occluder_.set("uAmbient", amb.x, amb.y, amb.z);
    occluder_.set("uLightCount", n);
    if (n > 0) {
        occluder_.set4v("uLightPos", pos, n);
        occluder_.set3v("uLightColor", color, n);
        occluder_.set4v("uLightCone", cone, n);
    }
    for (size_t oi = 0; oi < world.occluders.size(); ++oi) {
        const Occluder& o = world.occluders[oi];
        glMultiTexCoord3f(GL_TEXTURE0, o.glow, o.mirror ? 1.0f : 0.0f, static_cast<float>(oi + 1));
        glColor3f(o.color.x, o.color.y, o.color.z);
        glBegin(GL_POLYGON);
        for (const Vec2& p : o.pts) glVertex2f(p.x, p.y);
        glEnd();
        // Lighter outline so shapes stay readable in the dark.
        glColor3f(std::min(1.0f, o.color.x * 2.2f + 0.05f), std::min(1.0f, o.color.y * 2.2f + 0.05f),
                  std::min(1.0f, o.color.z * 2.2f + 0.05f));
        glBegin(GL_LINE_LOOP);
        for (const Vec2& p : o.pts) glVertex2f(p.x, p.y);
        glEnd();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glow_.use();
    glBegin(GL_QUADS);
    for (const Light* l : lights) {
        if (!l->bulb) continue;
        float e = l->intensity * l->level;
        glowQuad(l->pos, l->pos, l->color, 0.25f + 0.25f * l->size, e * 2.5f);
        glowQuad(l->pos, l->pos, Vec3(1, 1, 1), 0.12f, e * 2.0f);
    }
    for (const GlowLine& g : world.glow) glowQuad(g.a, g.b, g.color, g.width, g.intensity);
    glEnd();
    glDisable(GL_BLEND);
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
}

void Renderer::bloomPass(const RenderSettings& s) {
    down_.use();
    down_.set("uSrc", 0);
    glActiveTexture(GL_TEXTURE0);
    // Bright pass to half resolution.
    bloom_[0].bind();
    glBindTexture(GL_TEXTURE_2D, sceneBuf_.tex);
    down_.set("uTexel", 0.5f / sceneBuf_.w, 0.5f / sceneBuf_.h);
    down_.set("uThreshold", s.bloomThreshold);
    fullscreenQuad();
    // Half -> quarter, quarter -> eighth.
    auto downsample = [&](const RenderTarget& src, const RenderTarget& dst) {
        down_.use();
        dst.bind();
        glBindTexture(GL_TEXTURE_2D, src.tex);
        down_.set("uTexel", 0.5f / src.w, 0.5f / src.h);
        down_.set("uThreshold", -1.0f);
        fullscreenQuad();
    };
    auto blur = [&](RenderTarget& a, RenderTarget& b, int iterations) {
        blur_.use();
        blur_.set("uSrc", 0);
        for (int i = 0; i < iterations; ++i) {
            b.bind();
            glBindTexture(GL_TEXTURE_2D, a.tex);
            blur_.set("uDir", 1.0f / a.w, 0.0f);
            fullscreenQuad();
            a.bind();
            glBindTexture(GL_TEXTURE_2D, b.tex);
            blur_.set("uDir", 0.0f, 1.0f / a.h);
            fullscreenQuad();
        }
    };
    downsample(bloom_[0], bloom_[1]);
    blur(bloom_[1], bloom_[2], 2);
    downsample(bloom_[1], bloom_[3]);
    blur(bloom_[3], bloom_[4], 3);
}

void Renderer::composite(const View& view, const RenderSettings& s) {
    bindDefaultFramebuffer(view.fbW, view.fbH);
    composite_.use();
    bindTexture(0, sceneBuf_.tex);
    bindTexture(1, bloom_[1].tex);
    bindTexture(2, bloom_[3].tex);
    composite_.set("uScene", 0);
    composite_.set("uBloomA", 1);
    composite_.set("uBloomB", 2);
    composite_.set("uExposure", s.exposure);
    composite_.set("uBloom", s.bloom ? s.bloomStrength : 0.0f);
    composite_.set("uVignette", s.vignette ? 1.0f : 0.0f);
    fullscreenQuad();
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
}

void Renderer::render(const World& world, const View& view, const RenderSettings& s) {
    // Light buffer size is DPI-independent: resolutionScale texels per window point.
    float lightScale = std::min(1.0f, s.resolutionScale / std::max(view.pixelsPerPoint, 1e-3f));
    ensureTargets(view.fbW, view.fbH, std::max(1, static_cast<int>(view.fbW * lightScale)),
                  std::max(1, static_cast<int>(view.fbH * lightScale)));
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    std::vector<const Light*> lights;
    for (const Light& l : world.lights)
        if (l.intensity * l.level > 1e-3f) lights.push_back(&l);
    for (const Light& l : world.tempLights)
        if (l.intensity * l.level > 1e-3f) lights.push_back(&l);

    if (s.shadows) buildShadowMaps(world, lights);
    else shadowRow_.assign(lights.size(), -1);
    lightPass(world, view, s, lights);
    scenePass(world, view, s, lights);
    if (s.bloom) bloomPass(s);
    composite(view, s);
}

void Renderer::drawGizmos(const World& world, const View& view) const {
    view.apply();
    glUseProgram(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    auto circle = [](Vec2 c, float r) {
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 48; ++i) {
            Vec2 p = c + fromAngle(kTau * i / 48) * r;
            glVertex2f(p.x, p.y);
        }
        glEnd();
    };
    glColor4f(0.2f, 1.0f, 0.3f, 0.8f);
    for (const Occluder& o : world.occluders) {
        glBegin(GL_LINE_LOOP);
        for (const Vec2& p : o.pts) glVertex2f(p.x, p.y);
        glEnd();
    }
    auto drawLight = [&](const Light& l) {
        glColor4f(l.color.x, l.color.y, l.color.z, 0.35f);
        circle(l.pos, l.radius);
        glColor4f(1, 1, 1, 0.9f);
        circle(l.pos, std::max(l.size, 0.1f));
        if (l.cone) {
            glBegin(GL_LINES);
            for (float s : {-1.0f, 1.0f}) {
                Vec2 e = l.pos + fromAngle(l.dir + s * l.coneAngle) * l.radius;
                glVertex2f(l.pos.x, l.pos.y);
                glVertex2f(e.x, e.y);
            }
            glEnd();
        }
    };
    for (const Light& l : world.lights) drawLight(l);
    for (const Light& l : world.tempLights) drawLight(l);
    glColor4f(1.0f, 0.4f, 0.9f, 0.8f);
    glBegin(GL_LINES);
    for (const LineLight& ll : world.lineLights) {
        glVertex2f(ll.a.x, ll.a.y);
        glVertex2f(ll.b.x, ll.b.y);
    }
    glEnd();
    glDisable(GL_BLEND);
}
