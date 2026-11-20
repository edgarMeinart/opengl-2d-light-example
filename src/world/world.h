#pragma once
// Scene description consumed by the renderer: occluders, lights and per-frame emissive geometry.

#include <vector>

#include "../core/math.h"
#include "../gfx/materials.h"

enum class Flicker { None, Candle, Pulse, Blink, Neon, Strobe };
const char* flickerName(Flicker f);

// Brightness multiplier (0..1) for a flicker pattern at time t.
float flickerLevel(Flicker f, float t, float rate, float phase);

struct Light {
    Vec2 pos;
    Vec3 color{1.0f, 0.85f, 0.65f};
    float intensity = 1.5f;
    float radius = 25.0f;  // reach in world units
    float size = 0.8f;     // source radius; bigger = wider penumbra
    float height = 3.0f;   // height above the floor, used for bump shading
    bool shadows = true;
    bool bulb = true;      // draw a glowing dot at the light position

    bool cone = false;     // spotlight
    float dir = 0.0f;      // spotlight direction (radians)
    float coneAngle = 0.5f;     // half angle (radians)
    float coneSoftness = 0.15f; // angular fade at the cone edge (radians)

    Flicker flicker = Flicker::None;
    float flickerRate = 1.0f;
    float phase = 0.0f;

    float level = 1.0f;    // flicker result for this frame, set by World::updateFlicker
};

// Convex polygon, counter-clockwise. Circles are stored as polygons too.
struct Occluder {
    std::vector<Vec2> pts;
    Vec3 color{0.16f, 0.16f, 0.18f};
    bool mirror = false;   // reflects laser beams
    float glow = 0.0f;     // emissive strength of the top surface
};

// Light emitted along a segment (laser beams, arcs). No shadows.
struct LineLight {
    Vec2 a, b;
    Vec3 color;
    float intensity;
    float radius;
};

// Additive HDR glow strokes and dots (beams, bolts, bulbs, sparks, rain).
struct GlowLine {
    Vec2 a, b;
    Vec3 color;
    float width;
    float intensity;
};

struct World {
    FloorType floor = FloorType::Tiles;
    Vec3 ambient{0.04f, 0.04f, 0.05f};
    Vec3 flash{0, 0, 0};  // extra ambient this frame (lightning)

    std::vector<Occluder> occluders;
    std::vector<Light> lights;

    // Transient, rebuilt every frame by the example.
    std::vector<Light> tempLights;
    std::vector<LineLight> lineLights;
    std::vector<GlowLine> glow;

    void clear();
    void beginFrame();  // clears transient lists and flash
    void updateFlicker(float time);

    int addPolygon(std::vector<Vec2> pts, Vec3 color);
    int addBox(Vec2 center, Vec2 halfSize, float angle, Vec3 color);
    int addCircle(Vec2 center, float radius, Vec3 color, int segments = 20);
    int addWall(Vec2 a, Vec2 b, float thickness, Vec3 color);
    Light& addLight(Vec2 pos, Vec3 color, float intensity, float radius);

    void addGlowDot(Vec2 p, Vec3 color, float radius, float intensity) {
        glow.push_back({p, p, color, radius, intensity});
    }
    void addGlowLine(Vec2 a, Vec2 b, Vec3 color, float width, float intensity) {
        glow.push_back({a, b, color, width, intensity});
    }
};

// Rectangle as a convex CCW polygon.
std::vector<Vec2> boxPoints(Vec2 center, Vec2 halfSize, float angle);

struct RayHit {
    Vec2 point;
    Vec2 normal;  // outward surface normal
    float dist = 0;
    int occluder = -1;
};

// Nearest occluder edge hit along origin + dir * t, t in (0, maxDist].
bool raycast(const World& w, Vec2 origin, Vec2 dir, float maxDist, RayHit& hit);

bool pointInOccluder(const Occluder& o, Vec2 p);
