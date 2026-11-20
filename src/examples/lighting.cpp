// Lighting fundamentals: soft shadows, coloured lights, bump mapping and a sandbox.

#include <algorithm>

#include "example.h"
#include "imgui.h"

namespace {

const Vec3 kStone{0.16f, 0.15f, 0.14f};
const Vec3 kWood{0.24f, 0.15f, 0.08f};

// Eases a light towards the mouse while hovering, otherwise towards `idle`.
void follow(Light& l, const Input& in, Vec2 idle, float dt) {
    Vec2 target = in.hover ? in.mouse : idle;
    l.pos = lerp(l.pos, target, 1.0f - std::exp(-dt * 14.0f));
}

void colorEdit(const char* label, Vec3& c) { ImGui::ColorEdit3(label, &c.x, ImGuiColorEditFlags_NoInputs); }

// ---------------------------------------------------------------- Soft shadows

class SoftShadows : public Example {
public:
    void setup(World& w) override {
        w.floor = FloorType::Tiles;
        w.ambient = Vec3(0.02f, 0.02f, 0.025f);
        Light& l = w.addLight(Vec2(40, 25), Vec3(1.0f, 0.88f, 0.7f), 2.4f, 48.0f);
        l.size = 1.4f;
        l.height = 4.0f;
        w.addCircle(Vec2(20, 15), 1.2f, kStone);
        w.addCircle(Vec2(28, 35), 1.6f, kStone);
        w.addCircle(Vec2(56, 13), 1.0f, kStone);
        w.addCircle(Vec2(61, 36), 2.2f, kStone);
        w.addCircle(Vec2(47, 31), 0.7f, kStone);
        w.addBox(Vec2(32, 18), Vec2(2.0f, 2.0f), 0.3f, kWood);
        w.addBox(Vec2(51, 22), Vec2(0.6f, 3.5f), 0.8f, kWood);
        w.addWall(Vec2(9, 41), Vec2(24, 45), 0.9f, kStone);
        w.addWall(Vec2(63, 7), Vec2(71, 19), 0.9f, kStone);
        // A crate stack close to a wall: contact-hardened shadows.
        w.addWall(Vec2(10, 6), Vec2(10, 30), 0.9f, kStone);
        w.addBox(Vec2(12.5f, 22), Vec2(1.0f, 1.0f), 0.1f, kWood);
        w.addBox(Vec2(12.2f, 18), Vec2(0.8f, 0.8f), -0.2f, kWood);
    }

    void update(World& w, const Input& in, float dt, float time) override {
        Vec2 idle = Vec2(40, 25) + Vec2(std::cos(time * 0.4f) * 18.0f, std::sin(time * 0.6f) * 10.0f);
        follow(w.lights[0], in, idle, dt);
    }

    void ui(World& w, RenderSettings& s) override {
        Light& l = w.lights[0];
        ImGui::SliderFloat("Light size", &l.size, 0.0f, 4.0f, "%.2f");
        ImGui::SliderFloat("Reach", &l.radius, 10.0f, 80.0f, "%.0f");
        ImGui::SliderFloat("Intensity", &l.intensity, 0.2f, 6.0f, "%.1f");
        colorEdit("Colour", l.color);
        ImGui::Checkbox("Soft shadows", &s.softShadows);
        ImGui::TextDisabled("Bigger light = wider penumbra.\nShadows sharpen near the occluder.");
    }
};

// ---------------------------------------------------------------- Coloured lights

class ColoredLights : public Example {
public:
    void setup(World& w) override {
        w.floor = FloorType::Tiles;
        w.ambient = Vec3(0.012f, 0.012f, 0.015f);
        w.addCircle(Vec2(40, 25), 2.5f, kStone, 28);
        for (int i = 0; i < 10; ++i) w.addCircle(Vec2(40, 25) + fromAngle(kTau * i / 10) * 9.0f, 0.7f, kStone);
        w.addBox(Vec2(14, 12), Vec2(2, 2), 0.4f, kStone);
        w.addBox(Vec2(66, 38), Vec2(2.5f, 1.2f), -0.3f, kStone);
        w.addBox(Vec2(64, 10), Vec2(1.2f, 1.2f), 0.9f, kStone);
        w.addBox(Vec2(15, 39), Vec2(1.5f, 2.5f), 0.2f, kStone);
        rebuild(w);
    }

    void update(World& w, const Input& in, float dt, float time) override {
        angle_ += dt * speed_;
        for (int i = 0; i < count_; ++i) {
            float a = angle_ + kTau * i / count_;
            float r = 15.0f + 4.0f * std::sin(time * 0.7f + i * 1.3f);
            w.lights[i].pos = Vec2(40, 25) + fromAngle(a) * r;
        }
        if (mouseLight_) {
            Light& m = w.lights[count_];
            m.intensity = in.hover ? 1.6f : 0.0f;
            if (in.hover) m.pos = in.mouse;
        }
    }

    void ui(World& w, RenderSettings&) override {
        bool changed = ImGui::SliderInt("Lights", &count_, 1, 8);
        changed |= ImGui::Checkbox("White light at the mouse", &mouseLight_);
        if (changed) rebuild(w);
        ImGui::SliderFloat("Orbit speed", &speed_, -2.0f, 2.0f, "%.2f");
        ImGui::TextDisabled("Overlapping coloured lights add up;\nshadows take the colour of the other lights.");
    }

private:
    void rebuild(World& w) {
        w.lights.clear();
        for (int i = 0; i < count_; ++i) {
            Light& l = w.addLight(Vec2(40, 25), hueToRgb(static_cast<float>(i) / count_), 2.0f, 42.0f);
            l.size = 0.6f;
        }
        if (mouseLight_) {
            Light& m = w.addLight(Vec2(40, 25), Vec3(1, 1, 1), 0.0f, 30.0f);
            m.size = 0.4f;
        }
    }

    int count_ = 3;
    float speed_ = 0.35f;
    float angle_ = 0;
    bool mouseLight_ = false;
};

// ---------------------------------------------------------------- Bump mapping

class BumpMapping : public Example {
public:
    void configure(RenderSettings& s) override { s.bump = 1.6f; }

    void setup(World& w) override {
        w.floor = FloorType::Bricks;
        w.ambient = Vec3(0.012f, 0.012f, 0.014f);
        Light& l = w.addLight(Vec2(40, 25), Vec3(1.0f, 0.8f, 0.55f), 2.6f, 34.0f);
        l.height = 2.5f;
        l.size = 0.5f;
        Light& b = w.addLight(Vec2(20, 20), Vec3(0.35f, 0.55f, 1.0f), 1.6f, 26.0f);
        b.height = 1.2f;
        b.size = 0.4f;
        w.addCircle(Vec2(24, 32), 1.2f, kStone);
        w.addCircle(Vec2(58, 18), 1.2f, kStone);
    }

    void update(World& w, const Input& in, float dt, float time) override {
        Vec2 idle = Vec2(40, 25) + Vec2(std::cos(time * 0.5f), std::sin(time * 0.5f)) * 14.0f;
        follow(w.lights[0], in, idle, dt);
        w.lights[1].intensity = secondLight_ ? 1.6f : 0.0f;
        w.lights[1].pos = Vec2(40, 25) + Vec2(std::cos(-time * 0.3f) * 28.0f, std::sin(-time * 0.3f) * 16.0f);
    }

    void ui(World& w, RenderSettings& s) override {
        int floor = static_cast<int>(w.floor);
        const char* names[] = {floorName(FloorType::Tiles), floorName(FloorType::Bricks), floorName(FloorType::Metal),
                               floorName(FloorType::Cobble)};
        if (ImGui::Combo("Surface", &floor, names, IM_ARRAYSIZE(names))) w.floor = static_cast<FloorType>(floor);
        ImGui::SliderFloat("Bump strength", &s.bump, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Specular", &s.specular, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Light height", &w.lights[0].height, 0.3f, 10.0f, "%.1f");
        ImGui::Checkbox("Second (blue) light", &secondLight_);
        ImGui::TextDisabled("Low lights rake across the surface\nand exaggerate the relief.");
    }

private:
    bool secondLight_ = true;
};

// ---------------------------------------------------------------- Sandbox

class Sandbox : public Example {
public:
    void setup(World& w) override {
        w.floor = FloorType::Tiles;
        w.ambient = Vec3(0.03f, 0.03f, 0.035f);
        w.addBox(Vec2(30, 25), Vec2(2, 2), 0.2f, kStone);
        w.addBox(Vec2(50, 20), Vec2(1.5f, 3), -0.4f, kStone);
        w.addCircle(Vec2(42, 34), 1.5f, kStone);
        Light& l = w.addLight(Vec2(40, 20), Vec3(1.0f, 0.85f, 0.6f), 1.8f, 30.0f);
        l.size = 0.8f;
    }

    void update(World& w, const Input& in, float, float) override {
        if (!in.hover) {
            dragging_ = -1;
            return;
        }
        if (in.pressed[0]) {
            dragging_ = lightAt(w, in.mouse);
            if (dragging_ < 0) {
                Light& l = w.addLight(in.mouse, color_, intensity_, radius_);
                l.size = size_;
                l.flicker = static_cast<Flicker>(flicker_);
                l.flickerRate = 1.0f;
                if (spot_) {
                    l.cone = true;
                    l.coneAngle = 0.45f;
                    l.dir = std::atan2(25.0f - in.mouse.y, 40.0f - in.mouse.x);
                }
                if (randomHue_) color_ = hueToRgb(rng_.uniform());
                dragging_ = static_cast<int>(w.lights.size()) - 1;
            }
        }
        if (!in.down[0]) dragging_ = -1;
        if (dragging_ >= 0) w.lights[dragging_].pos = in.mouse;

        if (in.pressed[1]) {
            int hit = -1;
            for (size_t i = 0; i < w.occluders.size(); ++i)
                if (pointInOccluder(w.occluders[i], in.mouse)) hit = static_cast<int>(i);
            if (hit >= 0) w.occluders.erase(w.occluders.begin() + hit);
            else if (circle_) w.addCircle(in.mouse, boxSize_, kStone);
            else w.addBox(in.mouse, Vec2(boxSize_, boxSize_), rng_.range(0, kPi), kStone);
        }
    }

    void ui(World& w, RenderSettings&) override {
        ImGui::SeparatorText("New light (left click)");
        colorEdit("Colour", color_);
        ImGui::SameLine();
        ImGui::Checkbox("Random hue", &randomHue_);
        ImGui::SliderFloat("Intensity", &intensity_, 0.2f, 5.0f, "%.1f");
        ImGui::SliderFloat("Reach", &radius_, 5.0f, 60.0f, "%.0f");
        ImGui::SliderFloat("Size", &size_, 0.0f, 3.0f, "%.2f");
        const char* modes[] = {"Steady", "Candle", "Pulse", "Blink", "Broken neon", "Strobe"};
        ImGui::Combo("Flicker", &flicker_, modes, IM_ARRAYSIZE(modes));
        ImGui::Checkbox("Spotlight (aims at centre)", &spot_);
        ImGui::SeparatorText("Occluders (right click)");
        ImGui::SliderFloat("Block size", &boxSize_, 0.3f, 5.0f, "%.1f");
        ImGui::Checkbox("Round", &circle_);
        if (ImGui::Button("Clear lights")) w.lights.clear();
        ImGui::SameLine();
        if (ImGui::Button("Clear blocks")) w.occluders.clear();
        ImGui::TextDisabled("%zu lights, %zu blocks", w.lights.size(), w.occluders.size());
    }

private:
    static int lightAt(const World& w, Vec2 p) {
        for (size_t i = 0; i < w.lights.size(); ++i)
            if (length(w.lights[i].pos - p) < 1.0f) return static_cast<int>(i);
        return -1;
    }

    Vec3 color_{1.0f, 0.6f, 0.3f};
    float intensity_ = 1.8f, radius_ = 28.0f, size_ = 0.6f, boxSize_ = 1.5f;
    int flicker_ = 0;
    bool spot_ = false, randomHue_ = true, circle_ = false;
    int dragging_ = -1;
    Rng rng_{99};
};

}  // namespace

std::unique_ptr<Example> makeSoftShadows() { return std::unique_ptr<Example>(new SoftShadows()); }
std::unique_ptr<Example> makeColoredLights() { return std::unique_ptr<Example>(new ColoredLights()); }
std::unique_ptr<Example> makeBumpMapping() { return std::unique_ptr<Example>(new BumpMapping()); }
std::unique_ptr<Example> makeSandbox() { return std::unique_ptr<Example>(new Sandbox()); }
