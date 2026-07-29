#pragma once

#include <cli/cli.h>

namespace S2Demo
{
	// Empty application state context for S2Demo
	struct DemoContext
	{
	};

	// Registers default CLI commands (empty for now)
	void RegisterCliCommands(cli::Menu& rootMenu, DemoContext& ctx);
}
