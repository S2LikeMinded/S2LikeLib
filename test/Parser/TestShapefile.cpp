
#include <catch2/catch_test_macros.hpp>
#include <S2LL/Parser/Shapefile.hpp>

TEST_CASE("Shapefile parser initialization", "[parser][shapefile]") {
	S2LL::Parser::Shapefile parser;
	REQUIRE(parser.regions.empty());
}
