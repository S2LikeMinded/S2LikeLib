
#include "S2EditConfig.hpp"

#include <S2LL/Parser/Shapefile.hpp>
#include <S2LL/Core/Regions.hpp>

#include <argparse/argparse.hpp>
#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/loopscheduler.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>


int main(int argc, char const *argv[])
{
	const std::string name = "s2edit";
	const std::string version =
		std::to_string(_S2LIKELIB_VERSION_MAJOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_MINOR) + "." +
		std::to_string(_S2LIKELIB_VERSION_PATCH);

	std::string inputPath;
	std::string inputExtension;

	// Shapefile parser
	bool asShapefile = false, isShapefile = false;
	auto shapefilePtr = std::make_unique<S2LM::Parser::Shapefile>();

	// Datum and Cartesian coordinates
	std::vector<S2LM::Compound<S2LM::Polygon>> cs;
	std::vector<S2LM::Compound<S2LM::GLPolygon>> cgs;

	// ==== argparse definitions for argument parsing ====================== //
	argparse::ArgumentParser prog(name, version);

	// Specifies an input path
	prog.add_argument("input")
		.help("Path of input")
		.default_value(std::string())
		.store_into(inputPath);

	// Specifies a .shp file and read associated index files
	auto &fmts = prog.add_mutually_exclusive_group();
	fmts.add_argument("--shapefile").flag()
		.help("Read all Esri Shapefiles associated with the path of input")
		.store_into(asShapefile);

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
				if (asShapefile || inputExtension == ".shp")
				{
					isShapefile = true;
				}

				// Parse according to the extension
				std::cout << "Input: " << inputPath << "\n";
				if (isShapefile)
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
	auto rootMenu = std::make_unique<cli::Menu>(name);
	rootMenu->Insert
	(
		"input",
		[&](std::ostream& ost)
		{
			ost << "\tInput: " << inputPath << "\n";
			if (isShapefile)
			{
				const auto& sf = *shapefilePtr;
				const auto& wkt = sf.prj.wkt;

				const auto& geogcs = wkt.depthFirstKey("GEOGCS");
				std::string gcsName = geogcs.key();
				std::string unitName = geogcs["UNIT"].name();
				std::string primeMName = geogcs["PRIMEM"].name();
				std::string datumName = geogcs["DATUM"].name();
				std::string spheroidName = geogcs["DATUM"]["SPHEROID"].name();
				double unit = geogcs["UNIT"][1].valueAsDouble();
				double primeM = geogcs["PRIMEM"][1].valueAsDouble();
				double equatorialRadius = geogcs["DATUM"]["SPHEROID"][1].valueAsDouble();
				double invFlat = geogcs["DATUM"]["SPHEROID"][2].valueAsDouble();
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
				
				if (!wkt.hasKey("PROJCS")) return;

				const auto& projcs = wkt.depthFirstKey("PROJCS");
				std::string pcsName = projcs.name();
				std::string prjName = projcs["PROJECTION"].name();
				std::string unitProjName = projcs["UNIT"].name();
				double falseE = projcs.depthFirstKey("PARAMETER", "False_Easting")[1].valueAsDouble();
				double falseN = projcs.depthFirstKey("PARAMETER", "False_Northing")[1].valueAsDouble();
				double centralM = projcs.depthFirstKey("PARAMETER", "Central_Meridian")[1].valueAsDouble();
				double standardP1 = projcs.depthFirstKey("PARAMETER", "Standard_Parallel_1")[1].valueAsDouble();
				double auxS = projcs.depthFirstKey("PARAMETER", "Auxiliary_Sphere_Type")[1].valueAsDouble();
				double unitProj = projcs["UNIT"][1].valueAsDouble();

				ost << "\tProjection Coordinate System: " << pcsName << "\n"
					<< "\tProjection Name: " << prjName << "\n"
					<< "\tFalse Easting: " << falseE << "\n"
					<< "\tFalse Northing: " << falseN << "\n"
					<< "\tCentral Meridian: " << centralM << "\n"
					<< "\tStandard Parallel 1: " << standardP1 << "\n"
					<< "\tFalse Easting: " << auxS << "\n"
					<< "\tUnit: " << unitProjName << " (" << unitProj << ")\n";
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
			if (!isShapefile)
			{
				return;
			}
			ost << "Converting...\n";
			cgs.clear();

			S2LM::E3 ellipsoid = shapefilePtr->prj.ellipsoid;
			double unit = shapefilePtr->prj.wkt["UNIT"][1].valueAsDouble();

			double r;
			S2LM::S2 spc;
			S2LM::E3 Q;
			cgs.reserve(cs.size());
			for (const auto& c : cs)
			{
				cgs.emplace_back();
				auto& cg = cgs.back();
				cg.polygons.reserve(c.polygons.size());
				for (const auto& p : c.polygons)
				{
					auto& g = cg.polygons.emplace_back();
					g.vertices.reserve(p.vertices.size());
					for (const auto& v : p.vertices)
					{
						spc.p = v.x * unit;
						spc.a = v.y * unit;
						r = ellipsoid.x * sin(spc.p);

						Q.x = r * cos(spc.a);
						Q.y = r * sin(spc.a);;
						Q.z = ellipsoid.y * cos(spc.p);
						g.vertices.push_back(Q);
					}
				}
			}
			ost << "Conversion complete!\n";
		},
		"Convert spherical coordinates to 3D Cartesian coordinates"
	);

	rootMenu->Insert
	(
		"export",
		[&](std::ostream& ost, const std::string& fileName)
		{
			ost << "Exporting...\n";

			auto outPath = std::filesystem::path(fileName);
			outPath.replace_extension(".s2lm");
			std::ofstream ofs(outPath);

			std::ios_base::fmtflags ioFlags;
			std::streamsize ioPrecision;
			{
				ioFlags = ofs.flags(std::ios::right);
				ioPrecision = ofs.precision(std::numeric_limits<double>::digits10);
			}
			for (const auto& c : cgs)
			{
				for (const auto& p : c.polygons)
				{
					for (const auto& v : p.vertices)
					{
						ofs << v << "\n";
					}
				}
			}
			{
				ofs.flags(ioFlags);
				ofs.precision(ioPrecision);
			}
			ost << "Export complete!\n";
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
	cli.EnterAction([&name,&version](std::ostream& ost) {
		ost << name << " Command-Line Interface " << version << "\n";
	});

	cli::LoopScheduler scheduler;
	cli::CliLocalTerminalSession localSession(cli, scheduler, std::cout);

	// Exit
	cli.ExitAction([&name,&scheduler](std::ostream& ost) {
		ost << name << " CLI Exiting...\n";
		scheduler.Stop();
	});

	// Start interactive facility
	scheduler.Run();

	return EXIT_SUCCESS;
}
