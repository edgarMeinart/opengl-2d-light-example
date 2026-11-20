#pragma once
#include "gl.h"

// Procedurally generated, tileable floor materials (albedo + tangent-space normal map).
enum class FloorType { Tiles, Bricks, Metal, Cobble, Count };

struct Material {
    GLuint albedo = 0;
    GLuint normal = 0;
    float worldSize = 8.0f;  // world units covered by one texture repeat
    float specular = 0.2f;   // Blinn-Phong highlight strength
};

class Materials {
public:
    void init();  // generates all floor materials (needs a current GL context)
    void destroy();
    const Material& get(FloorType t) const { return mats_[static_cast<int>(t)]; }

private:
    Material mats_[static_cast<int>(FloorType::Count)];
};

const char* floorName(FloorType t);
