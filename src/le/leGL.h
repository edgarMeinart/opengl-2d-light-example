#pragma once
// Platform OpenGL + GLUT headers (glut.h pulls in gl.h).
#if defined(__APPLE__)
#  ifndef GL_SILENCE_DEPRECATION
#    define GL_SILENCE_DEPRECATION // OpenGL/GLUT still ship on macOS but are marked deprecated
#  endif
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
