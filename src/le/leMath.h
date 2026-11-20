#pragma once
#include <math.h>
#define Pi 3.14159265358979323846
enum leObjectType { lePolygon,leEllipse};
struct leObjectID
{
	unsigned int ID;
	leObjectType type;
	leObjectID(){}
	~leObjectID(){}
	leObjectID(unsigned int id,leObjectType t)
	{
		type = t;
		ID = id;
	}
};
float DtoR(float angle);

struct leVec2f
{	
	float x,y;

	leVec2f(){}
	leVec2f(float _x,float _y){x = _x;y = _y;}
	~leVec2f(){};

	leVec2f operator /(const float &scale) const; // divide by a scalar
	leVec2f operator *(const leVec2f &other) const; // component-wise multiply
	leVec2f operator +(const leVec2f &other) const; // add a vector
	leVec2f operator -(const leVec2f &other) const; // subtract a vector
	leVec2f operator -() const;

	bool operator==(leVec2f &other) const;
	bool operator>=(leVec2f &other) const;
	bool operator<=(leVec2f &other) const;
	bool operator <(leVec2f &other) const;
	bool operator >(leVec2f &other) const;

	bool operator!=(leVec2f &other) const;
	
	void Scale(float size); // multiply by a scalar in place
	float Length(); // vector length
	leVec2f Normal();
}; 
struct leEdge
{
	unsigned int ID;
	leVec2f v1;
	leVec2f v2;
	leVec2f LightVec1;
	leVec2f LightVec2;
	leObjectType type;
	leEdge(){}
	leEdge(leVec2f v_1,leVec2f v_2,leVec2f lpos, leObjectType t)
	{
		v1 = v_1;
		v2 = v_2;
		LightVec1 = v1 + (v1 - lpos) * leVec2f(15,15);
		LightVec2 = v2 + (v2 - lpos) * leVec2f(15,15);
		type = t;
	}

	
	~leEdge(){}
};
struct leAABB
{
	leVec2f upperBound;
	leVec2f lowerBound;
};
float dot(leVec2f v1,leVec2f v2);