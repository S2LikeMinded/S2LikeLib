#include "Commands.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <iomanip>
#include <locale>

namespace S2Edit
{
	void ParseInput(EditContext& ctx)
	{
		if (ctx.inputPath.empty())
			return;

		if (std::filesystem::is_directory(ctx.inputPath))
			throw std::runtime_error("directory not supported as input");

		if (!std::filesystem::is_regular_file(ctx.inputPath))
			throw std::runtime_error("input not a file or directory");

		std::filesystem::path input(ctx.inputPath);
		ctx.inputExtension = input.extension().string();
		std::locale locale;
		for (auto &c : ctx.inputExtension)
			c = std::tolower(c, locale);

		if (ctx.asShapefile || ctx.inputExtension == ".shp")
			ctx.isShapefile = true;

		std::cout << "Input: " << ctx.inputPath << "\n";
		if (!ctx.isShapefile)
			throw std::runtime_error(ctx.inputExtension + " not supported");

		ctx.shapefilePtr = std::make_unique<Parser::Shapefile>();
		ctx.shapefilePtr->parse(input);
		ctx.cs = ctx.shapefilePtr->regions;
	}

	void RegisterCliCommands(cli::Menu& rootMenu, EditContext& ctx)
	{
		rootMenu.Insert(
			"load",
			{"path"},
			[&ctx](std::ostream& ost, const std::string& path)
			{
				try
				{
					ctx.inputPath = path;
					ctx.asShapefile = true;
					ParseInput(ctx);
					ost << "Loaded " << ctx.cs.size() << " compound polygon(s) from " << path << "\n";
				}
				catch (const std::exception& e)
				{
					ost << "Error loading " << path << ": " << e.what() << "\n";
				}
			},
			"Load an Esri Shapefile: load <path>"
		);

		rootMenu.Insert(
			"input",
			[&ctx](std::ostream& ost)
			{
				ost << "\tInput: " << ctx.inputPath << "\n";
				if (ctx.isShapefile)
				{
					const auto& sf = *ctx.shapefilePtr;
					const auto& wkt = sf.prj.wkt;

					if (!wkt.hasKey("GEOGCS"))
					{
						ost << "GEOGCS[] not found\n";
						return;
					}

					const auto& geogcs = wkt("GEOGCS");
					const auto& datum = geogcs["DATUM"];
					std::string gcsName = geogcs.name();
					std::string unitName = geogcs["UNIT"].name();
					std::string primeMName = geogcs["PRIMEM"].name();
					std::string datumName = datum.name();
					std::string spheroidName = datum["SPHEROID"].name();
					double unit = geogcs["UNIT"].getDouble();
					double primeM = geogcs["PRIMEM"].getDouble();
					double equatorialRadius = datum["SPHEROID"].getDouble(1);
					double invFlattening = datum["SPHEROID"].getDouble(2);
					Ellipsoid ellipsoid(equatorialRadius, invFlattening);
					double polarRadius = ellipsoid.minor();

					ost << std::format("\tGeographic Coordinate System: {}\n", gcsName)
						<< std::format("\tAngular Unit: {} ({})\n", unitName, unit)
						<< std::format("\tPrime Meridian: {} ({})\n", primeMName, primeM)
						<< std::format("\tDatum: {}\n", datumName)
						<< std::format("\tSpheroid: {}\n", spheroidName)
						<< std::format("\tSemimajor Axis: {:.15g}\n", equatorialRadius)
						<< std::format("\tSemiminor Axis: {:.15g}\n", polarRadius)
						<< std::format("\tInverse Flattening: {:.15g}\n", invFlattening);

					if (!wkt.hasKey("PROJCS"))
					{
						ost << "PROJCS[] not found\n";
						return;
					}

					const auto& projcs = wkt("PROJCS");
					std::string pcsName = projcs.name();
					std::string prjName = projcs["PROJECTION"].name();
					std::string unitProjName = projcs["UNIT"].name();
					double falseE = projcs("PARAMETER", "False_Easting").getDouble();
					double falseN = projcs("PARAMETER", "False_Northing").getDouble();
					double centralM = projcs("PARAMETER", "Central_Meridian").getDouble();
					double standardP1 = projcs("PARAMETER", "Standard_Parallel_1").getDouble();
					double auxS = projcs("PARAMETER", "Auxiliary_Sphere_Type").getDouble();
					double unitProj = projcs["UNIT"].getDouble();

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

		rootMenu.Insert(
			"count",
			[&ctx](std::ostream& ost, const std::vector<std::string>& argv)
			{
				ost << "\tE2 compound polygons: " << ctx.cs.size() << "\n"
					<< "\tE3 compound polygons: " << ctx.cgs.size() << "\n";

				if (argv.size() == 1 && argv[0] == "detail")
				{
					for (size_t i = 0; i < ctx.cs.size(); ++i)
					{
						const auto& cpoly = ctx.cs[i];
						ost << "cPoly[" << i << "]: "
							<< cpoly.polygons.size() << " E2 polygon(s)\n";
					}
					for (size_t i = 0; i < ctx.cgs.size(); ++i)
					{
						const auto& cglpoly = ctx.cgs[i];
						ost << "cGLPoly[" << i << "]: "
							<< cglpoly.polygons.size() << " E3 polygon(s)\n";
					}
				}
			},
			"Numerics about loaded regions: \"detail\""
		);

		rootMenu.Insert(
			"convert",
			[&ctx](std::ostream& ost, const std::vector<std::string>& argv)
			{
				ost << "Converting...\n";
				ctx.cgs.clear();

				Ellipsoid ellipsoid = UnitSphere;
				double unit = 1.0;

				if (ctx.isShapefile)
				{
					const auto& sf = *ctx.shapefilePtr;
					const auto& wkt = sf.prj.wkt;

					if (wkt.hasKey("GEOGCS"))
					{
						const auto& geogcs = wkt("GEOGCS");
						unit = geogcs["UNIT"].getDouble();

						double a = geogcs("SPHEROID").getDouble(1);
						double invF = geogcs("SPHEROID").getDouble(2);
						ellipsoid = Ellipsoid(a, invF);
					}
				}

				S2 spc;
				ctx.cgs.reserve(ctx.cs.size());
				for (const auto& c : ctx.cs)
				{
					ctx.cgs.emplace_back();
					auto& cg = ctx.cgs.back();
					cg.polygons.reserve(c.polygons.size());
					for (const auto& p : c.polygons)
					{
						auto& g = cg.polygons.emplace_back();
						g.boundary.vertices.reserve(p.boundary.vertices.size());
						for (const auto& v : p.boundary.vertices)
						{
							spc.p = v.x * unit;
							spc.a = v.y * unit;
							g.boundary.vertices.push_back(ellipsoid.to_E3(spc));
						}
					}
				}
				ost << "Conversion complete!\n";
			},
			"Convert spherical coordinates to 3D Cartesian coordinates"
		);

		rootMenu.Insert(
			"export",
			[&ctx](std::ostream& ost, const std::string& fileName)
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
				for (const auto& c : ctx.cgs)
				{
					ofs << "====\n";
					for (const auto& p : c.polygons)
					{
						ofs << "----\n";
						for (const auto& v : p.boundary.vertices)
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

		rootMenu.Insert(
			"region",
			[&ctx](std::ostream& ost, const std::vector<std::string>& argv)
			{
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

				int index = std::stoi(argv[0]);
				if (index < 0 || static_cast<size_t>(index) >= ctx.cs.size())
				{
					ost << "Only " << ctx.cs.size() << " compound polygons\n";
					return;
				}
				const auto& cpoly = ctx.cs[index];

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
					ost << std::format("cPoly[{}][{}]:\n", index, j);

					std::ios_base::fmtflags ioFlags;
					std::streamsize ioPrecision;
					{
						ioFlags = ost.flags(std::ios::right);
						ioPrecision = ost.precision(std::numeric_limits<double>::digits10);
					}
					for (size_t k = 0; k < poly.boundary.vertices.size(); ++k)
					{
						const auto& vertex = poly.boundary.vertices[k];
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
	}
}
