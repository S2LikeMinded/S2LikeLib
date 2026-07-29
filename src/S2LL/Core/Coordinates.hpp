#pragma once

#include <cfenv>
#include <limits>
#include <numbers>
#include <ostream>
#include <S2LL/Core/Numerics.hpp>
#include <S2LL/Core/Utilities.hpp>

namespace S2LL
{
	struct E2;
	struct E3;
	struct S2;
	struct LL;

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

		// Checks if any coordinate component is NaN
		inline bool isnan() const noexcept
		{
			return std::isnan(x) || std::isnan(y) || std::isnan(z);
		}

		// Calculates vector magnitude using extended-precision S2LL::Double
		inline double mag() const
		{
			Double sqsum = sq(x) + sq(y) + sq(z);
			return static_cast<double>(sqrt(sqsum));
		}

		// Normalizes this 3D vector in-place using extended-precision S2LL::Double
		inline E3& normalize()
		{
			Double sqsum = sq(x) + sq(y) + sq(z);
			if (sqsum.iszero())
			{
				std::feraiseexcept(FE_DIVBYZERO);
				x = std::numeric_limits<double>::quiet_NaN();
				y = std::numeric_limits<double>::quiet_NaN();
				z = std::numeric_limits<double>::quiet_NaN();
				return *this;
			}
			Double d_m = sqrt(sqsum);
			x = static_cast<double>(div(Lift(x), d_m));
			y = static_cast<double>(div(Lift(y), d_m));
			z = static_cast<double>(div(Lift(z), d_m));
			return *this;
		}

		friend std::ostream& operator<<(std::ostream& ost, const E3& e3)
		{
			ost << e3.x << ' ' << e3.y << ' ' << e3.z;
			return ost;
		}
	};

	// Multiplies a 3D vector by a scalar using extended-precision S2LL::Double
	inline E3 operator*(const E3& v, double scalar)
	{
		return E3{
			static_cast<double>(mul(v.x, scalar)),
			static_cast<double>(mul(v.y, scalar)),
			static_cast<double>(mul(v.z, scalar))
		};
	}

	// Multiplies a scalar by a 3D vector using extended-precision S2LL::Double
	inline E3 operator*(double scalar, const E3& v)
	{
		return v * scalar;
	}

	// Plain-old-data type for spherical coordinates on spheres
	struct S2
	{
	public:
		// Polar angle, in radians
		double p;

		// Azimuthal angle, in radians
		double a;

		// Converts spherical coordinates to latitude-longitude pair
		LL to_LL() const noexcept;

		friend std::ostream& operator<<(std::ostream& ost, const S2& s2)
		{
			ost << s2.p << ' ' << s2.a;
			return ost;
		}
	};

	// Plain-old-data type for generic latitude-longitude coordinate pair
	struct LL
	{
	public:
		// Some latitude, in radians
		double lat;

		// Some longitude, in radians
		double lon;

		// Converts latitude-longitude pair to spherical coordinates
		S2 to_S2() const noexcept;

		friend std::ostream& operator<<(std::ostream& ost, const LL& ll)
		{
			ost << ll.lat << ' ' << ll.lon;
			return ost;
		}
	};

	inline LL S2::to_LL() const noexcept
	{
		return LL{ 0.5 * std::numbers::pi - p, a };
	}

	inline S2 LL::to_S2() const noexcept
	{
		return S2{ 0.5 * std::numbers::pi - lat, lon };
	}

	// Compile-time layout & copyability verification
	S2LL_ASSERT_POD(E2);
	S2LL_ASSERT_POD(E3);
	S2LL_ASSERT_POD(S2);
	S2LL_ASSERT_POD(LL);
}
