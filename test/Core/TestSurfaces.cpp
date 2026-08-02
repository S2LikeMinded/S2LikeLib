#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>
#include <S2LL/Core/Surfaces.hpp>
#include <limits>

using namespace S2LL::Literals;

TEST_CASE("Ellipsoids", "[core][surface]") {
	Catch::StringMaker<double>::precision = std::numeric_limits<double>::max_digits10;

	SECTION("Inverse flattening precision") {
		REQUIRE(S2LL::wgs84.inv_f() == 298.257223563);

		REQUIRE(S2LL::cgcs2000.inv_f() == 298.257222101);
	}

	SECTION("Coordinate conversion to E3 (triaxial)") {
		// Triaxial ellipsoid with (a, b, c) values
		S2LL::Ellipsoid e(100.0, 200.0, 300.0);

		S2LL::S2 s2{ 0.0, 0.0 }; // North Pole
		S2LL::E3 e3_s2 = e.to_E3(s2);
		REQUIRE(e3_s2.x == 0.0);
		REQUIRE(e3_s2.y == 0.0);
		REQUIRE(e3_s2.z == 300.0);

		s2 = { 0.5 * std::numbers::pi, 0.0 }; // Equator
		e3_s2 = e.to_E3(s2);
		REQUIRE(e3_s2.x == 100.0);
		REQUIRE(e3_s2.y == 0.0);
		REQUIRE(e3_s2.z == 0.0);

		s2 = { 90_deg, 0.0 }; // Equator
		e3_s2 = e.to_E3(s2);
		REQUIRE(e3_s2.x == 100.0);
		REQUIRE(e3_s2.y == 0.0);
		REQUIRE(e3_s2.z == 0.0);

		s2 = { std::numbers::pi, 0.0 }; // South Pole (precision test)
		e3_s2 = e.to_E3(s2);
		REQUIRE(e3_s2.x == 0.0);
		REQUIRE(e3_s2.y == 0.0);
		REQUIRE(e3_s2.z == -300.0);

		s2 = { 180_deg, 0.0 }; // South Pole (precision test)
		e3_s2 = e.to_E3(s2);
		REQUIRE(e3_s2.x == 0.0);
		REQUIRE(e3_s2.y == 0.0);
		REQUIRE(e3_s2.z == -300.0);

		S2LL::LL ll{ 0.0, 0.0 }; // Equator, prime meridian
		S2LL::E3 e3_ll = e.to_E3(ll);
		REQUIRE(e3_ll.x == 100.0);
		REQUIRE(e3_ll.y == 0.0);
		REQUIRE(e3_ll.z == 0.0);
	}

	SECTION("Linear transformation shear and inverse") {
		// (x, y, z) -> (x - (a/b)y, y, z - (c/b)y)
		const S2LL::LinearTransformation T(2.0, 4.0, 1.0);
		const S2LL::E3 p{ 6.0, 8.0, 3.0 };
		const S2LL::E3 sheared = T(p);
		REQUIRE(sheared.x == 2.0); // 6 - (2/4)*8
		REQUIRE(sheared.y == 8.0);
		REQUIRE(sheared.z == 1.0); // 3 - (1/4)*8

		const S2LL::E3 back = T.inverse(sheared);
		REQUIRE(back.x == p.x);
		REQUIRE(back.y == p.y);
		REQUIRE(back.z == p.z);
	}

	SECTION("Sheared sphere quadric has cross terms") {
		// Unit sphere sheared by (a, b, c) = (1, 2, 0.5):
		// M = S^-T S^-1 with S = [[1, -kx, 0], [0, 1, 0], [0, -kz, 1]]
		//   = [[1, kx, 0], [kx, 1+kx^2+kz^2, kz], [0, kz, 1]]
		const S2LL::LinearTransformation T(1.0, 2.0, 0.5);
		const auto M = T.quadric(S2LL::UnitSphere);
		REQUIRE(M.m[0] == 1.3125); // 1 + kx^2 + kz^2
		REQUIRE(M.m[1] == 0.5);
		REQUIRE(M.m[2] == 0.0);
		REQUIRE(M.m[3] == 0.5);
		REQUIRE(M.m[4] == 1.3125); // 1 + 0.25 + 0.0625
		REQUIRE(M.m[5] == 0.25);
		REQUIRE(M.m[6] == 0.0);
		REQUIRE(M.m[7] == 0.25);
		REQUIRE(M.m[8] == 1.0);
	}

	SECTION("Identity transformation leaves points and quadric unchanged") {
		const S2LL::LinearTransformation I;
		const S2LL::E3 p{ -1.0, 2.5, 7.0 };
		const S2LL::E3 same = I(p);
		REQUIRE(same.x == p.x);
		REQUIRE(same.y == p.y);
		REQUIRE(same.z == p.z);

		const auto M = I.quadric(S2LL::Ellipsoid(3.0));
		REQUIRE(M.m[0] == 1.0 / 9.0);
		REQUIRE(M.m[4] == 1.0 / 9.0);
		REQUIRE(M.m[8] == 1.0 / 9.0);
		REQUIRE(M.m[1] == 0.0);
		REQUIRE(M.m[5] == 0.0);
	}

	SECTION("BilinearForm operators and ray intersection") {
		// Unit sphere: q(p) = p^T M p with M = I
		const S2LL::BilinearForm M{ 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
		const S2LL::E3 a{ 1.0, 2.0, 3.0 };
		const S2LL::E3 b{ 4.0, 5.0, 6.0 };
		REQUIRE(M(a) == 14.0);            // 1 + 4 + 9
		REQUIRE(M(a, b) == 32.0);         // 1*4 + 2*5 + 3*6

		// Ray from (12,0,0) toward the origin hits the unit sphere at (1,0,0)
		const auto hit = M.intersect_ray({ 12.0, 0.0, 0.0 }, { -1.0, 0.0, 0.0 }, 1.0);
		REQUIRE(hit.has_value());
		REQUIRE(hit->x == 1.0);
		REQUIRE(hit->y == 0.0);
		REQUIRE(hit->z == 0.0);

		// A ray that misses the sphere yields no intersection
		const auto miss = M.intersect_ray({ 12.0, 2.0, 0.0 }, { -1.0, 0.0, 0.0 }, 1.0);
		REQUIRE_FALSE(miss.has_value());
	}
}
