#pragma once
// A self-contained demo scene selectable from the start menu.

#include <memory>
#include <vector>

#include "../core/math.h"
#include "../gfx/renderer.h"
#include "../world/world.h"

struct Input {
    Vec2 mouse;             // world coordinates
    bool hover = false;     // mouse is over the scene (not over UI)
    bool down[2] = {false, false};     // left, right held
    bool pressed[2] = {false, false};  // left, right went down this frame
};

class Example {
public:
    virtual ~Example() = default;
    // Builds the scene. Called once when the example starts.
    virtual void setup(World& w) = 0;
    // Advances the simulation. `w.beginFrame()` has already cleared transient lists.
    virtual void update(World& w, const Input& in, float dt, float time) = 0;
    // Example-specific controls inside the settings panel.
    virtual void ui(World&, RenderSettings&) {}
    // Adjusts the default render settings when the example starts.
    virtual void configure(RenderSettings&) {}
    // Visible world rectangle starting at the origin.
    virtual Vec2 worldSize() const { return {80.0f, 50.0f}; }

    // Examples that bypass the lighting renderer (the original 2011 debug view).
    virtual bool customRender() const { return false; }
    virtual void render(const View&) {}
};

struct ExampleInfo {
    const char* title;
    const char* description;
    const char* controls;
    std::unique_ptr<Example> (*create)();
};

const std::vector<ExampleInfo>& exampleList();
