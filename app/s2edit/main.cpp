
#include "S2EditConfig.hpp"

#include <S2LL/Parser/Shapefile.hpp>
#include <S2LL/Core/Region.hpp>

#include <argparse/argparse.hpp>
#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/loopscheduler.h>

#include <filesystem>

int main(int argc, char const *argv[])
{
	std::string inputPath;
	bool asShapeFile = false;

	std::vector<S2LM::Region> regions;

	// ==== argparse definitions for argument parsing ====================== //
	argparse::ArgumentParser prog
	(
		"S2Edit",
		std::to_string(_S2LIKELIB_VERSION_MAJOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_MINOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_PATCH)
	);

	// Specifies an input path
	prog.add_argument("input")
		.help("Path of input")
		.default_value(std::string())
		.store_into(inputPath);

	// Specifies a .shp file and read associated index files
	auto &fmts = prog.add_mutually_exclusive_group();
	fmts.add_argument("--shapefile").flag()
		.help("Read all Esri Shapefiles associated with the path of input")
		.store_into(asShapeFile);

	// Enables verbose mode
	prog.add_argument("--verbose").flag()
		.help("provides verbose information");

	// Provides additional information
	prog.add_description("Ellipsoidal region editor. Part of SphereLikeLib.");
	prog.add_epilog("(c) 2025 S2LikeMinded");

	// Parse arguments and exit on errors
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
		if (!inputPath.empty())
		{
			if (std::filesystem::is_regular_file(inputPath))
			{
				if (asShapeFile)
				{
					// S2LM::Parser::Shapefile parser(inputPath);
					// regions.push_back(parser.getRegions());
				}
			}
			else if (std::filesystem::is_directory(inputPath))
			{

			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// ==== Configure interactive facility ================================= //
	auto rootMenu = std::make_unique<cli::Menu>("s2edit");
	rootMenu->Insert("input", [&inputPath](std::ostream& out){
		out << "Path: " << inputPath << std::endl;
	}, "Print information about the input");

	// Setup interactive facility
	cli::Cli cli(std::move(rootMenu));
	cli::LoopScheduler scheduler;
	cli::CliLocalTerminalSession localSession(cli, scheduler, std::cout);

	// Entry
	cli.EnterAction([](std::ostream& out) {
		out << "Starting s2edit CLI facility..." << std::endl;
	});

	// Exit
	cli.ExitAction([&scheduler](std::ostream& ost) {
		ost << "Exiting..." << std::endl;
		scheduler.Stop();
	});

	// Start interactive facility
	scheduler.Run();

	return EXIT_SUCCESS;
}
