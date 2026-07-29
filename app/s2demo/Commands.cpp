#include "Commands.hpp"
#include "Demos.hpp"

#include <iostream>

namespace S2Demo
{
	void PrintDemoList(std::ostream& ost)
	{
		ost << "Available Demos:\n";
		ost << "  1: Demo 1\n";
	}

	void RegisterCliCommands(cli::Menu& rootMenu, DemoContext& ctx)
	{
		rootMenu.Insert(
			"list",
			[](std::ostream& ost)
			{
				PrintDemoList(ost);
			},
			"List all available demos"
		);

		rootMenu.Insert(
			"run",
			[](std::ostream& ost)
			{
				ost << "Please specify a demo ID (e.g., 'run 1'). Type 'list' to view available demos.\n";
			},
			"Run a demo: run <id>"
		);

		rootMenu.Insert(
			"run",
			{"id"},
			[&ctx](std::ostream& ost, int id)
			{
				switch (id)
				{
				case 1:
					RunDemo1(ost, ctx);
					break;
				default:
					ost << "Unknown demo ID: " << id << ". Type 'list' to view available demos.\n";
					break;
				}
			},
			"Run a demo by ID: run <id>"
		);
	}
}
