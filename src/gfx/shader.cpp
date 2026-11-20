#include "shader.h"

#include <cstdio>
#include <vector>

static GLuint compile(const char* name, GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 1);
        glGetShaderInfoLog(s, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::fprintf(stderr, "[shader %s] %s compile error:\n%s\n", name,
                     type == GL_VERTEX_SHADER ? "vertex" : "fragment", log.data());
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::load(const char* name, const char* vertexSrc, const char* fragmentSrc) {
    destroy();
    GLuint vs = compile(name, GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compile(name, GL_FRAGMENT_SHADER, fragmentSrc);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }
    id_ = glCreateProgram();
    glAttachShader(id_, vs);
    glAttachShader(id_, fs);
    glLinkProgram(id_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 1);
        glGetProgramInfoLog(id_, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::fprintf(stderr, "[shader %s] link error:\n%s\n", name, log.data());
        destroy();
        return false;
    }
    return true;
}

void Shader::destroy() {
    if (id_) glDeleteProgram(id_);
    id_ = 0;
}

void Shader::set(const char* u, int v) const { glUniform1i(glGetUniformLocation(id_, u), v); }
void Shader::set(const char* u, float v) const { glUniform1f(glGetUniformLocation(id_, u), v); }
void Shader::set(const char* u, float x, float y) const { glUniform2f(glGetUniformLocation(id_, u), x, y); }
void Shader::set(const char* u, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(id_, u), x, y, z);
}
void Shader::set3v(const char* u, const float* data, int count) const {
    glUniform3fv(glGetUniformLocation(id_, u), count, data);
}
void Shader::set4v(const char* u, const float* data, int count) const {
    glUniform4fv(glGetUniformLocation(id_, u), count, data);
}
