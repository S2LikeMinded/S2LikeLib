
#ifndef __POINT_HPP__
#define __POINT_HPP__

#include <ostream>


namespace S2LM
{
	// Plain-old-data type for 2D Cartesian coordinates
	struct E2
	{
		// 2D Cartesian coordinates
		double x, y;


		friend std::ostream& operator<<(std::ostream& ost, const E2& e2)
		{
			ost << "[" << e2.x << " " << e2.y << "]";
			return ost;
		}
	};

	// Plain-old-data type for 3D Cartesian coordinates
	struct E3
	{
	public:
		// 3D Cartesian coordinates
		double x, y, z;


		friend std::ostream& operator<<(std::ostream& ost, const E3& e3)
		{
			ost << e3.x << ' ' << e3.y << ' ' << e3.z;
			return ost;
		}
	};

	// Plain-old-data type for spherical coordinates
	struct S2
	{
	public:
		// Polar angle, in radians
		double p;

		// Azimuthal angle, in radians
		double a;


		friend std::ostream& operator<<(std::ostream& ost, const S2& s2)
		{
			ost << s2.p << ' ' << s2.a;
			return ost;
		}
	};

}

#endif // !__POINT_HPP__
