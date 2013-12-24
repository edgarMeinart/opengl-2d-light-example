#include "StdAfx.h"
#include "LightSystem.h"
#include <math.h> 


bool VertexIsEdge(leVec2f prev_point,
				  leVec2f curr_point,
				  leVec2f next_point,
				  leVec2f light_pos)
{
	leVec2f v1 = leVec2f(curr_point - prev_point).Normal();
	leVec2f v2 = leVec2f(next_point - curr_point).Normal();
	leVec2f v3 = (light_pos - curr_point);
	if ((dot(v1,v3) * dot(v2,v3)) < 0.0f)
		return true;
	else 
		return false;
}
bool Intersects(leAABB aabb1,leAABB aabb2)
{
	if  ( 
		aabb1.lowerBound.x > aabb2.upperBound.x ||
		aabb1.upperBound.x < aabb2.lowerBound.x ||
		aabb1.lowerBound.y > aabb2.upperBound.y ||
		aabb1.upperBound.y < aabb2.lowerBound.y
		)
	{
		return false;
	}
	return true;
}
leEdge GetEdgeFromPolygon(lePoly2D poly,leVec2f l_pos)
{
	int EdgeNum = 1;
	int Num = 0;
	int NextVertex = 0;
	int PrevVertex = 0;
	int CurrentVertex = 0;
	leEdge edge;
	for (int v_num = 0; v_num < poly.VertNum; v_num++)
	{
		if (v_num == (int) poly.VertNum - 1)
		{
			NextVertex = 0;
			PrevVertex = v_num - 1;
			CurrentVertex = v_num;
		}
		else if (v_num == 0)
		{
			NextVertex = v_num + 1;
			CurrentVertex = v_num;
			PrevVertex = poly.VertNum - 1;
		}
		else
		{
			NextVertex = v_num + 1;
			PrevVertex = v_num - 1;
			CurrentVertex = v_num;

		}
		leVec2f curr_v = poly.vert[CurrentVertex];
		leVec2f prev_v = poly.vert[PrevVertex];
		leVec2f next_v = poly.vert[NextVertex];

		if (VertexIsEdge(prev_v,curr_v,next_v,l_pos)) // HERE FIRST VERTEX
		{
			edge.v1 = curr_v;
			edge.LightVec1 = edge.v1 + (edge.v1 - l_pos) * leVec2f(5,5);
			Num = v_num + 1;
			break;			
		}
	}
	for (int v_num = Num; v_num < poly.VertNum; v_num++)
	{
		if (v_num == (int) poly.VertNum - 1)
		{
			NextVertex = 0;
			PrevVertex = v_num - 1;
			CurrentVertex = v_num;
		}
		else if (v_num == 0)
		{
			NextVertex = v_num + 1;
			CurrentVertex = v_num;
			PrevVertex = poly.VertNum - 1;
		}
		else
		{
			NextVertex = v_num + 1;
			PrevVertex = v_num - 1;
			CurrentVertex = v_num;

		}
		leVec2f curr_v = poly.vert[CurrentVertex];
		leVec2f prev_v = poly.vert[PrevVertex];
		leVec2f next_v = poly.vert[NextVertex];

		if (VertexIsEdge(prev_v,curr_v,next_v,l_pos)) // HERE SECOND VERTEX
		{
			edge.v2 = curr_v;
			edge.LightVec2 = edge.v2 +(edge.v2 - l_pos) * leVec2f(15,15);
			break;			
		}
	}
	return edge;
}


LightSystem::LightSystem(void)
{
}

LightSystem::~LightSystem(void)
{
}
void LightSystem::AddLight(leLight light)
{
	Lights.push_back(light);
}
void LightSystem::AddObject(lePoly2D poly)
{
	Polygons.push_back(poly);
	Polygons[Polygons.size() - 1].ID.type = lePolygon;
	Polygons[Polygons.size() - 1].ID.ID = Polygons.size() - 1;
}
void LightSystem::AddObject(leEllipse2D ellipse)
{
	Ellipses.push_back(ellipse);
	Ellipses[Ellipses.size() - 1].ID.type = leEllipse;
	Ellipses[Ellipses.size() - 1].ID.ID = Ellipses.size() - 1;
}
void LightSystem::DeleteLight(int lnum)
{
	Lights.erase(Lights.begin() + lnum);
}
void LightSystem::DeleteObject(leObjectType type,int num)
{
		switch(type)
		{
		case lePolygon:
			Polygons.erase(Polygons.begin() + num );
			for (int i = 0 ; i < (int) Polygons.size(); i++)
				Polygons[i].ID.ID = i;
			break;
		case leEllipse:
			Ellipses.erase(Ellipses.begin() + num);
			for (int i = 0 ; i < (int) Ellipses.size(); i++)
				Ellipses[i].ID.ID = i;
			break;
		}
}
void LightSystem::CheckLightPosition()
{
	int Light_Count = (int) Lights.size();
	int Polygon_Count = (int) Polygons.size();
	int Ellipses_count = (int) Ellipses.size();
	for (int l_num = 0; l_num < Light_Count;  l_num++)	
	{
		Lights[l_num].objID.clear();
		for (int e_num = 0; e_num < Ellipses_count; e_num++)
		{
			if ( Intersects( Lights[l_num].AABB(),Ellipses[e_num].AABB() ) )
			{
				Lights[l_num].objID.push_back(Ellipses[e_num].ID);
			}
		}
		for (int p_num = 0; p_num < Polygon_Count; p_num++)
		{
			if ( Intersects( Lights[l_num].AABB(),Polygons[p_num].AABB() ) )
			{
				Lights[l_num].objID.push_back(Polygons[p_num].ID);
			}
		}
	}



}
void LightSystem::FindEdges()
{
	int Light_Count = (int) Lights.size();
	int Polygon_Count = (int) Polygons.size();

	for (int l_num = 0; l_num < Light_Count;  l_num++)
		{
			
			if (l_num == 0)
				Edges.clear();
			for (int id_num = 0; id_num < (int) Lights[l_num].objID.size();  id_num++)
			{
				
				if (Lights[l_num].objID[id_num].type == lePolygon)
				{
						int ObjectID = Lights[l_num].objID[id_num].ID;
						leEdge e = GetEdgeFromPolygon(Polygons[ObjectID],Lights[l_num].Position);
						Edges.push_back(e);
				}
				if (Lights[l_num].objID[id_num].type == leEllipse)
				{
						int Obj_ID = Lights[l_num].objID[id_num].ID;
						leVec2f v1;
						leVec2f v2;
						leVec2f bp1,bp2;
						v1.x = sqrt(pow(Ellipses[Obj_ID].Position.x - Lights[l_num].Position.x,2) +
									pow(Ellipses[Obj_ID].Position.y - Lights[l_num].Position.y,2) );
						if ( v1.x > Ellipses[Obj_ID].Radius)
						{
							int k = 1;
							v1.y = sqrt(pow(v1.x,2) - pow(Ellipses[Obj_ID].Radius,2));
							v2.x = asin((Ellipses[Obj_ID].Position.x - Lights[l_num].Position.x) / v1.x);
							v2.y = asin(Ellipses[Obj_ID].Radius / v1.x);

							if (Ellipses[Obj_ID].Position.y <= Lights[l_num].Position.y )
								k = -1;
							else 
								k = 1;
							bp1.x = Lights[l_num].Position.x + v1.y * sin(v2.x - v2.y);
							bp1.y = Lights[l_num].Position.y + k * v1.y * cos(v2.x - v2.y);
							bp2.x = Lights[l_num].Position.x + v1.y * sin(v2.x + v2.y);
							bp2.y = Lights[l_num].Position.y + k * v1.y * cos(v2.x + v2.y);
							Edges.push_back(leEdge(bp1,bp2,Lights[l_num].Position,leEllipse));
						}		
				}
			}
		}

	

}
