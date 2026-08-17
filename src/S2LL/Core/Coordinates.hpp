#pragma once

#include <cfenv>
#include <limits>
#include <numbers>
#include <ostream>
#include <S2LL/Core/Curves.hpp>
#include <S2LL/Core/Numerics.hpp>
#include <S2LL/Core/Utilities.hpp>

using namespace S2LL::Literals;

namespace S2LL
{
	struct E2;
	struct E3;
	struct S2;
	struct LL;

	// Plain-old-data type for 2D Cartesian coordinates
	struct E2
	{
		template <size_t N = 0>
		using Loop = S2LL::Loop<E2, N>;

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
		template <size_t N = 0>
		using Loop = S2LL::Loop<E3, N>;

		// 3D Cartesian coordinates
		double x, y, z;

		// Checks if any coordinate component is NaN
		inline bool isnan() const noexcept
		{
			return std::isnan(x) || std::isnan(y) || std::isnan(z);
		}

		// Calculates vector magnitude using extended precision
		inline Double Mag() const
		{
			Double sqsum = Sq(x) + Sq(y) + Sq(z);
			return Sqrt(sqsum);
		}

		/// Calculates vector magnitude using extended precision
		inline double mag() const
		{
			return static_cast<double>(Mag());
		}

		// Normalizes this 3D vector in-place using extended precision
		inline E3& normalize()
		{
			Double sqsum = Sq(x) + Sq(y) + Sq(z);
			if (sqsum.iszero())
			{
				std::feraiseexcept(FE_DIVBYZERO);
				x = std::numeric_limits<double>::quiet_NaN();
				y = std::numeric_limits<double>::quiet_NaN();
				z = std::numeric_limits<double>::quiet_NaN();
				return *this;
			}
			Double d_m = Sqrt(sqsum);
			x = static_cast<double>(Div(Lift(x), d_m));
			y = static_cast<double>(Div(Lift(y), d_m));
			z = static_cast<double>(Div(Lift(z), d_m));
			return *this;
		}

		// Returns a normalized copy of this vector using extended precision
		inline E3 normalized() const
		{
			E3 temp = *this;
			temp.normalize();
			return temp;
		}

		// Calculates dot product using extended precision
		inline double dot(const E3& other) const noexcept
		{
			Double d = Mul(x, other.x) + Mul(y, other.y) + Mul(z, other.z);
			return static_cast<double>(d);
		}

		// Calculates cross product (*this x other) using extended precision
		inline E3 cross(const E3& other) const noexcept
		{
			Double cx = Mul(y, other.z) - Mul(z, other.y);
			Double cy = Mul(z, other.x) - Mul(x, other.z);
			Double cz = Mul(x, other.y) - Mul(y, other.x);
			return E3{
				static_cast<double>(cx),
				static_cast<double>(cy),
				static_cast<double>(cz)
			};
		}

		/// Converts (x, y, z) into spherical/geocentric coordinates
		S2 s2() const noexcept;

		/// Converts (x, y, z) into latitude-longitude coordinates
		LL ll() const noexcept;

		friend std::ostream& operator<<(std::ostream& ost, const E3& e3)
		{
			ost << e3.x << ' ' << e3.y << ' ' << e3.z;
			return ost;
		}
	};

	// Adds two 3D vectors using  extended precision
	inline E3 operator+(const E3& a, const E3& b) noexcept
	{
		return E3{
			static_cast<double>(Add(a.x, b.x)),
			static_cast<double>(Add(a.y, b.y)),
			static_cast<double>(Add(a.z, b.z))
		};
	}

	// Subtracts two 3D vectors using extended precision
	inline E3 operator-(const E3& a, const E3& b) noexcept
	{
		return E3{
			static_cast<double>(Sub(a.x, b.x)),
			static_cast<double>(Sub(a.y, b.y)),
			static_cast<double>(Sub(a.z, b.z))
		};
	}

	// Multiplies a 3D vector by a scalar using extended precision
	inline E3 operator*(const E3& v, double scalar)
	{
		return E3{
			static_cast<double>(Mul(v.x, scalar)),
			static_cast<double>(Mul(v.y, scalar)),
			static_cast<double>(Mul(v.z, scalar))
		};
	}

	// Multiplies a scalar by a 3D vector using extended precision
	inline E3 operator*(double scalar, const E3& v)
	{
		return v * scalar;
	}

	// Calculates dot product of two 3D vectors using extended precision
	inline double dot(const E3& a, const E3& b) noexcept
	{
		return a.dot(b);
	}

	// Calculates cross product of two 3D vectors using extended precision
	inline E3 cross(const E3& a, const E3& b) noexcept
	{
		return a.cross(b);
	}

	// Plain-old-data type for spherical/geocentric coordinates
	struct S2
	{
	public:
		template <size_t N = 0>
		using Loop = S2LL::Loop<S2, N>;

		// Polar angle, in radians
		double p;

		// Azimuthal angle, in radians
		double a;

		// Converts spherical coordinates to latitude-longitude pair
		LL ll() const noexcept;

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
		template <size_t N = 0>
		using Loop = S2LL::Loop<LL, N>;

		// Some latitude, in radians
		double lat;

		// Some longitude, in radians
		double lon;

		// Converts latitude-longitude pair to spherical coordinates
		S2 s2() const noexcept;

		// Converts latitude-longitude pair to unit-sphere direction vector
		E3 e3() const noexcept;

		friend std::ostream& operator<<(std::ostream& ost, const LL& ll)
		{
			ost << ll.lat << ' ' << ll.lon;
			return ost;
		}
	};

	inline S2 E3::s2() const noexcept
	{
		Double p = Acos(Div(Lift(z), Mag()));
		Double a = Atan2(Lift(y), Lift(x));
		return S2 {
			static_cast<double>(p),
			static_cast<double>(a)
		};
	}

	inline LL E3::ll() const noexcept
	{
		return s2().ll();
	}

	inline LL S2::ll() const noexcept
	{
		return LL{ 0.5_pi - p, a };
	}

	inline S2 LL::s2() const noexcept
	{
		return S2{ 0.5_pi - lat, lon };
	}

	inline E3 LL::e3() const noexcept
	{
		auto [sin_lat, cos_lat] = SinCos(lat);
		auto [sin_lon, cos_lon] = SinCos(lon);
		return E3{
			static_cast<double>(Mul(cos_lat, cos_lon)),
			static_cast<double>(Mul(cos_lat, sin_lon)),
			static_cast<double>(sin_lat)
		};
	}

	// Compile-time layout & copyability verification
	S2LL_ASSERT_POD(E2);
	S2LL_ASSERT_POD(E3);
	S2LL_ASSERT_POD(S2);
	S2LL_ASSERT_POD(LL);
	S2LL_ASSERT_POD(E3::Loop<4>);
}
