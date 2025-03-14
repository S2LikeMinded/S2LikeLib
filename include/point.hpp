
#ifndef __POINT_HPP__
#define __POINT_HPP__

class Point
{
public:
	// Constructor using spherical coordinates
	Point(double polar, double azimuth)
	: p(polar), a(azimuth)
	{

	}

private:
	// Spherical coordinates
	double p, a, r;

	// Cartesian coordinates
	double x, y, z;

public:
	// Output Cartesian coordinates
	friend &std::ostream operator<<(std::ostream &os, const Point& p)
	{
		os << p.x << " " << p.y << " " << p.z;
		return os;
	}
};

#endif // !__POINT_HPP__
