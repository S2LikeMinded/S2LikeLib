
#ifndef __POINT_HPP__
#define __POINT_HPP__

#include <ostream>


namespace S2LM
{

	// Plain-old-data type for 2D Cartesian coordinates
	struct E2
	{

		double x;

		double y;
	};

	// Plain-old-data type for 3D Cartesian coordinates
	struct E3
	{

		double x;

		double y;

		double z;
	};

	// Plain-old-data type for spherical coordinates
	struct S2
	{

		S2(double p, double a) : p(p), a(a) {}

		// Polar angle, in radians
		double p;

		// Azimuthal angle, in radians
		double a;
	};

}

#endif // !__POINT_HPP__
