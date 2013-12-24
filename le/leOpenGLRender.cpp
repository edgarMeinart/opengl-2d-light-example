#include "StdAfx.h"
#include "leOpenGLRender.h"
#define PI  3.14f
void DragGLLineCircle(float x,float y,float radius)
{
	glPushMatrix();
	glBegin(GL_LINE_LOOP);
	for (float angle = 0.0f; angle <= 2 * 3.14f ; angle += ((2*3.14f) / 25))
	{
		glVertex3f(radius * cos(angle) + x,radius * sin(angle) + y,1.0f);
	}
	glEnd();

	glPopMatrix();
}
void DrawGLCircle(float x,float y,float radius)
{
	glPushMatrix();
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(x,y,1.0f);
	for (float angle = 0.0f; angle <= 2 * 3.14f ; angle += ((2*3.14f) / 25))
	{
		glVertex3f(radius * cos(angle) + x,radius * sin(angle) + y,1.0f);
	}
	glVertex3f(x + radius,y,1.0f);
	glEnd();
	glPopMatrix();
}
void DrawGLLinePolygon(lePoly2D poly)
{
	glPushMatrix();
	glBegin(GL_LINE_LOOP);
	for (int v_num = 0 ; v_num < poly.VertNum; v_num++)
	{
		glVertex3f(poly.vert[v_num].x,poly.vert[v_num].y,1.0f);
	}
	glEnd();
	glPopMatrix();
}
void DrawGLPolygon(lePoly2D poly)
{
	glPushMatrix();
	glBegin(GL_TRIANGLE_STRIP);
	for (int v_num = 0 ; v_num < poly.VertNum; v_num++)
	{
		glVertex3f(poly.vert[v_num].x,poly.vert[v_num].y,1.0f);
	}
	glEnd();
	glPopMatrix();
}

void DrawGLLine(leVec2f p1,leVec2f p2)
{
	glPushMatrix();
	glBegin(GL_LINES);
	glVertex3f(p1.x,p1.y,1.0f);
	glVertex3f(p2.x,p2.y,1.0f);
	glEnd();
	glPopMatrix();
}
void DrawGLPoint(leVec2f pos,int Size)
{
	glPushMatrix();
	glPointSize((float)Size);
	glBegin(GL_POINTS);
	glVertex3f(pos.x,pos.y,0.0f);
	glEnd();
	glPopMatrix();
}
void DrawGLLight(leVec2f p, float depth ,float r,float inten)
{
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(0.0f, 0.0f, 0.0f, inten);
	glVertex3f(p.x,p.y, depth);
	glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
	for (float angle = 0.0f; angle <= PI*2; angle += ((PI*2)/32) )
		glVertex3f(r * cosf(angle) + p.x, r * sinf(angle) + p.y, depth);  
	glVertex3f(p.x + r, p.y, depth);
	glEnd();
}
void DrawGLQuad(leVec2f v1,leVec2f v2,leVec2f v3,leVec2f v4,float d)
{
	glPushMatrix();
	glColor4f(0.1f,0.1f,0.1f,0.0f);
	glBegin(GL_QUADS);
	glVertex3f(v1.x,v1.y,d);
	glVertex3f(v2.x,v2.y,d);
	glVertex3f(v3.x,v3.y,d);
	glVertex3f(v4.x,v4.y,d);
	glEnd();
	glPopMatrix();
}
void DrawGLQuad(leVec2f p,float size,float depth)
{
	//glColor4f(1.0f,1.0f,1.0f,1.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f,0.0f); glVertex3f(p.x + size,p.x + size,depth);
	glTexCoord2f(1.0f,0.0f); glVertex3f(p.x + size,p.x - size,depth);
	glTexCoord2f(1.0f,1.0f); glVertex3f(p.x - size,p.x - size,depth);
	glTexCoord2f(0.0f,1.0f); glVertex3f(p.x - size,p.x + size,depth);
	glEnd();	
}