#pragma once
#include "gl.h"

// Off-screen colour target: one texture attached to an EXT framebuffer object.
struct RenderTarget {
    GLuint fbo = 0, tex = 0;
    int w = 0, h = 0;

    // internalFormat e.g. GL_RGBA16F_ARB. Returns false if the FBO is incomplete.
    bool create(int width, int height, GLenum internalFormat);
    void destroy();
    // Binds the FBO and sets the viewport to the whole target.
    void bind() const;
};

// Binds the window framebuffer with the given viewport.
void bindDefaultFramebuffer(int width, int height);
