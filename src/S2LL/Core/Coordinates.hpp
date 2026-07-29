#pragma once

#include <numbers>
#include <ostream>
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

		friend std::ostream& operator<<(std::ostream& ost, const E3& e3)
		{
			ost << e3.x << ' ' << e3.y << ' ' << e3.z;
			return ost;
		}
	};

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
