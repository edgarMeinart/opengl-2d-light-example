#pragma once
// Deferred-ish 2D lighting pipeline (OpenGL 2.1 + GLSL 1.20):
//   1. CPU builds a polar shadow map per light (nearest occluder distance per angle).
//   2. Light pass: every light is splatted additively into an HDR light buffer, with
//      area-light soft shadows (penumbra), spotlight cones and bump/specular shading.
//   3. Scene pass: floor albedo and occluders are multiplied by the light buffer; glow
//      strokes (beams, bolts, bulbs, sparks) are added on top in HDR.
//   4. Bloom (bright pass + blurred mip chain) and ACES tone mapping to the window.

#include <vector>

#include "../core/math.h"
#include "../world/world.h"
#include "materials.h"
#include "shader.h"
#include "target.h"

// Maps a world-space rectangle onto the framebuffer.
struct View {
    int fbW = 1, fbH = 1;  // framebuffer size in pixels
    Vec2 min, max;         // visible world rectangle
    float pixelsPerPoint = 1.0f;  // framebuffer pixels per window point (2 on Retina)

    // Smallest view containing `worldSize` (anchored at the origin), centred, aspect-correct.
    static View fit(int fbW, int fbH, Vec2 worldSize);
    void apply() const;  // legacy projection/modelview for immediate-mode drawing
    // Framebuffer pixel (origin top-left) to world coordinates.
    Vec2 toWorld(float px, float py) const;
};

struct RenderSettings {
    bool shadows = true;
    bool softShadows = true;
    int penumbraQuality = 1;  // 0 low, 1 medium, 2 high
    float penumbraScale = 1.0f;
    float bump = 1.0f;
    float specular = 1.0f;
    float ambient = 1.0f;
    float exposure = 1.0f;
    bool bloom = true;
    float bloomStrength = 0.9f;
    float bloomThreshold = 0.9f;
    bool vignette = true;
    float resolutionScale = 1.0f;  // light buffer texels per window point (2 = full Retina)
};

class Renderer {
public:
    bool init();
    void destroy();
    // Renders the world into the window framebuffer (left bound on return).
    void render(const World& world, const View& view, const RenderSettings& settings);
    // Debug overlay: light radii, cones and occluder outlines, drawn on the current framebuffer.
    void drawGizmos(const World& world, const View& view) const;

    static constexpr int kShadowRes = 1024;  // angular bins per light
    static constexpr int kMaxShadowLights = 64;
    static constexpr int kMinLevels = 10;    // min-pyramid levels: window radius 2^L - 1 bins

private:
    void ensureTargets(int w, int h, int lightW, int lightH);
    void buildShadowMaps(const World& world, const std::vector<const Light*>& lights);
    void lightPass(const World& world, const View& view, const RenderSettings& s,
                   const std::vector<const Light*>& lights);
    void scenePass(const World& world, const View& view, const RenderSettings& s,
                   const std::vector<const Light*>& lights);
    void bloomPass(const RenderSettings& s);
    void composite(const View& view, const RenderSettings& s);

    Shader light_, lineLight_, floor_, occluder_, glow_, down_, blur_, composite_;
    RenderTarget lightBuf_, sceneBuf_;
    RenderTarget bloom_[5];  // half, quarter A/B, eighth A/B
    GLuint shadowTex_ = 0;     // (distance, occluder id) per angular bin, one row per light
    GLuint shadowMinTex_ = 0;  // min-pyramid of the distances, kMinLevels rows per light
    std::vector<float> shadowData_, shadowMin_;
    std::vector<int> shadowRow_;  // per active light, -1 = unshadowed
    float cos_[kShadowRes], sin_[kShadowRes];
    Materials materials_;
    int fbW_ = 0, fbH_ = 0, lightW_ = 0, lightH_ = 0;
};
