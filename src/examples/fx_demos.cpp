// Effect showcases: flickering/blinking lights, lasers with mirrors, a thunderstorm,
// a Tesla coil and sweeping searchlights.

#include <algorithm>

#include "../fx/effects.h"
#include "example.h"
#include "imgui.h"

namespace {

const Vec3 kStone{0.16f, 0.15f, 0.14f};
const Vec3 kDark{0.10f, 0.10f, 0.11f};
const Vec3 kMirror{0.55f, 0.6f, 0.68f};

void colorEdit(const char* label, Vec3& c) { ImGui::ColorEdit3(label, &c.x, ImGuiColorEditFlags_NoInputs); }

struct Area {
    Vec2 center;
    float radius;
};

// Random points at least `spacing` apart and outside every `keepClear` area, so props don't overlap.
template <size_t N>
std::vector<Vec2> scatter(Rng& r, int count, Vec2 lo, Vec2 hi, float spacing, const Area (&keepClear)[N]) {
    std::vector<Vec2> pts;
    for (int attempt = 0; attempt < count * 50 && static_cast<int>(pts.size()) < count; ++attempt) {
        Vec2 p(r.range(lo.x, hi.x), r.range(lo.y, hi.y));
        bool ok = true;
        for (const Area& a : keepClear) ok = ok && length(p - a.center) >= a.radius;
        for (Vec2 q : pts) ok = ok && length(p - q) >= spacing;
        if (ok) pts.push_back(p);
    }
    return pts;
}

// Four walls around the rectangle lo..hi.
void addRoom(World& w, Vec2 lo, Vec2 hi, float t, Vec3 color) {
    w.addWall(Vec2(lo.x, lo.y), Vec2(hi.x, lo.y), t, color);
    w.addWall(Vec2(lo.x, hi.y), Vec2(hi.x, hi.y), t, color);
    w.addWall(Vec2(lo.x, lo.y), Vec2(lo.x, hi.y), t, color);
    w.addWall(Vec2(hi.x, lo.y), Vec2(hi.x, hi.y), t, color);
}

// ---------------------------------------------------------------- Flicker & blink

class FlickerBlink : public Example {
public:
    void setup(World& w) override {
        w.floor = FloorType::Bricks;
        w.ambient = Vec3(0.01f, 0.009f, 0.009f);
        addRoom(w, Vec2(3, 3), Vec2(77, 47), 1.0f, kStone);
        // Interior walls with doorways.
        w.addWall(Vec2(30, 3), Vec2(30, 20), 1.0f, kStone);
        w.addWall(Vec2(30, 27), Vec2(30, 47), 1.0f, kStone);
        w.addWall(Vec2(54, 3), Vec2(54, 30), 1.0f, kStone);
        w.addWall(Vec2(54, 37), Vec2(54, 47), 1.0f, kStone);
        w.addWall(Vec2(54, 25), Vec2(66, 25), 1.0f, kStone);
        w.addWall(Vec2(71, 25), Vec2(77, 25), 1.0f, kStone);
        for (Vec2 p : {Vec2(15, 16), Vec2(15, 32), Vec2(42, 14), Vec2(42, 36), Vec2(66, 37)})
            w.addCircle(p, 1.3f, kStone);

        auto torch = [&](Vec2 p, float phase) {
            Light& l = w.addLight(p, Vec3(1.0f, 0.5f, 0.18f), 1.8f, 22.0f);
            l.size = 0.5f;
            l.flicker = Flicker::Candle;
            l.phase = phase;
        };
        torch(Vec2(5, 24), 0.0f);
        torch(Vec2(27.5f, 40), 3.3f);
        torch(Vec2(27.5f, 8), 7.1f);
        torch(Vec2(17, 44.5f), 9.4f);
        torch(Vec2(50, 44.5f), 5.5f);

        Light& beacon = w.addLight(Vec2(42, 6), Vec3(1.0f, 0.08f, 0.04f), 3.0f, 28.0f);
        beacon.flicker = Flicker::Blink;
        beacon.flickerRate = 1.1f;
        beacon.size = 0.3f;

        Light& neon = w.addLight(Vec2(66, 43.5f), Vec3(0.25f, 0.9f, 1.0f), 2.2f, 26.0f);
        neon.flicker = Flicker::Neon;
        neon.size = 3.0f;  // long tube: wide penumbra
        neon.bulb = false;

        Light& crystal = w.addLight(Vec2(66, 13), Vec3(0.75f, 0.3f, 1.0f), 2.4f, 24.0f);
        crystal.flicker = Flicker::Pulse;
        crystal.flickerRate = 0.6f;
        crystal.size = 0.6f;
        crystal.bulb = false;

        Light& strobe = w.addLight(Vec2(42, 25), Vec3(0.9f, 0.95f, 1.0f), 0.0f, 30.0f);
        strobe.flicker = Flicker::Strobe;
        strobe.flickerRate = 2.0f;
        strobe.size = 0.3f;
    }

    void update(World& w, const Input&, float, float time) override {
        for (int i = 0; i < kTorches; ++i) w.lights[i].flickerRate = speed_;
        w.lights[kBeacon].flickerRate = 1.1f * speed_;
        w.lights[kNeon].flickerRate = speed_;
        w.lights[kCrystal].flickerRate = 0.6f * speed_;
        w.lights[kStrobe].intensity = strobe_ ? 2.5f : 0.0f;

        // Neon tube and crystal glow follow their light's level (World::updateFlicker runs later,
        // so evaluate the same pattern here).
        const Light& neon = w.lights[kNeon];
        float nl = flickerLevel(neon.flicker, time, neon.flickerRate, neon.phase);
        w.addGlowLine(Vec2(62, 44.6f), Vec2(70, 44.6f), neon.color, 0.18f, 3.5f * nl);
        const Light& c = w.lights[kCrystal];
        float cl = flickerLevel(c.flicker, time, c.flickerRate, c.phase);
        for (int i = 0; i < 4; ++i) {
            Vec2 a = c.pos + fromAngle(kTau * i / 4 + time * 0.3f) * 0.9f;
            Vec2 b = c.pos + fromAngle(kTau * (i + 1) / 4 + time * 0.3f) * 0.9f;
            w.addGlowLine(a, b, c.color, 0.12f, 2.5f * cl);
        }
        w.addGlowDot(c.pos, Vec3(1, 0.8f, 1), 0.35f, 3.0f * cl);
    }

    void ui(World& w, RenderSettings&) override {
        ImGui::SliderFloat("Flicker speed", &speed_, 0.1f, 3.0f, "%.2f");
        colorEdit("Torch colour", torchColor_);
        for (int i = 0; i < kTorches; ++i) w.lights[i].color = torchColor_;
        ImGui::Checkbox("Strobe light", &strobe_);
        ImGui::SameLine();
        ImGui::TextDisabled("(photosensitivity)");
        ImGui::TextDisabled("Torches: candle noise. Red beacon: blink.\nCyan tube: broken neon. Crystal: pulse.");
    }

private:
    // Light indices, in the order setup() adds them.
    enum { kTorches = 5, kBeacon = 5, kNeon, kCrystal, kStrobe };
    float speed_ = 1.0f;
    bool strobe_ = false;
    Vec3 torchColor_{1.0f, 0.5f, 0.18f};
};

// ---------------------------------------------------------------- Laser & mirrors

class LaserMirrors : public Example {
public:
    void setup(World& w) override {
        w.floor = FloorType::Metal;
        w.ambient = Vec3(0.015f, 0.015f, 0.02f);
        // Fixed mirrors along the edges.
        auto mirror = [&](Vec2 c, float halfLen, float angle) {
            int i = w.addBox(c, Vec2(halfLen, 0.15f), angle, kMirror);
            w.occluders[i].mirror = true;
            return i;
        };
        mirror(Vec2(40, 47), 14.0f, 0.0f);
        mirror(Vec2(40, 3), 14.0f, 0.0f);
        mirror(Vec2(77, 25), 10.0f, kPi * 0.5f);
        mirror(Vec2(68, 44), 4.0f, -0.6f);
        mirror(Vec2(68, 6), 4.0f, 0.6f);
        // Rotating mirrors.
        const Vec2 spin[] = {Vec2(30, 32), Vec2(48, 16), Vec2(55, 34), Vec2(22, 14)};
        for (int i = 0; i < 4; ++i) rotating_.push_back({mirror(spin[i], 2.4f, 0.0f), spin[i], i * 0.8f, (i % 2 ? -1.0f : 1.0f) * (0.3f + 0.1f * i)});
        // Absorbing blocks.
        w.addBox(Vec2(40, 25), Vec2(1.5f, 1.5f), 0.785f, kDark);
        w.addCircle(Vec2(62, 25), 1.8f, kDark);
        w.addBox(Vec2(14, 38), Vec2(2.0f, 1.0f), 0.3f, kDark);
        Light& fill = w.addLight(Vec2(6, 25), Vec3(1, 0.3f, 0.25f), 0.6f, 12.0f);
        fill.size = 0.3f;
    }

    void update(World& w, const Input& in, float dt, float time) override {
        for (Rotating& r : rotating_) {
            r.angle += r.speed * spin_ * dt;
            w.occluders[r.index].pts = boxPoints(r.center, Vec2(2.4f, 0.15f), r.angle);
        }
        Vec2 origin(6, 25);
        Vec2 dir = in.hover ? in.mouse - origin : fromAngle(0.55f * std::sin(time * 0.4f));
        fire(w, origin, dir, color_, dt);
        if (second_) fire(w, Vec2(74, 46), fromAngle(kPi * 1.25f + 0.4f * std::sin(time * 0.3f)), Vec3(0.2f, 1.0f, 0.3f), dt);
        sparks_.update(dt, 3.0f);
        sparks_.draw(w);
        w.addGlowDot(origin, color_, 0.5f, 2.0f);
    }

    void ui(World&, RenderSettings&) override {
        ImGui::SliderInt("Max bounces", &bounces_, 0, 30);
        ImGui::SliderFloat("Mirror spin", &spin_, 0.0f, 4.0f, "%.2f");
        colorEdit("Laser colour", color_);
        ImGui::Checkbox("Second laser", &second_);
        ImGui::TextDisabled("Aim with the mouse. Silver bars reflect,\ndark blocks absorb and throw sparks.");
    }

private:
    void fire(World& w, Vec2 origin, Vec2 dir, Vec3 color, float dt) {
        LaserPath path = traceLaser(w, origin, dir, bounces_, 400.0f);
        drawLaser(w, path, color, 1.6f);
        if (path.hit) {
            Vec2 back = path.points.back() - path.points[path.points.size() - 2];
            Vec2 out = reflect(normalize(back), path.endNormal);
            sparkDebt_ += dt * 90.0f;
            int n = static_cast<int>(sparkDebt_);
            sparkDebt_ -= n;
            sparks_.burst(path.points.back(), out, 1.1f, n, lerp(color, Vec3(1, 0.8f, 0.3f), 0.5f), 14.0f, 0.35f);
        }
    }

    struct Rotating {
        int index;
        Vec2 center;
        float angle, speed;
    };
    std::vector<Rotating> rotating_;
    Particles sparks_;
    float sparkDebt_ = 0;
    int bounces_ = 12;
    float spin_ = 1.0f;
    bool second_ = true;
    Vec3 color_{1.0f, 0.12f, 0.08f};
};

// ---------------------------------------------------------------- Thunderstorm

class Thunderstorm : public Example {
public:
    void configure(RenderSettings& s) override { s.bloomStrength = 1.1f; }

    void setup(World& w) override {
        w.floor = FloorType::Cobble;
        w.ambient = Vec3(0.012f, 0.016f, 0.03f);
        w.addBox(Vec2(18, 30), Vec2(5, 3.5f), 0.15f, Vec3(0.2f, 0.12f, 0.1f));
        w.addBox(Vec2(60, 16), Vec2(4, 4), -0.3f, Vec3(0.2f, 0.12f, 0.1f));
        for (int i = 0; i < 8; ++i) w.addWall(Vec2(46.0f + i * 3.0f, 40), Vec2(47.6f + i * 3.0f, 40.4f), 0.35f, kStone);
        Rng r(5);
        const Area clear[] = {{Vec2(40, 25), 7}, {Vec2(18, 30), 8}, {Vec2(60, 16), 7.5f}, {Vec2(57, 40), 4}};
        for (Vec2 p : scatter(r, 16, Vec2(6, 6), Vec2(74, 44), 5.0f, clear))
            w.addCircle(p, r.range(0.8f, 2.0f), Vec3(0.08f, 0.11f, 0.07f), 14);
        Light& lantern = w.addLight(Vec2(40, 25), Vec3(1.0f, 0.6f, 0.25f), 1.4f, 16.0f);
        lantern.flicker = Flicker::Candle;
        lantern.size = 0.35f;
        next_ = 1.5f;
    }

    void update(World& w, const Input& in, float dt, float) override {
        Light& lantern = w.lights[0];
        if (in.hover) lantern.pos = lerp(lantern.pos, in.mouse, 1.0f - std::exp(-dt * 10.0f));
        if (in.hover && in.pressed[0]) strike(in.mouse);
        next_ -= dt * frequency_;
        if (next_ <= 0) {
            strike(Vec2(rng_.range(8, 72), rng_.range(8, 42)));
            next_ = rng_.range(2.0f, 6.0f);
        }

        if (age_ >= 0) {
            age_ += dt;
            // Main stroke plus two return strokes, each a sharp flash that decays.
            float f = 0;
            const float strokes[3] = {0.0f, 0.13f, 0.27f};
            for (float s : strokes)
                if (age_ >= s) f = std::max(f, std::exp(-(age_ - s) * 14.0f) * (s == 0 ? 1.0f : 0.75f));
            if (age_ > 0.13f && age_ - dt <= 0.13f) regenerate();
            if (age_ > 0.27f && age_ - dt <= 0.27f) regenerate();
            if (f < 0.01f && age_ > 0.3f) age_ = -1;
            else {
                drawBolt(w, bolt_, Vec3(0.7f, 0.75f, 1.0f), 6.0f * f, 0.32f, 1.4f * f);
                w.flash = Vec3(0.35f, 0.38f, 0.5f) * f;
                Light l;
                l.pos = target_;
                l.color = Vec3(0.75f, 0.8f, 1.0f);
                l.intensity = 7.0f * f;
                l.radius = 75.0f;
                l.size = 1.5f;
                l.height = 10.0f;
                l.bulb = false;
                w.tempLights.push_back(l);
            }
        }
        sparks_.update(dt, 2.5f);
        sparks_.draw(w, 4.0f);
        if (rain_ > 0) {
            rainDrops_.update(dt, Vec2(-5, -5), Vec2(90, 55), rain_);
            rainDrops_.draw(w, 0.25f + 1.5f * std::min(1.0f, w.flash.x * 3.0f));
        }
    }

    void ui(World&, RenderSettings&) override {
        if (ImGui::Button("Strike now")) strike(Vec2(rng_.range(8, 72), rng_.range(8, 42)));
        ImGui::SameLine();
        ImGui::TextDisabled("or click the ground");
        ImGui::SliderFloat("Storm intensity", &frequency_, 0.0f, 4.0f, "%.1f");
        ImGui::SliderInt("Rain", &rain_, 0, 1500);
    }

private:
    void strike(Vec2 at) {
        target_ = at;
        origin_ = at + Vec2(rng_.range(-25, 25), 70);
        age_ = 0;
        regenerate();
        sparks_.burst(at, Vec2(0, 1), kPi, 70, Vec3(0.6f, 0.7f, 1.0f), 22.0f, 0.6f);
    }

    void regenerate() {
        bolt_.clear();
        makeBolt(origin_, target_, 0.18f, true, rng_, bolt_);
    }

    Rng rng_{2024};
    std::vector<BoltSegment> bolt_;
    Vec2 origin_, target_;
    float age_ = -1, next_ = 0, frequency_ = 1.0f;
    int rain_ = 600;
    Rain rainDrops_;
    Particles sparks_;
};

// ---------------------------------------------------------------- Tesla coil

class TeslaCoil : public Example {
public:
    void setup(World& w) override {
        w.floor = FloorType::Metal;
        w.ambient = Vec3(0.008f, 0.009f, 0.014f);
        const Vec3 pylon(0.22f, 0.22f, 0.26f);
        for (int i = 0; i < 7; ++i) {
            Vec2 p = Vec2(40, 25) + fromAngle(kTau * i / 7 + 0.3f) * (13.0f + 4.0f * (i % 2));
            pylons_.push_back(p);
            w.addCircle(p, 0.9f, pylon, 14);
        }
        w.addBox(Vec2(12, 40), Vec2(3, 2), 0.2f, kDark);
        w.addBox(Vec2(68, 10), Vec2(2, 3), -0.3f, kDark);
        Light& coil = w.addLight(Vec2(40, 25), color_, 2.2f, 40.0f);
        coil.size = 1.4f;
        coil.bulb = false;
        coil.height = 4.0f;
    }

    void update(World& w, const Input& in, float dt, float time) override {
        const Vec2 c(40, 25);
        timer_ -= dt;
        if (timer_ <= 0) {
            timer_ = 0.05f;
            arcs_.clear();
            targets_.clear();
            for (int i = 0; i < arcCount_; ++i) {
                Vec2 to;
                bool mouseArc = attract_ && in.hover && length(in.mouse - c) < range_ && rng_.uniform() < 0.6f;
                if (mouseArc) to = in.mouse;
                else if (rng_.uniform() < 0.55f) {
                    // Strike the nearest side of a pylon within range.
                    Vec2 p = pylons_[rng_.next() % pylons_.size()];
                    if (length(p - c) > range_) continue;
                    to = p - normalize(p - c) * 0.9f;
                } else {
                    to = c + fromAngle(rng_.range(0, kTau)) * rng_.range(4.0f, range_ * 0.5f);
                }
                Vec2 from = c + normalize(to - c) * 1.6f;
                makeBolt(from, to, 0.2f, true, rng_, arcs_);
                targets_.push_back(to);
            }
            for (Vec2 t : targets_)
                if (rng_.uniform() < 0.3f) sparks_.burst(t, c - t, 1.4f, 6, color_, 10.0f, 0.3f);
            flicker_ = rng_.range(0.6f, 1.0f);
        }
        drawBolt(w, arcs_, color_, 2.6f * flicker_, 0.13f, 0.35f * flicker_);
        w.lights[0].color = color_;
        w.lights[0].intensity = 0.7f + 0.8f * flicker_;
        // The coil itself: glowing toroid rings.
        for (int ring = 0; ring < 2; ++ring) {
            float rr = 1.0f + 0.5f * ring;
            for (int i = 0; i < 16; ++i) {
                Vec2 a = c + fromAngle(kTau * i / 16 + time) * rr, b = c + fromAngle(kTau * (i + 1) / 16 + time) * rr;
                w.addGlowLine(a, b, color_, 0.1f, 1.5f * flicker_);
            }
        }
        w.addGlowDot(c, Vec3(1, 1, 1), 0.6f, 3.0f * flicker_);
        sparks_.update(dt, 3.0f);
        sparks_.draw(w);
    }

    void ui(World&, RenderSettings&) override {
        ImGui::SliderInt("Arcs", &arcCount_, 1, 12);
        ImGui::SliderFloat("Range", &range_, 8.0f, 40.0f, "%.0f");
        colorEdit("Arc colour", color_);
        ImGui::Checkbox("Arcs jump to the mouse", &attract_);
    }

private:
    std::vector<Vec2> pylons_, targets_;
    std::vector<BoltSegment> arcs_;
    Particles sparks_;
    Rng rng_{31337};
    float timer_ = 0, flicker_ = 1, range_ = 24.0f;
    int arcCount_ = 5;
    bool attract_ = true;
    Vec3 color_{0.55f, 0.65f, 1.0f};
};

// ---------------------------------------------------------------- Searchlights

class Searchlights : public Example {
public:
    void setup(World& w) override {
        w.floor = FloorType::Cobble;
        w.ambient = Vec3(0.01f, 0.012f, 0.016f);
        addRoom(w, Vec2(2, 2), Vec2(78, 48), 1.2f, kStone);
        Rng r(11);
        const Area clear[] = {{Vec2(4, 46), 4}, {Vec2(76, 46), 4}, {Vec2(40, 4), 4}};
        for (Vec2 p : scatter(r, 14, Vec2(10, 8), Vec2(70, 42), 5.0f, clear)) {
            if (r.uniform() < 0.5f)
                w.addBox(p, Vec2(r.range(0.8f, 1.8f), r.range(0.8f, 1.8f)), r.range(0, kPi), Vec3(0.24f, 0.16f, 0.09f));
            else
                w.addCircle(p, r.range(0.6f, 1.0f), Vec3(0.18f, 0.2f, 0.22f), 12);
        }
        const Vec2 towers[] = {Vec2(4, 46), Vec2(76, 46), Vec2(40, 4)};
        const float base[] = {-0.8f, -kPi + 0.8f, kPi * 0.5f};
        for (int i = 0; i < 3; ++i) {
            Light& l = w.addLight(towers[i], Vec3(0.9f, 0.95f, 1.0f), 3.5f, 70.0f);
            l.cone = true;
            l.dir = base[i];
            l.size = 0.4f;
            l.height = 6.0f;
            bases_[i] = base[i];
        }
        Light& player = w.addLight(Vec2(40, 25), Vec3(1.0f, 0.7f, 0.35f), 1.0f, 8.0f);
        player.size = 0.2f;
        player.flicker = Flicker::Candle;
    }

    void update(World& w, const Input& in, float dt, float time) override {
        Light& player = w.lights[3];
        if (in.hover) player.pos = lerp(player.pos, in.mouse, 1.0f - std::exp(-dt * 12.0f));
        spotted_ = false;
        for (int i = 0; i < 3; ++i) {
            Light& l = w.lights[i];
            l.dir = bases_[i] + 0.75f * std::sin(time * speed_ * (0.5f + 0.17f * i) + i * 2.0f);
            l.coneAngle = cone_;
            l.coneSoftness = cone_ * 0.3f;
            // Is the player inside this cone with a clear line of sight?
            Vec2 to = player.pos - l.pos;
            float dist = length(to);
            bool inCone = dot(normalize(to), fromAngle(l.dir)) > std::cos(cone_) && dist < l.radius * 0.8f;
            RayHit hit;
            bool seen = inCone && !raycast(w, l.pos, normalize(to), dist, hit);
            l.color = seen ? Vec3(1.0f, 0.25f, 0.2f) : Vec3(0.9f, 0.95f, 1.0f);
            spotted_ |= seen;
        }
        if (spotted_) w.addGlowDot(player.pos, Vec3(1, 0.2f, 0.1f), 0.6f, 2.0f + 2.0f * std::sin(time * 20.0f));
    }

    void ui(World&, RenderSettings&) override {
        ImGui::SliderFloat("Sweep speed", &speed_, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Beam width", &cone_, 0.08f, 0.6f, "%.2f");
        if (spotted_) ImGui::TextColored(ImVec4(1, 0.3f, 0.2f, 1), "SPOTTED!");
        else ImGui::TextDisabled("Sneak with the mouse; hide behind crates.");
    }

private:
    float bases_[3] = {0, 0, 0};
    float speed_ = 1.0f, cone_ = 0.2f;
    bool spotted_ = false;
};

}  // namespace

std::unique_ptr<Example> makeFlickerBlink() { return std::unique_ptr<Example>(new FlickerBlink()); }
std::unique_ptr<Example> makeLaserMirrors() { return std::unique_ptr<Example>(new LaserMirrors()); }
std::unique_ptr<Example> makeThunderstorm() { return std::unique_ptr<Example>(new Thunderstorm()); }
std::unique_ptr<Example> makeTeslaCoil() { return std::unique_ptr<Example>(new TeslaCoil()); }
std::unique_ptr<Example> makeSearchlights() { return std::unique_ptr<Example>(new Searchlights()); }
