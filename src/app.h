#pragma once
// Application shell: start menu, example lifecycle, input, settings panel.

#include <memory>

#include "examples/example.h"
#include "gfx/renderer.h"
#include "world/world.h"

struct GLFWwindow;

class App {
public:
    // startExample: index into exampleList(), or -1 for the start menu.
    bool init(GLFWwindow* window, int startExample);
    void frame();  // update + render + UI for one frame (caller swaps buffers)
    void shutdown();

private:
    void start(int index);
    Input gatherInput(const View& view);
    void drawMenu();
    void drawPanel();

    GLFWwindow* window_ = nullptr;
    Renderer renderer_;
    RenderSettings settings_;

    int current_ = -1;  // -1 = start menu
    World world_;
    std::unique_ptr<Example> example_;
    // Animated scene behind the start menu.
    World backdropWorld_;
    std::unique_ptr<Example> backdrop_;

    double lastTime_ = 0;
    float time_ = 0;
    float fps_ = 0;
    bool panel_ = true;
    bool gizmos_ = false;
    bool paused_ = false;
    bool prevDown_[2] = {false, false};
};
