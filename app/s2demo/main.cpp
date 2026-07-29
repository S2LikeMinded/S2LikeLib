#include "S2DemoConfig.hpp"
#include "Commands.hpp"
#include "Demos.hpp"

#include <argparse/argparse.hpp>
#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/loopscheduler.h>

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char const *argv[])
{
	const std::string name = "S2Demo";
	const std::string version =
		std::to_string(_S2LIKELIB_VERSION_MAJOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_MINOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_PATCH);

	S2Demo::DemoContext ctx;

	// ==== argparse definitions for argument parsing ====================== //
	argparse::ArgumentParser prog(name, version);

	prog.add_description("SphereLikeLib Demo Application.");
	prog.add_epilog("(c) 2025 S2LikeMinded");

	prog.add_argument("demo")
		.help("Demo ID to run directly (e.g. 1)")
		.scan<'i', int>()
		.default_value(0)
		.remaining();

	prog.add_argument("-d", "--demo")
		.help("Demo ID to run directly (e.g. -d 1)")
		.scan<'i', int>()
		.default_value(0);

	try
	{
		prog.parse_args(argc, argv);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << prog;
		return EXIT_FAILURE;
	}

	int demo_id = 0;
	if (prog.is_used("demo"))
	{
		demo_id = prog.get<int>("demo");
	}
	else if (prog.is_used("--demo"))
	{
		demo_id = prog.get<int>("--demo");
	}

	if (demo_id != 0)
	{
		if (demo_id == 1)
		{
			S2Demo::RunDemo1(std::cout, ctx);
			return EXIT_SUCCESS;
		}
		else
		{
			std::cerr << "Unknown demo ID: " << demo_id << ". Type 'S2Demo' to launch interactive shell or 'S2Demo 1' for Demo 1.\n";
			return EXIT_FAILURE;
		}
	}

	// ==== Configure interactive facility ================================= //
	auto rootMenu = std::make_unique<cli::Menu>(name);
	S2Demo::RegisterCliCommands(*rootMenu, ctx);

	cli::Cli cli(std::move(rootMenu));

	cli.EnterAction([&name, &version](std::ostream& ost) {
		ost << name << " Command-Line Interface " << version << "\n";
	});

	cli::LoopScheduler scheduler;
	cli::CliLocalTerminalSession localSession(cli, scheduler, std::cout);

	cli.ExitAction([&name, &scheduler](std::ostream& ost) {
		ost << name << " CLI Exiting...\n";
		scheduler.Stop();
	});

	scheduler.Run();

	return EXIT_SUCCESS;
}
