#pragma once

#include <S2LL/Core/Coordinates.hpp>
#include <S2LL/Core/Numerics.hpp>

#include <cmath>

namespace S2LL
{
	/// Representation of 3D ellipsoidal and spherical surfaces
	class Ellipsoid
	{
		/// Semi-major axis length along the x axis
		double a;

		/// Semi-median axis length along the y axis
		double b;

		/// Semi-minor axis length along the z axis
		Double c;

	public:
		/// Constructs a spherical surface with uniform radius r
		Ellipsoid(double r);

		/// Constructs an oblate spheroid with semimajor axis a and inverse flattening inv_f
		Ellipsoid(double a, double inv_f);

		/// Constructs a triaxial ellipsoid with semi-principal axes a, b, and minor
		Ellipsoid(double a, double b, double c);

		/// Returns the semi-major radius
		inline double major() const { return a; };

		/// Returns the semi-median radius
		inline double median() const { return b; };

		/// Returns the semi-minor radius
		inline double minor() const { return static_cast<double>(c); }

		/// Returns true if the surface is a sphere (a == b == c)
		inline bool is_sphere() const noexcept { return a == b && b == static_cast<double>(c); }

		/// Returns the inverse flattening of the ellipsoid
		double inv_f() const;

		/// Converts spherical coordinates (polar angle p, azimuth a) into 3D Cartesian space
		inline E3 to_E3(const S2& s2) const noexcept
		{
			auto [sin_p, cos_p] = sin_and_cos(s2.p);
			auto [sin_a, cos_a] = sin_and_cos(s2.a);
			return E3{
				static_cast<double>(mul(mul(a, sin_p), cos_a)),
				static_cast<double>(mul(mul(b, sin_p), sin_a)),
				static_cast<double>(mul(c, cos_p))
			};
		}

		/// Converts latitude-longitude coordinates (lat, lon) into 3D Cartesian space
		inline E3 to_E3(const LL& ll) const noexcept
		{
			auto [sin_lat, cos_lat] = sin_and_cos(ll.lat);
			auto [sin_lon, cos_lon] = sin_and_cos(ll.lon);
			return E3{
				static_cast<double>(mul(mul(a, cos_lat), cos_lon)),
				static_cast<double>(mul(mul(b, cos_lat), sin_lon)),
				static_cast<double>(mul(c, sin_lat))
			};
		}
	};

	/// Unit sphere (r = 1.0)
	inline const Ellipsoid UnitSphere{1.0};

	/// WGS 84 reference ellipsoid (a = 6378137.0 m, 1/f = 298.257223563)
	inline const Ellipsoid wgs84{6378137.0, 298.257223563};

	/// CGCS2000 reference ellipsoid (a = 6378137.0 m, 1/f = 298.257222101)
	inline const Ellipsoid cgcs2000{6378137.0, 298.257222101};
}
