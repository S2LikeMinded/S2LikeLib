
#ifndef __REGION_HPP__
#define __REGION_HPP__

#include <S2LL/Core/Coordinates.hpp>

#include <vector>

namespace S2LM
{

	struct Polygon
	{

		std::vector<S2LM::E2> vertices;
	};

	struct CompoundPolygon
	{

		std::vector<Polygon> polygons;
	};
}

#endif // !__REGION_HPP__
