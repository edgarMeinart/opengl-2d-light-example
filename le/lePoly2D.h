#pragma once
#include <vector>
#include "leMath.h"




struct lePoly2D
{
public:

	int VertNum; //Количество вершин
	leObjectID ID;
	std::vector <leVec2f> vert; //Вершины

	lePoly2D();
	~lePoly2D();
	void AddVertex(leVec2f v);
	void RemoveVertex(int num);

	leAABB AABB();
};

struct leEllipse2D
{
public:
	leVec2f Position;
	float Radius;
	leObjectID ID;
	leEllipse2D();
	~leEllipse2D();
	leEllipse2D(leVec2f Pos, float R){Position = Pos; Radius = R;}

	leAABB AABB();

};