#pragma once
#include "leMath.h"
#include <vector>
enum leLightType {le_flight  /* Flashlight */,le_llight /* Lamp */}; 
class leLight
{
public:
	leVec2f Position;
	leLightType type;
	float Radius;
	float Intensity;
	bool Sleep;

	leVec2f v[3];
	float Angle;
	float s_Angle;


	std::vector < leObjectID > objID;
	
	leLight(void);
	~leLight(void);
	leLight(leVec2f p, float r,float inten){
		type = le_llight;
		Position = p;Radius = r;Intensity = inten;
											Sleep = false;
											}
	leLight(leVec2f p, float r,float angle, float sangle);


	void Move(float x, float y);
	void leLight::Move(float x, float y,float a,float sa);

	leAABB AABB();


};
