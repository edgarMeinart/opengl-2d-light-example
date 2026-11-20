// 2D Light Engine: GLFW window, OpenGL 2.1 context and Dear ImGui setup.

#include "gfx/gl.h"  // before GLFW, which must not pull in its own GL headers
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

namespace {

void usage(const char* argv0) {
    std::printf("Usage: %s [--example N] [--list]\n\n", argv0);
    std::printf("  --example N   start example N directly (1-based, see --list)\n");
    std::printf("  --list        list the examples and exit\n");
}

bool hasExtension(const char* name) {
    const char* all = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    return all && std::strstr(all, name);
}

void setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    style.FontSizeBase = 16.0f;
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.WindowPadding = ImVec2(14, 12);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 7);
    ImVec4* c = style.Colors;
    c[ImGuiCol_Button] = ImVec4(0.20f, 0.24f, 0.32f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.95f, 0.62f, 0.25f, 0.85f);
    c[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.72f, 0.35f, 1.0f);
    c[ImGuiCol_Header] = ImVec4(0.20f, 0.24f, 0.32f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.95f, 0.62f, 0.25f, 0.6f);
    c[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.7f, 0.3f, 1.0f);
    c[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.65f, 0.3f, 1.0f);
    c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.14f, 0.19f, 1.0f);
    c[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.09f, 1.0f);
}

}  // namespace

int main(int argc, char** argv) {
    int startExample = -1;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--list")) {
            const std::vector<ExampleInfo>& list = exampleList();
            for (size_t e = 0; e < list.size(); ++e) std::printf("%2zu  %s\n", e + 1, list[e].title);
            return 0;
        } else if (!std::strcmp(argv[i], "--example") && i + 1 < argc) {
            startExample = std::atoi(argv[++i]) - 1;
            if (startExample < 0 || startExample >= static_cast<int>(exampleList().size())) {
                std::fprintf(stderr, "No example %s (see --list)\n", argv[i]);
                return 1;
            }
        } else {
            usage(argv[0]);
            return !std::strcmp(argv[i], "--help") || !std::strcmp(argv[i], "-h") ? 0 : 1;
        }
    }

    glfwSetErrorCallback([](int code, const char* msg) { std::fprintf(stderr, "GLFW error %d: %s\n", code, msg); });
    if (!glfwInit()) return 1;
    // Legacy 2.1 context: fixed-function matrices + GLSL 1.20 (also the macOS default).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* window = glfwCreateWindow(1280, 800, "2D Light Engine", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::printf("OpenGL %s, %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
    for (const char* ext : {"GL_EXT_framebuffer_object", "GL_ARB_texture_float"})
        if (!hasExtension(ext)) {
            std::fprintf(stderr, "Missing required OpenGL extension %s\n", ext);
            return 1;
        }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.Fonts->AddFontDefaultVector();
    setupStyle();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    App app;
    if (!app.init(window, startExample)) {
        std::fprintf(stderr, "Renderer initialisation failed\n");
        return 1;
    }
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        app.frame();
        ImGui::Render();
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    app.shutdown();
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
