#pragma once
#include <windows.h>
#include <gl/gl.h>
#include <vector>
#include <math.h>
#include "lePoly2D.h"

void DragGLLineCircle(float x,float y,float radius);
void DrawGLCircle(float x,float y,float radius);
void DrawGLLinePolygon(lePoly2D poly);
void DrawGLPolygon(lePoly2D poly);
void DrawGLLine(leVec2f p1,leVec2f p2);
void DrawGLPoint(leVec2f pos,int Size);


void DrawGLLight(leVec2f p, float depth ,float r,float inten);
void DrawGLQuad(leVec2f v1,leVec2f v2,leVec2f v3,leVec2f v4,float d);
void DrawGLQuad(leVec2f p,float size,float depth);