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

	SECTION("Magnitude calculation using Double") {
		S2LL::E3 v1{ 1.0, 1.0, 1.0 };
		REQUIRE(v1.mag() == std::sqrt(3.0));

		S2LL::E3 v2{ -2.0, -1.0, -2.0 };
		REQUIRE(v2.mag() == 3.0);

		S2LL::E3 v3{ 3.0, 4.0, 12.0 };
		REQUIRE(v3.mag() == 13.0);
	}

	SECTION("In-place normalization using Double") {
		S2LL::E3 v1{ 3.0, 0.0, 4.0 };
		v1.normalize();
		REQUIRE(v1.x == 0.6);
		REQUIRE(v1.y == 0.0);
		REQUIRE(v1.z == 0.8);
		REQUIRE(v1.mag() == 1.0);

		S2LL::E3 v2{ 0.0, 0.0, 0.0 };
		v2.normalize();
		REQUIRE(v2.isnan());
		REQUIRE(std::isnan(v2.x));
		REQUIRE(std::isnan(v2.y));
		REQUIRE(std::isnan(v2.z));
	}
}

#include <numbers>

TEST_CASE("S2 coordinates", "[core][coordinates]") {
	SECTION("Default initialization") {
		S2LL::S2 c{};

		REQUIRE(c.p == 0.0);
		REQUIRE(c.a == 0.0);
	}

	SECTION("Conversion to LL") {
		S2LL::S2 s2{ 0.0, 1.25 };
		S2LL::LL ll = s2.to_LL();

		REQUIRE(ll.lat == 0.5 * std::numbers::pi);
		REQUIRE(ll.lon == s2.a);
	}
}

TEST_CASE("LL coordinates", "[core][coordinates]") {
	SECTION("Default initialization") {
		S2LL::LL c{};

		REQUIRE(c.lat == 0.0);
		REQUIRE(c.lon == 0.0);
	}

	SECTION("Conversion to S2") {
		S2LL::LL ll{ 0.0, 1.25 };
		S2LL::S2 s2 = ll.to_S2();

		REQUIRE(s2.p == 0.5 * std::numbers::pi);
		REQUIRE(s2.a == ll.lon);
	}
}
