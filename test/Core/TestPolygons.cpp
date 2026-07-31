#include <catch2/catch_test_macros.hpp>
#include <S2LL/Core/Regions.hpp>

#include <type_traits>

TEST_CASE("Compound Polygon container", "[core][polygon]") {
	S2LL::Compound<S2LL::PlanePolygon<>> cPoly;

	REQUIRE(cPoly.polygons.empty());

	SECTION("Adding a polygon") {
		cPoly.polygons.emplace_back();
		REQUIRE(cPoly.polygons.size() == 1);
	}
}

TEST_CASE("PlanePolygon owns a cyclic E2 loop", "[core][polygon]") {
	S2LL::PlanePolygon<> poly;
	poly.boundary = S2LL::Loop<S2LL::E2>{ {0, 0}, {1, 0}, {1, 1} };

	REQUIRE(poly.size() == 3);
	REQUIRE(poly[3].x == 0.0);   // cyclic wrap
	REQUIRE(poly[-1].y == 1.0);  // cyclic wrap (closing vertex)

	SECTION("Edges are realized from consecutive vertices") {
		const auto e0 = poly.edge(0);
		REQUIRE(e0.a.x == 0.0);
		REQUIRE(e0.b.x == 1.0);

		const auto closing = poly.edge(2);
		REQUIRE(closing.a.x == 1.0);
		REQUIRE(closing.b.x == 0.0);  // wraps back to vertex 0
	}

	SECTION("Edges are straight segments") {
		static_assert(std::is_same_v<S2LL::PlanePolygon<>::edge_type, S2LL::LineSegment<S2LL::E2>>);
	}
}

TEST_CASE("GeodesicTag on E2 resolves to the segment edge model", "[core][polygon]") {
	static_assert(std::is_same_v<
		S2LL::EdgeTraits<S2LL::EdgeTag::Geodesic, S2LL::E2>::edge_type,
		S2LL::EdgeTraits<S2LL::EdgeTag::Straight, S2LL::E2>::edge_type>);

	S2LL::PolygonBase<S2LL::E2, S2LL::EdgeTag::Geodesic> poly;
	poly.boundary = S2LL::Loop<S2LL::E2>{ {0, 0}, {1, 0}, {1, 1} };

	const auto e1 = poly.edge(1);
	REQUIRE(e1.a.x == 1.0);
	REQUIRE(e1.b.x == 1.0);
}

TEST_CASE("Array-backed polygons own fixed-size loops", "[core][polygon]") {
	S2LL::PolygonBase<S2LL::E2, S2LL::EdgeTag::Geodesic, 3> poly;
	poly.boundary = S2LL::E2::Loop<3>{ {0, 0}, {1, 0}, {1, 1} };

	REQUIRE(poly.size() == 3);
	REQUIRE(poly[3].x == 0.0);  // cyclic wrap

	const auto closing = poly.edge(2);
	REQUIRE(closing.a.x == 1.0);
	REQUIRE(closing.b.x == 0.0);  // wraps back to vertex 0

	static_assert(std::is_same_v<decltype(poly)::loop_type, S2LL::Loop<S2LL::E2, 3>>);
}

TEST_CASE("E3 polygon flavors realize their own edge types", "[core][polygon]") {
	S2LL::SpacePolygon<> chord;
	chord.boundary = S2LL::Loop<S2LL::E3>{ {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
	const auto chordEdge = chord.edge(0);
	REQUIRE(chordEdge.a.z == 0.0);
	REQUIRE(chordEdge.b.z == 0.0);

	S2LL::GEP<> sectional;
	sectional.boundary = S2LL::Loop<S2LL::E3>{ {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
	const auto sectionalEdge = sectional.edge(2, S2LL::UnitSphere);
	REQUIRE(sectionalEdge.a.z == 1.0);
	REQUIRE(sectionalEdge.b.x == 1.0);

	S2LL::GP<> geodesic;
	geodesic.boundary = S2LL::Loop<S2LL::E3>{ {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
	const auto geodesicEdge = geodesic.edge(0, S2LL::UnitSphere);
	REQUIRE(geodesicEdge.a.y == 0.0);
	REQUIRE(geodesicEdge.b.y == 1.0);

	static_assert(std::is_same_v<S2LL::SpacePolygon<>::edge_type, S2LL::LineSegment<S2LL::E3>>);
	static_assert(std::is_same_v<S2LL::GEP<>::edge_type, S2LL::EllipticArc>);
	static_assert(std::is_same_v<S2LL::GP<>::edge_type, S2LL::GeodesicArc>);
}

TEST_CASE("Polygon aliases match the tagged types", "[core][polygon]") {
	static_assert(std::is_same_v<S2LL::PlanePolygon<>, S2LL::PolygonBase<S2LL::E2>>);
}
