
#include <S2LL/Core/Point.hpp>

using S2LM::Point;

Point::Point():
	c(false), s(false)
{

}

Point::Point(double x, double y, double z) :
	c(true), s(false),
	x(x), y(y), z(z)
{

}

Point::Point(double polar, double azimuth) :
	c(false), s(true),
	p(polar), a(azimuth)
{

}
