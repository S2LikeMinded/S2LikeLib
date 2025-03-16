
#ifndef __POINT_HPP__
#define __POINT_HPP__

#include <ostream>


namespace S2LM
{

	class Point
	{
	public:
		// Construct an uninitialized point
		Point();

		// Constructor using Cartesian coordinates
		Point(double x, double y, double z);

		// Constructor using spherical coordinates
		Point(double polar, double azimuth);

	private:
		// Are Cartesian coordinates initialized?
		bool c;

		// Are spherical coordinates intialized?
		bool s;

		// Cartesian coordinates
		double x, y, z;

		// Spherical coordinates
		double p, a;

	public:
		// Output Cartesian coordinates
		friend std::ostream& operator<<(std::ostream& ost, const Point& point);
	};

	inline std::ostream& operator<<(std::ostream& ost, const Point& point)
	{
		ost << '[';
		if (point.c)
		{
			ost << point.x << ' ' << point.y << ' ' << point.z;
			if (point.s)
			{
				ost << ' ';
			}
		}
		if (point.s)
		{
			ost << '(' << point.p << ' ' << point.a << ')';
		}
		ost << ']';
		return ost;
	}
}

#endif // !__POINT_HPP__
