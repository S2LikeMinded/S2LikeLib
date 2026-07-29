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

		s2 = { static_cast<double>(90_deg), 0.0 }; // Equator
		e3_s2 = e.to_E3(s2);
		REQUIRE(e3_s2.x == 100.0);
		REQUIRE(e3_s2.y == 0.0);
		REQUIRE(e3_s2.z == 0.0);

		s2 = { std::numbers::pi, 0.0 }; // South Pole (precision test)
		e3_s2 = e.to_E3(s2);
		REQUIRE(e3_s2.x == 0.0);
		REQUIRE(e3_s2.y == 0.0);
		REQUIRE(e3_s2.z == -300.0);

		s2 = { static_cast<double>(180_deg), 0.0 }; // South Pole (precision test)
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
}
