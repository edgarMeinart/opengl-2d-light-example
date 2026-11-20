#pragma once
// Reusable effects built on top of World: sparks, lightning bolts, laser beams, rain.

#include <vector>

#include "../world/world.h"

// Short-lived glowing streaks (sparks, embers).
class Particles {
public:
    void burst(Vec2 at, Vec2 dir, float spread, int count, Vec3 color, float speed, float life);
    void update(float dt, float drag, Vec2 gravity = Vec2(0, 0));
    void draw(World& w, float intensity = 3.0f) const;
    void clear() { ps_.clear(); }

private:
    struct P {
        Vec2 p, v;
        float life, maxLife;
        Vec3 color;
    };
    std::vector<P> ps_;
    Rng rng_{1234};
};

struct BoltSegment {
    Vec2 a, b;
    float weight;  // 1 for the main channel, smaller for branches
};

// Jagged lightning path from `from` to `to` (midpoint displacement with random branches).
void makeBolt(Vec2 from, Vec2 to, float jaggedness, bool branches, Rng& rng, std::vector<BoltSegment>& out);

// Glow strokes for a bolt, plus line lights along the main channel (lightIntensity 0 = none).
void drawBolt(World& w, const std::vector<BoltSegment>& bolt, Vec3 color, float intensity, float width,
              float lightIntensity);

struct LaserPath {
    std::vector<Vec2> points;  // origin, bounce points..., end
    bool hit = false;          // ended on a non-mirror surface
    Vec2 endNormal;
};

LaserPath traceLaser(const World& w, Vec2 origin, Vec2 dir, int maxBounces, float maxDist);
void drawLaser(World& w, const LaserPath& path, Vec3 color, float intensity);

// Falling rain streaks seen from above (slanted by wind).
class Rain {
public:
    void update(float dt, Vec2 min, Vec2 max, int count);
    void draw(World& w, float intensity) const;

private:
    struct Drop {
        Vec2 p;
        float z, speed;
    };
    std::vector<Drop> drops_;
    Rng rng_{777};
};
