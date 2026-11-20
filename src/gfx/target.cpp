#include "target.h"

#include <cstdio>

bool RenderTarget::create(int width, int height, GLenum internalFormat) {
    destroy();
    w = width;
    h = height;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
        std::fprintf(stderr, "Framebuffer %dx%d incomplete (0x%x)\n", w, h, status);
        return false;
    }
    return true;
}

void RenderTarget::destroy() {
    if (fbo) glDeleteFramebuffersEXT(1, &fbo);
    if (tex) glDeleteTextures(1, &tex);
    fbo = tex = 0;
    w = h = 0;
}

void RenderTarget::bind() const {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glViewport(0, 0, w, h);
}

void bindDefaultFramebuffer(int width, int height) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glViewport(0, 0, width, height);
}
