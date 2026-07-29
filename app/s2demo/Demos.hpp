#pragma once

#include <iostream>

namespace S2Demo
{
	struct DemoContext;

	// Prints the list of available demos
	void PrintDemoList(std::ostream& ost);

	// Demo 1: Interactive spherical surface computation demo
	void RunDemo1(std::ostream& ost, DemoContext& ctx);
}
