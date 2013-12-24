#include "StdAfx.h"
#include "lePoly2D.h"

lePoly2D::lePoly2D()
{
}

lePoly2D::~lePoly2D()
{
}

void lePoly2D::AddVertex(leVec2f v)
{
	vert.push_back(v);
	VertNum = (int)vert.size();
}
void lePoly2D::RemoveVertex(int num)
{
	vert.erase(vert.begin() + num);
	VertNum = (int)vert.size();
}
leAABB lePoly2D::AABB()
{
	leVec2f l,u;
	leAABB aabb;
	l = vert[0];
	u = l;

	for (int i = 1 ; i < (int) vert.size(); i++)
	{
		l.x = std::min(l.x,vert[i].x);
		l.y = std::min(l.y,vert[i].y);

		u.x = std::max(u.x,vert[i].x);
		u.y = std::max(u.y,vert[i].y);
	}
	aabb.lowerBound = l;
	aabb.upperBound = u;

	return aabb;
}


leEllipse2D::leEllipse2D()
{
}
leEllipse2D::~leEllipse2D()
{
}
leAABB leEllipse2D::AABB()
{
	leAABB aabb;
	aabb.lowerBound = Position - leVec2f(Radius,Radius);
	aabb.upperBound = Position + leVec2f(Radius,Radius);
	return aabb;
}