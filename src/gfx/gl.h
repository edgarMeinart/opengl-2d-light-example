#pragma once
// OpenGL 2.1 (compatibility) headers with the extension prototypes we use:
// GLSL 1.20 shaders, EXT_framebuffer_object, ARB_texture_float.
// macOS exports these from OpenGL.framework; Linux (Mesa/NVIDIA) from libGL.

#if defined(__APPLE__)
#  ifndef GL_SILENCE_DEPRECATION
#    define GL_SILENCE_DEPRECATION
#  endif
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
#else
#  ifndef GL_GLEXT_PROTOTYPES
#    define GL_GLEXT_PROTOTYPES 1
#  endif
#  include <GL/gl.h>
#  include <GL/glext.h>
#endif
