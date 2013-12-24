#include "StdAfx.h"
#include "leMath.h"



float DtoR(float angle)
{
	return angle * (Pi / 180.0f);
};
// 2D vector
float leVec2f::Length()
{
	return sqrt(x*x + y*y);
}
void leVec2f::Scale(float size)
{
	x *= size; y *= size;
}
leVec2f leVec2f::operator /(const float &scale) const
{
	return leVec2f(x / scale,y / scale);
}
leVec2f leVec2f::operator *(const leVec2f &other) const
{
	return leVec2f(x * other.x,y * other.y);
}
leVec2f leVec2f::operator +(const leVec2f &other) const
{
	return leVec2f(x + other.x,y + other.y);
}
leVec2f leVec2f::operator -(const leVec2f &other) const
{
	return leVec2f(x - other.x,y - other.y);
}
leVec2f leVec2f::operator -() const
{
	return leVec2f(-x,-y);
}
bool leVec2f::operator == (leVec2f &other) const
{
	if ( x == other.x && y == other.y)
		return true;
	return false;
}
bool leVec2f::operator >= (leVec2f &other) const
{
	if ( x >= other.x && y >= other.y)
		return true;
	return false;
}
bool leVec2f::operator <= (leVec2f &other) const
{
	if ( x <= other.x && y <= other.y)
		return true;
	return false;
}
bool leVec2f::operator < (leVec2f &other) const
{
	if ( x < other.x && y < other.y)
		return true;
	return false;
}
bool leVec2f::operator > (leVec2f &other) const
{
	if ( x > other.x && y > other.y)
		return true;
	return false;
}
bool leVec2f::operator != (leVec2f &other) const
{
	if ( x != other.x && y != other.y)
		return true;
	return false;
}
leVec2f leVec2f::Normal()
{
	return leVec2f(-y,x);
}

// DOT
float dot(leVec2f v1,leVec2f v2)
{
	return (v1.x * v2.x + v1.y * v2.y);
}