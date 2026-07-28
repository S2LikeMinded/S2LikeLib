#include <catch2/catch_test_macros.hpp>
#include <S2LL/Core/Regions.hpp>

TEST_CASE("Compound Polygon container", "[core][polygon]") {
	S2LL::Compound<S2LL::Polygon> cPoly;

	REQUIRE(cPoly.polygons.empty());

	SECTION("Adding a polygon") {
		cPoly.polygons.emplace_back();
		REQUIRE(cPoly.polygons.size() == 1);
	}
}
