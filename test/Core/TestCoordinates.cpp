#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <S2LL/Core/Coordinates.hpp>

TEST_CASE("E2 coordinates", "[core][coordinates]") {
	SECTION("Default initialization") {
		S2LL::E2 c{};

		REQUIRE(c.x == 0.0);
		REQUIRE(c.y == 0.0);
	}
}

TEST_CASE("E3 coordinates", "[core][coordinates]") {
	SECTION("Default initialization") {
		S2LL::E3 c{};

		REQUIRE(c.x == 0.0);
		REQUIRE(c.y == 0.0);
		REQUIRE(c.z == 0.0);
	}
}

TEST_CASE("S2 coordinates", "[core][coordinates]") {
	SECTION("Default initialization") {
		S2LL::S2 c{};

		REQUIRE(c.p == 0.0);
		REQUIRE(c.a == 0.0);
	}
}

TEST_CASE("LatLon coordinates", "[core][coordinates]") {
	SECTION("Default initialization") {
		S2LL::LatLon c{};

		REQUIRE(c.lat == 0.0);
		REQUIRE(c.lon == 0.0);
	}
}
