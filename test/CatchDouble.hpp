#pragma once

#include <catch2/catch_tostring.hpp>
#include <S2LL/Core/Numerics.hpp>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace Catch
{
	template <>
	struct StringMaker<S2LL::Double>
	{
		static std::string convert(const S2LL::Double& value)
		{
			std::ostringstream oss;
			oss << std::setprecision(std::numeric_limits<double>::max_digits10)
			    << "Double{ hi: " << value.hi << ", lo: " << value.lo << " }";
			return oss.str();
		}
	};
}
