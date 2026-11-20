// The original 2011 demo: silhouette points and shadow rays from the `le` engine,
// drawn as a debug view. Kept as close to the original as the new app shell allows.

#include <cstdio>

#include "../le/LightSystem.h"
#include "../le/leOpenGLRender.h"
#include "example.h"
#include "imgui.h"

namespace {

// Next non-comment, non-empty line. Returns false at end of file.
bool readLine(FILE* file, char* line) {
    do {
        if (!std::fgets(line, 255, file)) return false;
    } while (line[0] == '/' || line[0] == '\n' || line[0] == '\r');
    return true;
}

class Classic : public Example {
public:
    void setup(World&) override { loaded_ = loadWorld("data/map.txt"); }
    Vec2 worldSize() const override { return {50.0f, 50.0f}; }
    bool customRender() const override { return true; }

    void update(World&, const Input& in, float dt, float) override {
        if (!loaded_ || ls_.Lights.size() < 2) return;
        // The original advanced on a 15 ms GLUT timer and followed the mouse while dragging.
        float ticks = dt / 0.015f;
        if (in.hover && in.down[0]) mouse_ = in.mouse;
        ls_.Lights[0].Move(mouse_.x, mouse_.y);
        angle_ += 0.1f * ticks;
        ls_.Lights[1].Move(15, 15, angle_, 25);
        ls_.CheckLightPosition();
        ls_.FindEdges();
    }

    void render(const View& view) override {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        view.apply();
        if (!loaded_) return;
        for (size_t i = 0; i < ls_.Polygons.size(); ++i) {
            glColor3f(0.0f, 1.0f, 0.0f);
            DrawGLLinePolygon(ls_.Polygons[i]);
        }
        for (size_t i = 0; i < ls_.Ellipses.size(); ++i)
            DragGLLineCircle(ls_.Ellipses[i].Position.x, ls_.Ellipses[i].Position.y, ls_.Ellipses[i].Radius);
        for (size_t i = 0; i < ls_.Lights.size(); ++i) {
            const leLight& l = ls_.Lights[i];
            glColor3f(1.0f, 0.0f, 0.0f);
            if (l.type == le_llight) {
                DrawGLPoint(l.Position, 4);
                DragGLLineCircle(l.Position.x, l.Position.y, l.Radius);
            } else {
                DrawGLLine(l.Position, l.v[1]);
                DrawGLLine(l.Position, l.v[2]);
                DrawGLPoint(l.Position, 4);
            }
        }
        glColor3f(0.0f, 0.0f, 0.0f);
        for (size_t i = 0; i < ls_.Edges.size(); ++i) {
            DrawGLPoint(ls_.Edges[i].v1, 4);
            DrawGLPoint(ls_.Edges[i].v2, 4);
            DrawGLLine(ls_.Edges[i].v1, ls_.Edges[i].LightVec1);
            DrawGLLine(ls_.Edges[i].v2, ls_.Edges[i].LightVec2);
        }
    }

    void ui(World&, RenderSettings&) override {
        if (!loaded_) {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "data/map.txt not found.\nRun from the repository root.");
            return;
        }
        ImGui::TextWrapped(
            "The 2011 engine: AABB broad phase, silhouette vertices (black dots) and shadow rays. "
            "No shading, just the geometry.");
        ImGui::Text("Polygons %zu   Circles %zu", ls_.Polygons.size(), ls_.Ellipses.size());
        ImGui::Text("Lights %zu     Edges %zu", ls_.Lights.size(), ls_.Edges.size());
    }

private:
    bool loadWorld(const char* path) {
        FILE* file = std::fopen(path, "rt");
        if (!file) {
            std::perror(path);
            return false;
        }
        char line[256];
        int count = 0;
        float x = 0, y = 0, radius = 0, intensity = 0;
        if (readLine(file, line) && std::sscanf(line, "Polygons %d", &count) == 1) {
            for (int p = 0; p < count; ++p) {
                ls_.AddObject(lePoly2D());
                int vertices = 0;
                if (!readLine(file, line)) break;
                std::sscanf(line, "Polygon %d", &vertices);
                for (int v = 0; v < vertices && readLine(file, line); ++v) {
                    std::sscanf(line, "%f %f", &x, &y);
                    ls_.Polygons[p].AddVertex(leVec2f(x, y));
                }
            }
        }
        if (readLine(file, line) && std::sscanf(line, "Circle %d", &count) == 1)
            for (int c = 0; c < count && readLine(file, line); ++c) {
                std::sscanf(line, "%f %f %f", &x, &y, &radius);
                ls_.AddObject(leEllipse2D(leVec2f(x, y), radius));
            }
        if (readLine(file, line) && std::sscanf(line, "Lights %d", &count) == 1)
            for (int l = 0; l < count && readLine(file, line); ++l) {
                std::sscanf(line, "Light %f %f %f %f", &x, &y, &radius, &intensity);
                ls_.AddLight(leLight(leVec2f(x, y), radius, intensity));
            }
        std::fclose(file);
        // The two flashlights were hard-coded in the original.
        ls_.AddLight(leLight(leVec2f(15, 15), 15, 45, 15));
        ls_.AddLight(leLight(leVec2f(32, 15), 10, 110, 45));
        return true;
    }

    LightSystem ls_;
    Vec2 mouse_{0, 0};
    float angle_ = 0;
    bool loaded_ = false;
};

}  // namespace

std::unique_ptr<Example> makeClassic() { return std::unique_ptr<Example>(new Classic()); }
