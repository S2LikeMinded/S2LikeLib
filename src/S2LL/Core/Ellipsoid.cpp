#include <cmath>
#include <S2LL/Core/Surfaces.hpp>
#include <S2LL/Core/Numerics.hpp>

namespace S2LL
{
	Ellipsoid::Ellipsoid(double r)
		: a(r), b(r), c{r, 0.0}
	{
	}

	Ellipsoid::Ellipsoid(double a, double inv_f)
		: a(a), b(a), c(S2LL::sub(a, S2LL::div(a, inv_f)))
	{
	}

	Ellipsoid::Ellipsoid(double a, double b, double c)
		: a(a), b(b), c{c, 0.0}
	{
	}

	double Ellipsoid::inv_f() const
	{
		return static_cast<double>(S2LL::div(a, S2LL::sub(a, c)));
	}
}
