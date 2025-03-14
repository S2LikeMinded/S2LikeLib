
#ifndef __POINT_HPP__
#define __POINT_HPP__

#include <ostream>

class Point
{
public:
	// Constructor using spherical coordinates
	Point(double polar, double azimuth);

private:
	// Spherical coordinates
	double p, a, r;

	// Cartesian coordinates
	double x, y, z;

public:
	// Output Cartesian coordinates
	friend std::ostream& operator<<(std::ostream& ost, const Point& point);
};

#endif // !__POINT_HPP__
