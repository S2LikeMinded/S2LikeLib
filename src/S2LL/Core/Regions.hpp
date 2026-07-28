#pragma once

#include <S2LL/Core/Coordinates.hpp>

#include <vector>

namespace S2LL
{

	struct Polygon
	{

		std::vector<S2LL::E2> vertices;
	};

	struct GeodesicLikePolygon
	{

		std::vector<S2LL::E3> vertices;
	};

	typedef GeodesicLikePolygon GLPolygon;

	template <typename T>
	struct Compound
	{

		std::vector<T> polygons;
	};
}

