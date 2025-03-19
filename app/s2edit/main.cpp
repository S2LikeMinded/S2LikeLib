
#include "S2EditConfig.hpp"

#include <S2LL/Parser/Shapefile.hpp>
#include <S2LL/Core/Regions.hpp>

#include <argparse/argparse.hpp>
#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/loopscheduler.h>

#include <algorithm>
#include <filesystem>
#include <locale>
#include <iomanip>

int main(int argc, char const *argv[])
{
	std::string version =
		std::to_string(_S2LIKELIB_VERSION_MAJOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_MINOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_PATCH);
	std::string inputPath;
	std::string inputExtension;
	bool asShapeFile = false;

	std::vector<S2LM::CompoundPolygon> cpolyRegions;

	// ==== argparse definitions for argument parsing ====================== //
	argparse::ArgumentParser prog("S2Edit", version);

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
				// Detect file extension and force upper case
				std::filesystem::path input(inputPath);
				inputExtension = input.extension().string();
				{
					std::locale locale;
					for (auto &c: inputExtension)
						c = std::tolower(c, locale);
				}

				// Overrides file extension if specified
				if (asShapeFile)
				{
					inputExtension = ".shp";
				}

				// Parse according to the extension
				std::cout << "Input: " << inputPath << "\n";
				if (inputExtension == ".shp")
				{
					S2LM::Parser::Shapefile parser(input);
					cpolyRegions = parser.regions;
				}
				else
				{
					throw std::runtime_error(inputExtension + " not supported");
				}
			}
			else if (std::filesystem::is_directory(inputPath))
			{
				throw std::runtime_error("directory not supported as input");
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// ==== Configure interactive facility ================================= //
	auto rootMenu = std::make_unique<cli::Menu>("S2Edit");
	rootMenu->Insert
	(
		"input",
		[&inputPath](std::ostream& ost)
		{
			ost << "Input: " << inputPath << "\n";
		},
		"Information about the input"
	);

	rootMenu->Insert
	(
		"stats",
		[&cpolyRegions](std::ostream& ost, const std::vector<std::string>& argv)
		{
			// stats
			ost << "Count: " << cpolyRegions.size() << " cPoly(s)\n";
			if (argv.empty()) return;

			// stats list
			if (argv[0] == "list")
			{
				for (size_t i = 0; i < cpolyRegions.size(); ++i)
				{
					const auto& cpoly = cpolyRegions[i];
					ost << "cPoly[" << i << "]: "
						<< cpoly.polygons.size() << " Poly(s)\n";
				}
				ost << "\n";
			}
		},
		"Numerics about loaded regions: <list>"
	);

	rootMenu->Insert
	(
		"region",
		[&cpolyRegions](std::ostream& ost, const std::vector<std::string>& argv)
		{
			// region
			if (argv.empty())
			{
				ost << "Must supply <index>\n";
				return;
			}
			else if (argv.size() > 2)
			{
				ost << "Too many arguments\n";
				return;
			}

			// region <index> [<subindex>]
			int index = std::stoi(argv[0]);
			if (index >= cpolyRegions.size())
			{
				ost << "Only " << cpolyRegions.size() << " compound polygons\n";
				return;
			}
			const auto& cpoly = cpolyRegions[index];

			size_t subIndexStart, subIndexFinal;
			if (argv.size() == 2)
			{
				size_t subIndex = (size_t)std::stoi(argv[1]);
				if (subIndex >= cpoly.polygons.size())
				{
				ost << "Only " << cpoly.polygons.size() << " polygons\n";
					return;
				}
				subIndexStart = subIndex;
				subIndexFinal = subIndexStart + 1;
			}
			else
			{
				subIndexStart = 0;
				subIndexFinal = cpoly.polygons.size();
			}

			for (size_t j = subIndexStart; j < subIndexFinal; ++j)
			{
				const auto& poly = cpoly.polygons[j];
				ost << "cPoly[" << index << "][" << j << "]:\n";
				std::ios_base::fmtflags ioFlags =
					ost.flags(std::ios::right);
				std::streamsize ioPrecision =
					ost.precision(std::numeric_limits<double>::digits10);
				for (size_t k = 0; k < poly.vertices.size(); ++k)
				{
					const auto& vertex = poly.vertices[k];
					ost << "    " << vertex.x << " " << vertex.y << "\n";
				}
				ost.flags(ioFlags);
				ost.precision(ioPrecision);
			}
		},
		"Display information about region: <index> [<subindex>]"
	);

	// Setup interactive facility
	cli::Cli cli(std::move(rootMenu));

	// Entry
	cli.EnterAction([&version](std::ostream& ost) {
		ost << "S2Edit Command-Line Interface " << version << "\n";
	});

	cli::LoopScheduler scheduler;
	cli::CliLocalTerminalSession localSession(cli, scheduler, std::cout);

	// Exit
	cli.ExitAction([&scheduler](std::ostream& ost) {
		ost << "S2Edit CLI Exiting...\n";
		scheduler.Stop();
	});

	// Start interactive facility
	scheduler.Run();

	return EXIT_SUCCESS;
}
