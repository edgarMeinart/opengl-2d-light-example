#include "StdAfx.h"
#include "leLight.h"


leLight::leLight(void)
{
}

leLight::~leLight(void)
{
}
void leLight::Move(float x, float y)
{
	Sleep = false;
	Position.x = x;
	Position.y = y;
	if (type == le_flight)
	{
		v[0] = Position;
		v[1].x = Position.x + cos( DtoR(Angle - (s_Angle / 2)) ) * Radius;
		v[1].y = Position.x + sin( DtoR(Angle - (s_Angle / 2)) ) * Radius;

		v[2].x = Position.x + cos( DtoR(Angle + (s_Angle / 2)) ) * Radius;
		v[2].y = Position.x + sin( DtoR(Angle + (s_Angle / 2)) ) * Radius;
	}
}
void leLight::Move(float x, float y,float a,float sa)
{
	Position.x = x;
	Position.y = y;
	Angle = a;
	s_Angle = sa;
	if (type == le_flight)
	{
		v[0] = Position;
		v[1].x = Position.x + cos( DtoR(Angle - (s_Angle / 2)) ) * Radius;
		v[1].y = Position.x + sin( DtoR(Angle - (s_Angle / 2)) ) * Radius;

		v[2].x = Position.x + cos( DtoR(Angle + (s_Angle / 2)) ) * Radius;
		v[2].y = Position.x + sin( DtoR(Angle + (s_Angle / 2)) ) * Radius;
	}
}
leLight::leLight(leVec2f p, float r, float angle, float sangle)
{

	type = le_flight;
	Position = p;
	Radius = r;
	Angle = angle;
	s_Angle = sangle;
	
	v[0] = p;
	v[1].x = p.x + cos( DtoR(angle - (sangle / 2)) ) * r;
	v[1].y = p.y + sin( DtoR(angle - (sangle / 2)) ) * r;

	v[2].x = p.x + cos( DtoR(angle + (sangle / 2)) ) * r;
	v[2].y = p.y + sin( DtoR(angle + (sangle / 2)) ) * r;

}
leAABB leLight::AABB()
{
	leAABB aabb;
	if (type == le_llight)
	{
	aabb.lowerBound = Position - leVec2f(Radius,Radius);
	aabb.upperBound = Position + leVec2f(Radius,Radius);
	}
	if (type == le_flight)
	{
		leVec2f l,u;
		l = v[0];
		u = l;
		for (int i = 1; i < 3; i++)
		{
			l.x = std::min(l.x,v[i].x);	
			l.y = std::min(l.y,v[i].y);	

			u.x = std::max(u.x,v[i].x);	
			u.y = std::max(u.y,v[i].y);
		}
		aabb.lowerBound = l;
		aabb.upperBound = u;
	}
	return aabb;
}