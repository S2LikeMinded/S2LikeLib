
#include <S2LL/Point.hpp>

Point::Point(double polar, double azimuth) :
	p(polar),
	a(azimuth)
{

}

std::ostream& operator<<(std::ostream& ost, const Point& point)
{
	ost << point.x << " " << point.y << " " << point.z;
	return ost;
}
