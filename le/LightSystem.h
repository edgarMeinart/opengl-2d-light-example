#pragma once
#include "leMath.h"
#include "leLight.h"
#include "lePoly2D.h"
 // DODELAT PROVERKU POZICII SVETA NA NALICIJE OBJEKTOV I
//NAHOZDENIJE KRAINIH TOCEK!!!!

bool VertexIsEdge(leVec2f prev_point,
				  leVec2f curr_point,
				  leVec2f next_point,
				  leVec2f light_pos);
leEdge GetEdgeFromPolygon(lePoly2D poly,leVec2f l_pos);
bool Intersects(leAABB aabb1,leAABB aabb2);


class LightSystem
{
public:
	std::vector < leLight > Lights;
	std::vector < lePoly2D > Polygons;
	std::vector < leEllipse2D > Ellipses;
	std::vector < leEdge > Edges;
public:
	LightSystem(void);
	~LightSystem(void);

	void AddLight(leLight light);
	void AddObject(lePoly2D poly);
	void AddObject(leEllipse2D ellipse);

	void DeleteLight(int lnum);
	void DeleteObject(leObjectType type,int num);

	void CheckLightPosition();
	void FindEdges();

};
