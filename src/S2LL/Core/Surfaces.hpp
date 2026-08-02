#pragma once

#include <S2LL/Core/Coordinates.hpp>
#include <S2LL/Core/Numerics.hpp>

#include <array>
#include <cmath>
#include <optional>

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

	/// Symmetric 3x3 bilinear/quadratic form, stored row-major (9 entries).
	/// The implicit surface is p^T M p = rhs, e.g. rhs = 1 for an ellipsoid.
	struct BilinearForm
	{
		double m[9];

		/// Quadratic form B(p, p) = p^T M p
		inline double operator()(const E3& p) const noexcept
		{
			return m[0] * p.x * p.x + 2.0 * m[1] * p.x * p.y + 2.0 * m[2] * p.x * p.z
				+ m[4] * p.y * p.y + 2.0 * m[5] * p.y * p.z + m[8] * p.z * p.z;
		}

		/// Symmetric bilinear form B(a, b) = a^T M b (B(a, a) == operator()(a))
		inline double operator()(const E3& a, const E3& b) const noexcept
		{
			return m[0] * a.x * b.x + m[1] * (a.x * b.y + a.y * b.x) + m[2] * (a.x * b.z + a.z * b.x)
				+ m[4] * a.y * b.y + m[5] * (a.y * b.z + a.z * b.y) + m[8] * a.z * b.z;
		}

		/// Closest ray intersection with p^T M p = rhs (ray direction assumed
		/// normalized). Returns nullopt when the ray misses.
		inline std::optional<E3> intersect_ray(const E3& o, const E3& d, double rhs) const
		{
			const double a = (*this)(d, d);
			const double b = 2.0 * (*this)(o, d);
			const double c = (*this)(o) - rhs;
			const double disc = b * b - 4.0 * a * c;
			if (disc < 0.0)
			{
				return std::nullopt;
			}
			const double sq = std::sqrt(disc);
			const double t0 = (-b - sq) / (2.0 * a);
			const double t1 = (-b + sq) / (2.0 * a);
			if (t0 >= 0.0)
			{
				return E3{ o.x + t0 * d.x, o.y + t0 * d.y, o.z + t0 * d.z };
			}
			if (t1 >= 0.0)
			{
				return E3{ o.x + t1 * d.x, o.y + t1 * d.y, o.z + t1 * d.z };
			}
			return std::nullopt;
		}
	};

	/// Invertible linear transformation of 3D space (3x3 matrix, row-major).
	/// Used to shear a base surface (e.g. the sphere of Demo 1c) into a
	/// general ellipsoid whose quadric form has off-diagonal (cross) terms.
	class LinearTransformation
	{
		Double m[3][3];
		// Shear parameters kx = sx/sy, kz = sz/sy, kept for quadric recomputation
		Double kx, kz;

	public:
		/// The identity transformation
		constexpr LinearTransformation() noexcept
			: m{ {Double::One, Double::Zero, Double::Zero},
				{Double::Zero, Double::One, Double::Zero},
				{Double::Zero, Double::Zero, Double::One} },
			kx(Double::Zero), kz(Double::Zero)
		{
		}

		/// Shear along the y axis: (x, y, z) -> (x - (sx/sy)y, y, z - (sz/sy)y).
		/// sy must be nonzero; the matrix is unimodular (det == 1).
		inline explicit LinearTransformation(double sx, double sy, double sz) noexcept
			: m{ {Double::One, -Lift(sx) / Lift(sy), Double::Zero},
				{Double::Zero, Double::One, Double::Zero},
				{Double::Zero, -Lift(sz) / Lift(sy), Double::One} },
			kx(Lift(sx) / Lift(sy)), kz(Lift(sz) / Lift(sy))
		{
		}

		/// Applies the transformation to a point
		inline E3 operator()(const E3& p) const noexcept
		{
			return E3{
				static_cast<double>(add(p.x, mul(m[0][1], p.y))),
				p.y,
				static_cast<double>(add(p.z, mul(m[2][1], p.y)))
			};
		}

		/// Applies the inverse transformation to a point
		inline E3 inverse(const E3& p) const noexcept
		{
			return E3{
				static_cast<double>(add(p.x, mul(kx, p.y))),
				p.y,
				static_cast<double>(add(p.z, mul(kz, p.y)))
			};
		}

		/// Bilinear form M of the image of an ellipsoid under this
		/// transformation: the image satisfies p^T M p = 1.
		inline BilinearForm quadric(const Ellipsoid& e) const noexcept
		{
			// Base triaxial form: x^2/a^2 + y^2/b^2 + z^2/c^2 = 1.
			// Under the shear p' = S p the implicit form becomes p^T M p = 1
			// with M = S^-T D S^-1, whose gradient 2 M p is the surface
			// normal. (S^T D S would give the normal of the inverse shear.)
			const Double a = Lift(e.major());
			const Double b = Lift(e.median());
			const Double c = Lift(e.minor());
			const Double ia2 = Double::One / (a * a);
			const Double ib2 = Double::One / (b * b);
			const Double ic2 = Double::One / (c * c);
			const Double m11 = ia2 + kx * kx * ib2 + kz * kz * ic2;
			const Double m12 = kx * ib2;
			const Double m22 = kx * kx * ia2 + ib2 + kz * kz * ic2;
			const Double m23 = kz * ic2;
			return BilinearForm{
				static_cast<double>(m11), static_cast<double>(m12), 0.0,
				static_cast<double>(m12), static_cast<double>(m22), static_cast<double>(m23),
				0.0, static_cast<double>(m23), static_cast<double>(ic2)
			};
		}
	};
}
