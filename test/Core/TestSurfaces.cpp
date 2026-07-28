#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>
#include <S2LL/Core/Surfaces.hpp>
#include <limits>

TEST_CASE("Ellipsoids", "[core][surface]") {
	Catch::StringMaker<double>::precision = std::numeric_limits<double>::max_digits10;

	SECTION("Inverse flattening accuracy") {
		// a-a/f: 298.25722356299718285
		// FMA:   298.25722356299718285
		// Exact: 298.25722356300002502
		// Still: 298.25722356299718285
		REQUIRE(S2LL::wgs84.inv_f() == 298.257223563);

		REQUIRE(S2LL::cgcs2000.inv_f() == 298.257222101);
	}
}
