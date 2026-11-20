#include "app.h"

#include "gfx/gl.h"  // before GLFW, which must not pull in its own GL headers
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>

#include "imgui.h"

namespace {
constexpr int kBackdropExample = 1;  // "Coloured lights" animates behind the start menu
}

bool App::init(GLFWwindow* window, int startExample) {
    window_ = window;
    if (!renderer_.init()) return false;
    backdrop_ = exampleList()[kBackdropExample].create();
    backdrop_->setup(backdropWorld_);
    lastTime_ = glfwGetTime();
    start(startExample);
    return true;
}

void App::shutdown() { renderer_.destroy(); }

void App::start(int index) {
    current_ = index;
    settings_ = RenderSettings();
    time_ = 0;
    paused_ = false;
    example_.reset();
    world_ = World();
    if (index < 0) return;
    example_ = exampleList()[index].create();
    example_->configure(settings_);
    example_->setup(world_);
}

Input App::gatherInput(const View& view) {
    Input in;
    double mx = 0, my = 0;
    int ww = 1, wh = 1;
    glfwGetCursorPos(window_, &mx, &my);
    glfwGetWindowSize(window_, &ww, &wh);
    // Cursor is in window points; the view is in framebuffer pixels (HiDPI).
    in.mouse = view.toWorld(static_cast<float>(mx * view.fbW / std::max(ww, 1)),
                            static_cast<float>(my * view.fbH / std::max(wh, 1)));
    bool uiOwnsMouse = ImGui::GetIO().WantCaptureMouse;
    in.hover = !uiOwnsMouse && glfwGetWindowAttrib(window_, GLFW_HOVERED);
    const int buttons[2] = {GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT};
    for (int b = 0; b < 2; ++b) {
        bool down = glfwGetMouseButton(window_, buttons[b]) == GLFW_PRESS;
        in.down[b] = down && in.hover;
        in.pressed[b] = in.down[b] && !prevDown_[b];
        prevDown_[b] = down;
    }
    return in;
}

void App::frame() {
    double now = glfwGetTime();
    float dt = static_cast<float>(std::min(now - lastTime_, 0.1));
    lastTime_ = now;
    fps_ = fps_ == 0 ? 60.0f : mixf(fps_, 1.0f / std::max(dt, 1e-4f), 0.05f);

    const bool inMenu = current_ < 0;
    Example& ex = inMenu ? *backdrop_ : *example_;
    World& world = inMenu ? backdropWorld_ : world_;

    int fbW = 1, fbH = 1;
    glfwGetFramebufferSize(window_, &fbW, &fbH);
    View view = View::fit(fbW, fbH, ex.worldSize());
    int winW = 1, winH = 1;
    glfwGetWindowSize(window_, &winW, &winH);
    view.pixelsPerPoint = static_cast<float>(fbW) / std::max(winW, 1);
    Input in = gatherInput(view);
    if (inMenu) in = Input();  // the backdrop runs on autopilot

    if (!paused_) {
        time_ += dt;
        world.beginFrame();
        ex.update(world, in, dt, time_);
        world.updateFlicker(time_);
    }

    if (ex.customRender()) {
        bindDefaultFramebuffer(fbW, fbH);
        ex.render(view);
    } else {
        RenderSettings s = settings_;
        if (inMenu) s.exposure *= 0.6f;  // dim the backdrop so the menu reads well
        renderer_.render(world, view, s);
        if (gizmos_ && !inMenu) renderer_.drawGizmos(world, view);
    }

    if (inMenu) drawMenu();
    else drawPanel();

    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            if (inMenu) glfwSetWindowShouldClose(window_, GLFW_TRUE);
            else start(-1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) panel_ = !panel_;
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && !inMenu) paused_ = !paused_;
        if (ImGui::IsKeyPressed(ImGuiKey_G, false)) gizmos_ = !gizmos_;
    }
}

void App::drawMenu() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(820.0f, vp->Size.x - 40.0f), 0.0f));
    ImGui::SetNextWindowBgAlpha(0.82f);
    ImGui::Begin("##menu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 2.2f);
    ImGui::TextUnformatted("2D Light Engine");
    ImGui::PopFont();
    ImGui::TextDisabled("Dynamic 2D lights and shadows in OpenGL. A 2011 school project, revived.");
    ImGui::Spacing();
    ImGui::SeparatorText("Examples");

    const std::vector<ExampleInfo>& list = exampleList();
    if (ImGui::BeginTable("examples", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX)) {
        for (size_t i = 0; i < list.size(); ++i) {
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(i));
            char label[96];
            std::snprintf(label, sizeof(label), "%zu.  %s", i + 1, list[i].title);
            if (ImGui::Button(label, ImVec2(-FLT_MIN, ImGui::GetFrameHeight() * 1.5f))) start(static_cast<int>(i));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", list[i].description);
            ImGui::PopStyleColor();
            ImGui::Spacing();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::Button("Quit", ImVec2(120, 0))) glfwSetWindowShouldClose(window_, GLFW_TRUE);
    ImGui::SameLine();
    ImGui::TextDisabled("  Esc: back to menu   F1: panel   Space: pause   G: gizmos");
    ImGui::End();
}

void App::drawPanel() {
    if (!panel_) return;
    const ExampleInfo& info = exampleList()[current_];
    const float width = ImGui::GetFontSize() * 25.0f;
    ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0), ImVec2(width, ImGui::GetMainViewport()->Size.y - 24));
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin(info.title, nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoCollapse);
    ImGui::PushItemWidth(-ImGui::GetFontSize() * 9.5f);  // leave room for the labels
    int next = current_;  // applied after End() so the example outlives its own UI
    bool restart = false;
    if (ImGui::Button("< Menu")) next = -1;
    ImGui::SameLine();
    if (ImGui::Button("Restart")) restart = true;
    ImGui::SameLine();
    ImGui::Checkbox("Pause", &paused_);
    ImGui::SameLine();
    ImGui::TextDisabled("%.0f fps", fps_);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("%s", info.controls);
    ImGui::PopTextWrapPos();

    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) example_->ui(world_, settings_);

    if (!example_->customRender() && ImGui::CollapsingHeader("Rendering")) {
        RenderSettings& s = settings_;
        ImGui::Checkbox("Shadows", &s.shadows);
        ImGui::SameLine();
        ImGui::Checkbox("Soft", &s.softShadows);
        ImGui::SameLine();
        ImGui::Checkbox("Gizmos (G)", &gizmos_);
        const char* quality[] = {"Low", "Medium", "High"};
        ImGui::Combo("Penumbra quality", &s.penumbraQuality, quality, 3);
        ImGui::SliderFloat("Penumbra scale", &s.penumbraScale, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Bump", &s.bump, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Specular", &s.specular, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Ambient", &s.ambient, 0.0f, 5.0f, "%.2f");
        ImGui::SliderFloat("Exposure", &s.exposure, 0.1f, 4.0f, "%.2f");
        ImGui::Checkbox("Bloom", &s.bloom);
        ImGui::SameLine();
        ImGui::Checkbox("Vignette", &s.vignette);
        ImGui::SliderFloat("Bloom strength", &s.bloomStrength, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Bloom threshold", &s.bloomThreshold, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Light buffer res", &s.resolutionScale, 0.25f, 2.0f, "%.2f");
        ImGui::SetItemTooltip("Light buffer texels per window point; 2 = full Retina resolution");
        ImGui::TextDisabled("%zu lights, %zu line lights, %zu occluders", world_.lights.size() + world_.tempLights.size(),
                            world_.lineLights.size(), world_.occluders.size());
    }
    ImGui::PopItemWidth();
    ImGui::End();
    if (restart || next != current_) start(next);
}
