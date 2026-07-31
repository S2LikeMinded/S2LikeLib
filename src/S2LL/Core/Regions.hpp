#pragma once

#include <S2LL/Core/Coordinates.hpp>
#include <S2LL/Core/Surfaces.hpp>

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace S2LL
{

	// Edge-model tags: select the edge realization of a polygon through EdgeTraits
	namespace EdgeTag
	{
		// Straight line segment
		struct Straight;

		// Great sectional intersections with ellipsoids
		struct GreatSectional;

		// Geodesic (shortest) path
		struct Geodesic;
	}

	// Straight edge between two vertices
	template <typename V>
	struct LineSegment
	{
		V a, b;
	};

	// Shortest-path edge along the surface between two vertices. On a sphere this
	// is the minor great-circle arc; on a general ellipsoid it is the ellipsoidal
	// geodesic, whose shape might not be uniquely defined and depends on the
	// ellipsoid.
	//
	// Placeholder: currently stores only the endpoints; the ellipsoid enters the
	// picture through the edge factory (and will drive the arc's geometry once it
	// is implemented).
	struct GeodesicArc
	{
		E3 a, b;
	};

	// Edge traced by the intersection of the surface with the plane through the
	// two vertices (a great ellipse when the plane passes through the ellipsoid
	// center). Its shape depends on the ellipsoid's axes.
	//
	// Placeholder: currently stores only the endpoints; the ellipsoid enters the
	// picture through the edge factory.
	struct EllipticArc
	{
		E3 a, b;
	};

	// Edge-model traits. Every supported (geometry tag, vertex type) combination
	// specializes EdgeTraits with an edge_type and an edge(a, b, e) factory.
	// The primary template is intentionally undefined: an unspecialized
	// combination simply has no edge model.
	template <typename Tag, typename V>
	struct EdgeTraits;

	template <>
	struct EdgeTraits<EdgeTag::Straight, E2>
	{
		using edge_type = LineSegment<E2>;

		static constexpr LineSegment<E2> edge(const E2& a, const E2& b, const Ellipsoid&)
		{
			return LineSegment<E2>{ a, b };
		}
	};

	// In the Euclidean plane, geodesics are straight segments
	template <>
	struct EdgeTraits<EdgeTag::Geodesic, E2> : EdgeTraits<EdgeTag::Straight, E2>
	{
	};

	template <>
	struct EdgeTraits<EdgeTag::Straight, E3>
	{
		using edge_type = LineSegment<E3>;

		static constexpr LineSegment<E3> edge(const E3& a, const E3& b, const Ellipsoid&)
		{
			return LineSegment<E3>{ a, b };
		}
	};

	template <>
	struct EdgeTraits<EdgeTag::GreatSectional, E3>
	{
		using edge_type = EllipticArc;

		static constexpr EllipticArc edge(const E3& a, const E3& b, const Ellipsoid&)
		{
			return EllipticArc{ a, b };
		}
	};

	template <>
	struct EdgeTraits<EdgeTag::Geodesic, E3>
	{
		using edge_type = GeodesicArc;

		static constexpr GeodesicArc edge(const E3& a, const E3& b, const Ellipsoid&)
		{
			return GeodesicArc{ a, b };
		}
	};

	/// Polygonal region owning a cyclic vertex loop. The polygon owns the loop;
	/// the (Tag, V)-keyed edge model owns the inference from consecutive cyclic
	/// vertices to concrete edges (the closing edge is implicit).
	template <typename V, typename Tag = EdgeTag::Straight, size_t N = 0>
	struct PolygonBase
	{
		using vertex_type = V;
		using tag_type = Tag;
		using loop_type = Loop<V, N>;
		using edge_type = typename EdgeTraits<Tag, V>::edge_type;

		constexpr PolygonBase() = default;

		/// Constructs a polygon directly from its cyclic vertex list
		constexpr PolygonBase(std::initializer_list<V> list) : boundary(list) {}

		/// Cyclic list of vertices in boundary order (vector or array storage)
		loop_type boundary;

		/// Number of vertices
		constexpr size_t size() const noexcept { return boundary.size(); }

		/// Cyclic access to vertices
		constexpr const V& operator[](ptrdiff_t i) const noexcept { return boundary[i]; }
		constexpr V& operator[](ptrdiff_t i) noexcept { return boundary[i]; }

		/// Realizes the i-th edge on the given surface (unit sphere by default);
		/// wraps cyclically so the closing edge is included
		constexpr edge_type edge(ptrdiff_t i, const Ellipsoid& e = UnitSphere) const
		{
			return EdgeTraits<Tag, V>::edge(boundary[i], boundary[i + 1], e);
		}
	};

	/// Plane polygon (N = 0 means dynamic vector storage)
	template <size_t N = 0>
	using PlanePolygon = PolygonBase<E2, EdgeTag::Straight, N>;

	/// Space polygon (N = 0 means dynamic vector storage)
	template <size_t N = 0>
	using SpacePolygon = PolygonBase<E3, EdgeTag::Straight, N>;

	/// Great Elliptic Polygon (N = 0 means dynamic vector storage)
	template <size_t N = 0>
	using GEP = PolygonBase<E3, EdgeTag::GreatSectional, N>;

	/// Geodesic Surface Polygon (N = 0 means dynamic vector storage)
	template <size_t N = 0>
	using GP = PolygonBase<E3, EdgeTag::Geodesic, N>;

	/// Placeholder: Algebraic representation of Compound polygons
	template <typename T>
	struct Compound
	{

		std::vector<T> polygons;
	};

	// Fixed-size polygon flavors are plain-old-data: trivial
	// (including default construction) and standard-layout
	S2LL_ASSERT_POD(PlanePolygon<4>);
	S2LL_ASSERT_POD(SpacePolygon<4>);
	S2LL_ASSERT_POD(GEP<4>);
	S2LL_ASSERT_POD(GP<4>);
}

