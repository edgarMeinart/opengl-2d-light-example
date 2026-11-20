#include "effects.h"

#include <algorithm>

// ---------------------------------------------------------------- Particles

void Particles::burst(Vec2 at, Vec2 dir, float spread, int count, Vec3 color, float speed, float life) {
    float base = std::atan2(dir.y, dir.x);
    for (int i = 0; i < count; ++i) {
        float a = base + rng_.range(-spread, spread);
        float s = speed * rng_.range(0.3f, 1.0f);
        float l = life * rng_.range(0.5f, 1.0f);
        ps_.push_back({at, fromAngle(a) * s, l, l, color});
    }
}

void Particles::update(float dt, float drag, Vec2 gravity) {
    float k = std::exp(-drag * dt);
    for (P& p : ps_) {
        p.v = (p.v + gravity * dt) * k;
        p.p += p.v * dt;
        p.life -= dt;
    }
    ps_.erase(std::remove_if(ps_.begin(), ps_.end(), [](const P& p) { return p.life <= 0; }), ps_.end());
}

void Particles::draw(World& w, float intensity) const {
    for (const P& p : ps_) {
        float t = p.life / p.maxLife;
        Vec2 tail = p.p - p.v * 0.035f;
        // Cools from white-hot to the base colour as it dies.
        Vec3 c = lerp(p.color, Vec3(1, 1, 1), t * t);
        w.addGlowLine(tail, p.p, c, 0.07f, intensity * t);
    }
}

// ---------------------------------------------------------------- Lightning

void makeBolt(Vec2 from, Vec2 to, float jaggedness, bool branches, Rng& rng, std::vector<BoltSegment>& out) {
    std::vector<BoltSegment> segs{{from, to, 1.0f}};
    float offset = length(to - from) * jaggedness;
    for (int gen = 0; gen < 6; ++gen) {
        std::vector<BoltSegment> next;
        next.reserve(segs.size() * 2 + 4);
        for (const BoltSegment& s : segs) {
            Vec2 d = s.b - s.a;
            Vec2 mid = (s.a + s.b) * 0.5f + normalize(perp(d)) * rng.range(-offset, offset);
            next.push_back({s.a, mid, s.weight});
            next.push_back({mid, s.b, s.weight});
            if (branches && gen < 4 && rng.uniform() < 0.28f * s.weight) {
                Vec2 bd = rotate(mid - s.a, rng.range(-0.7f, 0.7f)) * rng.range(1.0f, 2.2f);
                next.push_back({mid, mid + bd, s.weight * 0.45f});
            }
        }
        segs.swap(next);
        offset *= 0.5f;
    }
    out.insert(out.end(), segs.begin(), segs.end());
}

void drawBolt(World& w, const std::vector<BoltSegment>& bolt, Vec3 color, float intensity, float width,
              float lightIntensity) {
    for (const BoltSegment& s : bolt) {
        w.addGlowLine(s.a, s.b, color, width * (0.5f + 0.5f * s.weight), intensity * s.weight);
        w.addGlowLine(s.a, s.b, Vec3(1, 1, 1), width * 0.25f, intensity * 2.0f * s.weight);
    }
    if (lightIntensity <= 0) return;
    // Light the floor along the main channel in runs of ~3 units. A run ends at a gap in the chain
    // (the next arc of a multi-arc bolt) and at the end, so tips and short arcs are lit too.
    Vec2 runStart, runEnd;
    float runLen = 0;
    auto flush = [&]() {
        if (runLen > 0) w.lineLights.push_back({runStart, runEnd, color, lightIntensity, 9.0f});
        runLen = 0;
    };
    for (const BoltSegment& s : bolt) {
        if (s.weight < 0.99f) continue;
        if (runLen > 0 && length(s.a - runEnd) > 1e-4f) flush();
        if (runLen == 0) runStart = s.a;
        runLen += length(s.b - s.a);
        runEnd = s.b;
        if (runLen > 3.0f) flush();
    }
    flush();
}

// ---------------------------------------------------------------- Laser

LaserPath traceLaser(const World& w, Vec2 origin, Vec2 dir, int maxBounces, float maxDist) {
    LaserPath path;
    path.points.push_back(origin);
    Vec2 o = origin, d = normalize(dir);
    float remaining = maxDist;
    for (int bounce = 0; bounce <= maxBounces && remaining > 0; ++bounce) {
        RayHit hit;
        if (!raycast(w, o, d, remaining, hit)) {
            path.points.push_back(o + d * remaining);
            return path;
        }
        path.points.push_back(hit.point);
        remaining -= hit.dist;
        if (!w.occluders[hit.occluder].mirror) {
            path.hit = true;
            path.endNormal = hit.normal;
            return path;
        }
        d = reflect(d, hit.normal);
        o = hit.point + hit.normal * 1e-3f;
    }
    return path;
}

void drawLaser(World& w, const LaserPath& path, Vec3 color, float intensity) {
    for (size_t i = 0; i + 1 < path.points.size(); ++i) {
        Vec2 a = path.points[i], b = path.points[i + 1];
        w.addGlowLine(a, b, color, 0.5f, intensity * 0.3f);          // halo
        w.addGlowLine(a, b, lerp(color, Vec3(1, 1, 1), 0.6f), 0.12f, intensity * 3.0f);  // hot core
        w.lineLights.push_back({a, b, color, intensity * 0.45f, 5.0f});
    }
    for (size_t i = 1; i + 1 < path.points.size(); ++i)  // bounce flares
        w.addGlowDot(path.points[i], color, 0.45f, intensity * 1.5f);
    if (path.hit) {
        Vec2 p = path.points.back();
        w.addGlowDot(p, lerp(color, Vec3(1, 1, 1), 0.5f), 0.6f, intensity * 3.0f);
        Light l;
        l.pos = p + path.endNormal * 0.4f;
        l.color = color;
        l.intensity = intensity * 0.8f;
        l.radius = 9.0f;
        l.size = 0.2f;
        l.height = 1.0f;
        l.bulb = false;
        l.flicker = Flicker::Candle;
        l.flickerRate = 3.0f;
        w.tempLights.push_back(l);
    }
}

// ---------------------------------------------------------------- Rain

void Rain::update(float dt, Vec2 min, Vec2 max, int count) {
    while (static_cast<int>(drops_.size()) < count)
        drops_.push_back({Vec2(rng_.range(min.x, max.x), rng_.range(min.y, max.y)), rng_.uniform(), rng_.range(30, 45)});
    drops_.resize(count);
    const Vec2 wind = normalize(Vec2(-0.35f, -1.0f));
    for (Drop& d : drops_) {
        d.p += wind * d.speed * dt;
        if (d.p.y < min.y || d.p.x < min.x) {
            d.p = Vec2(rng_.range(min.x, max.x + 15.0f), max.y + rng_.range(0, 5));
            d.z = rng_.uniform();
        }
    }
}

void Rain::draw(World& w, float intensity) const {
    const Vec2 wind = normalize(Vec2(-0.35f, -1.0f));
    for (const Drop& d : drops_) {
        float len = 0.8f + 1.2f * d.z;
        w.addGlowLine(d.p, d.p - wind * len, Vec3(0.55f, 0.65f, 0.8f), 0.04f, intensity * (0.3f + 0.7f * d.z));
    }
}
