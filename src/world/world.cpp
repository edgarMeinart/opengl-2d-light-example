#include "world.h"

#include <algorithm>

const char* flickerName(Flicker f) {
    switch (f) {
        case Flicker::None: return "Steady";
        case Flicker::Candle: return "Candle";
        case Flicker::Pulse: return "Pulse";
        case Flicker::Blink: return "Blink";
        case Flicker::Neon: return "Broken neon";
        case Flicker::Strobe: return "Strobe";
    }
    return "?";
}

float flickerLevel(Flicker f, float t, float rate, float phase) {
    switch (f) {
        case Flicker::None:
            return 1.0f;
        case Flicker::Candle: {
            float x = t * rate + phase;
            float n = 0.55f * noise1(x * 7.0f) + 0.30f * noise1(x * 17.0f + 3.1f) + 0.15f * noise1(x * 41.0f + 7.7f);
            // Occasional gusts that dip the flame.
            float gust = smoothstepf(0.72f, 0.95f, noise1(x * 1.3f + 11.0f));
            return 0.62f + 0.38f * n - 0.25f * gust;
        }
        case Flicker::Pulse:
            return 0.2f + 0.8f * (0.5f + 0.5f * std::sin(kTau * 0.5f * t * rate + phase));
        case Flicker::Blink: {
            float s = std::sin(kTau * t * rate + phase);
            return 0.03f + 0.97f * smoothstepf(-0.15f, 0.15f, s);
        }
        case Flicker::Neon: {
            // Mostly on; random slots drop out or stutter, sometimes in bursts.
            int slot = static_cast<int>(std::floor((t + phase) * 14.0f * rate));
            float burst = noise1((t + phase) * 0.7f);
            float r = hash1(slot * 31 + static_cast<int>(phase * 100));
            float threshold = burst > 0.6f ? 0.45f : 0.08f;
            if (r < threshold * 0.5f) return 0.04f;
            if (r < threshold) return 0.35f + 0.3f * hash1(slot * 17 + 5);
            return 0.95f + 0.05f * hash1(slot);
        }
        case Flicker::Strobe: {
            float x = (t + phase) * rate * 2.0f;
            return (x - std::floor(x)) < 0.07f ? 1.0f : 0.0f;
        }
    }
    return 1.0f;
}

void World::clear() {
    occluders.clear();
    lights.clear();
    beginFrame();
}

void World::beginFrame() {
    tempLights.clear();
    lineLights.clear();
    glow.clear();
    flash = Vec3(0, 0, 0);
}

void World::updateFlicker(float time) {
    for (Light& l : lights) l.level = flickerLevel(l.flicker, time, l.flickerRate, l.phase);
    for (Light& l : tempLights) l.level = flickerLevel(l.flicker, time, l.flickerRate, l.phase);
}

int World::addPolygon(std::vector<Vec2> pts, Vec3 color) {
    float area = 0;
    for (size_t i = 0; i < pts.size(); ++i) area += cross(pts[i], pts[(i + 1) % pts.size()]);
    if (area < 0) std::reverse(pts.begin(), pts.end());
    Occluder o;
    o.pts = std::move(pts);
    o.color = color;
    occluders.push_back(std::move(o));
    return static_cast<int>(occluders.size()) - 1;
}

std::vector<Vec2> boxPoints(Vec2 c, Vec2 h, float angle) {
    return {c + rotate(Vec2(-h.x, -h.y), angle), c + rotate(Vec2(h.x, -h.y), angle),
            c + rotate(Vec2(h.x, h.y), angle), c + rotate(Vec2(-h.x, h.y), angle)};
}

int World::addBox(Vec2 c, Vec2 h, float angle, Vec3 color) { return addPolygon(boxPoints(c, h, angle), color); }

int World::addCircle(Vec2 c, float r, Vec3 color, int segments) {
    std::vector<Vec2> pts;
    for (int i = 0; i < segments; ++i) pts.push_back(c + fromAngle(kTau * i / segments) * r);
    return addPolygon(std::move(pts), color);
}

int World::addWall(Vec2 a, Vec2 b, float thickness, Vec3 color) {
    Vec2 d = b - a;
    return addBox((a + b) * 0.5f, Vec2(length(d) * 0.5f, thickness * 0.5f), std::atan2(d.y, d.x), color);
}

Light& World::addLight(Vec2 pos, Vec3 color, float intensity, float radius) {
    Light l;
    l.pos = pos;
    l.color = color;
    l.intensity = intensity;
    l.radius = radius;
    l.phase = static_cast<float>(lights.size()) * 1.7f;
    lights.push_back(l);
    return lights.back();
}

bool raycast(const World& w, Vec2 o, Vec2 d, float maxDist, RayHit& hit) {
    float best = maxDist;
    bool found = false;
    for (size_t oi = 0; oi < w.occluders.size(); ++oi) {
        const std::vector<Vec2>& p = w.occluders[oi].pts;
        for (size_t i = 0; i < p.size(); ++i) {
            Vec2 a = p[i], e = p[(i + 1) % p.size()] - a;
            float denom = cross(d, e);
            if (std::fabs(denom) < 1e-9f) continue;
            Vec2 ao = a - o;
            float t = cross(ao, e) / denom;  // distance along the ray
            float s = cross(ao, d) / denom;  // position along the edge
            if (t > 1e-4f && t < best && s >= 0.0f && s <= 1.0f) {
                best = t;
                found = true;
                hit.occluder = static_cast<int>(oi);
                hit.normal = normalize(Vec2(e.y, -e.x));  // outward for CCW polygons
            }
        }
    }
    if (found) {
        hit.dist = best;
        hit.point = o + d * best;
    }
    return found;
}

bool pointInOccluder(const Occluder& o, Vec2 p) {
    for (size_t i = 0; i < o.pts.size(); ++i)
        if (cross(o.pts[(i + 1) % o.pts.size()] - o.pts[i], p - o.pts[i]) < 0) return false;
    return !o.pts.empty();
}
