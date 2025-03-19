
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

	// Shapefile parser
	bool asShapeFile = false, isShapeFile = false;
	auto shapefilePtr = std::make_unique<S2LM::Parser::Shapefile>();

	// Datum and Cartesian coordinates
	std::vector<S2LM::Compound<S2LM::Polygon>> cs;
	std::vector<S2LM::Compound<S2LM::GLPolygon>> cgs;

	// ==== argparse definitions for argument parsing ====================== //
	argparse::ArgumentParser prog("s2edit", version);

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
				if (asShapeFile || inputExtension == ".shp")
				{
					isShapeFile = true;
				}

				// Parse according to the extension
				std::cout << "Input: " << inputPath << "\n";
				if (isShapeFile)
				{
					shapefilePtr->parse(input);
					cs = shapefilePtr->regions;
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
			else
			{
				throw std::runtime_error("input not a file or directory");
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
	rootMenu->Insert
	(
		"input",
		[&](std::ostream& ost)
		{
			ost << "\tInput: " << inputPath << "\n";
			if (isShapeFile)
			{
				const auto& sf = *shapefilePtr;
				const auto& wkt = sf.prj.wkt;

				std::string gcsName = wkt.key();
				std::string unitName = wkt["UNIT"].name();
				std::string primeMName = wkt["PRIMEM"].name();
				std::string datumName = wkt["DATUM"].name();
				std::string spheroidName = wkt["DATUM"]["SPHEROID"].name();
				double unit = wkt["UNIT"][1].valueAsDouble();
				double primeM = wkt["PRIMEM"][1].valueAsDouble();
				double equatorialRadius = wkt["DATUM"]["SPHEROID"][1].valueAsDouble();
				double invFlat = wkt["DATUM"]["SPHEROID"][2].valueAsDouble();
				double polarRadius = equatorialRadius - equatorialRadius / invFlat;

				std::ios_base::fmtflags ioFlags;
				std::streamsize ioPrecision;
				{
					ioFlags = ost.flags(std::ios::right);
					ioPrecision = ost.precision(std::numeric_limits<double>::digits10);
				}
				ost << "\tGeographic Coordinate System: " << gcsName << "\n"
					<< "\tAngular Unit: " << unitName << " (" << unit << ")\n"
					<< "\tPrime Meridian: " << primeMName << " (" << primeM << ")\n"
					<< "\tDatum: " << datumName << "\n"
					<< "\tSpheroid: " << spheroidName << "\n"
					<< "\tSemimajor Axis: " << equatorialRadius << "\n"
					<< "\tSemiminor Axis: " << polarRadius << "\n"
					<< "\tInverse Flattening: " << invFlat << "\n";
				{
					ost.flags(ioFlags);
					ost.precision(ioPrecision);
				}
			}
		},
		"Information about the input"
	);

	rootMenu->Insert
	(
		"count",
		[&](std::ostream& ost, const std::vector<std::string>& argv)
		{
			// count
			ost << "\tE2 compound polygons: " << cs.size() << "\n"
				<< "\tE3 compound polygons: " << cgs.size() << "\n";

			// count detail
			if (argv.size() == 1 && argv[0] == "detail")
			{
				for (size_t i = 0; i < cs.size(); ++i)
				{
					const auto& cpoly = cs[i];
					ost << "cPoly[" << i << "]: "
						<< cpoly.polygons.size() << " E2 polygon(s)\n";
				}
				for (size_t i = 0; i < cgs.size(); ++i)
				{
					const auto& cglpoly = cgs[i];
					ost << "cGLPoly[" << i << "]: "
						<< cglpoly.polygons.size() << " E3 polygon(s)\n";
				}
			}
		},
		"Numerics about loaded regions: \"detail\""
	);

	rootMenu->Insert
	(
		"convert",
		[&](std::ostream& ost, const std::vector<std::string>& argv)
		{
			std::cout << "Converting...\n";
			std::cout << "Conversion complete!\n";
		},
		"Convert E2 coordinates to E3 coordinates"
	);

	rootMenu->Insert
	(
		"export",
		[&](std::ostream& ost, const std::string& fileName)
		{
			std::cout << "Exporting...\n";
			std::cout << "Export complete!\n";
		},
		"Export to <fileName>.s2lm"
	);

	rootMenu->Insert
	(
		"region",
		[&](std::ostream& ost, const std::vector<std::string>& argv)
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
			if (index >= cs.size())
			{
				ost << "Only " << cs.size() << " compound polygons\n";
				return;
			}
			const auto& cpoly = cs[index];

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

				std::ios_base::fmtflags ioFlags;
				std::streamsize ioPrecision;
				{
					ioFlags = ost.flags(std::ios::right);
					ioPrecision = ost.precision(std::numeric_limits<double>::digits10);
				}
				for (size_t k = 0; k < poly.vertices.size(); ++k)
				{
					const auto& vertex = poly.vertices[k];
					ost << "    " << vertex << "\n";
				}
				{
					ost.flags(ioFlags);
					ost.precision(ioPrecision);
				}
			}
		},
		"Display information about region: <index> [<subindex>]"
	);

	// Setup interactive facility
	cli::Cli cli(std::move(rootMenu));

	// Entry
	cli.EnterAction([&version](std::ostream& ost) {
		ost << "s2edit Command-Line Interface " << version << "\n";
	});

	cli::LoopScheduler scheduler;
	cli::CliLocalTerminalSession localSession(cli, scheduler, std::cout);

	// Exit
	cli.ExitAction([&scheduler](std::ostream& ost) {
		ost << "s2edit CLI Exiting...\n";
		scheduler.Stop();
	});

	// Start interactive facility
	scheduler.Run();

	return EXIT_SUCCESS;
}
