#include "S2EditConfig.hpp"
#include "Commands.hpp"

#include <argparse/argparse.hpp>
#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/loopscheduler.h>

#include <filesystem>
#include <iostream>
#include <locale>
#include <memory>
#include <string>

int main(int argc, char const *argv[])
{
	const std::string name = "S2Edit";
	const std::string version =
		std::to_string(_S2LIKELIB_VERSION_MAJOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_MINOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_PATCH);

	S2Edit::EditContext ctx;

	// ==== argparse definitions for argument parsing ====================== //
	argparse::ArgumentParser prog(name, version);

	prog.add_argument("input")
		.help("Path of input")
		.default_value(std::string())
		.store_into(ctx.inputPath);

	auto &fmts = prog.add_mutually_exclusive_group();
	fmts.add_argument("--shapefile").flag()
		.help("Read all Esri Shapefiles associated with the path of input")
		.store_into(ctx.asShapefile);

	prog.add_argument("--verbose").flag()
		.help("provides verbose information");

	prog.add_description("Ellipsoidal region editor. Part of SphereLikeLib.");
	prog.add_epilog("(c) 2025 S2LikeMinded");

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

	// ==== detect input format and parse ================================== //
	try
	{
		S2Edit::ParseInput(ctx);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// ==== Configure interactive facility ================================= //
	auto rootMenu = std::make_unique<cli::Menu>(name);
	S2Edit::RegisterCliCommands(*rootMenu, ctx);

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
