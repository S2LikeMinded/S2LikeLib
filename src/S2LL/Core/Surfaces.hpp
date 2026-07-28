#pragma once

#include <S2LL/Core/Numerics.hpp>

namespace S2LL
{
	// Representation of 3D ellipsoidal and spherical surfaces
	class Ellipsoid
	{
		// Semi-major axis length along the x axis
		double a;

		// Semi-median axis length along the y axis
		double b;

		// Semi-minor axis length along the z axis
		Double c;

	public:
		// Constructs a spherical surface with uniform radius r
		Ellipsoid(double r);

		// Constructs an oblate spheroid with semimajor axis a and inverse flattening inv_f
		Ellipsoid(double a, double inv_f);

		// Constructs a triaxial ellipsoid with semi-principal axes a, b, and c
		Ellipsoid(double a, double b, double c);

		// Returns the inverse flattening of the ellipsoid
		double inv_f() const;
	};

	// Unit sphere (r = 1.0)
	inline const Ellipsoid UnitSphere{1.0};

	// WGS 84 reference ellipsoid (a = 6378137.0 m, 1/f = 298.257223563)
	inline const Ellipsoid wgs84{6378137.0, 298.257223563};

	// CGCS2000 reference ellipsoid (a = 6378137.0 m, 1/f = 298.257222101)
	inline const Ellipsoid cgcs2000{6378137.0, 298.257222101};
}
