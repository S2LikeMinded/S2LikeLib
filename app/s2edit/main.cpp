
#include "S2EditConfig.hpp"

#include <S2LL/Point.hpp>

#include <iostream>

int main(int argc, char const *argv[])
{
	std::cout << "s2edit version "
		<< _S2EDIT_VERSION_MAJOR << "."
		<< _S2EDIT_VERSION_MINOR << "."
		<< _S2EDIT_VERSION_PATCH << "\n";

	Point p(0., 0.);
	std::cout << p << "\n";
	return 0;
}
