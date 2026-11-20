#pragma once
#include "gl.h"

// GLSL 1.20 program: compile/link with logged errors and by-name uniform setters.
class Shader {
public:
    bool load(const char* name, const char* vertexSrc, const char* fragmentSrc);
    void destroy();
    void use() const { glUseProgram(id_); }

    void set(const char* uniform, int v) const;
    void set(const char* uniform, float v) const;
    void set(const char* uniform, float x, float y) const;
    void set(const char* uniform, float x, float y, float z) const;
    void set3v(const char* uniform, const float* data, int count) const;
    void set4v(const char* uniform, const float* data, int count) const;

private:
    GLuint id_ = 0;
};
