#pragma once

#include <S2LL/Core/Coordinates.hpp>

#include <vector>

namespace S2LM
{

	struct Polygon
	{

		std::vector<S2LM::E2> vertices;
	};

	struct GeodesicLikePolygon
	{

		std::vector<S2LM::E3> vertices;
	};

	typedef GeodesicLikePolygon GLPolygon;

	template <typename T>
	struct Compound
	{

		std::vector<T> polygons;
	};
}

