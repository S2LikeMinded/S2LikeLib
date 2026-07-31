#pragma once

#include <S2LL/Parser/Shapefile.hpp>
#include <S2LL/Core/Regions.hpp>
#include <S2LL/Core/Surfaces.hpp>

#include <cli/cli.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace S2LL;

namespace S2Edit
{
	// Application state context
	struct EditContext
	{
		std::string inputPath;
		std::string inputExtension;
		bool asShapefile = false;
		bool isShapefile = false;
		std::unique_ptr<Parser::Shapefile> shapefilePtr = std::make_unique<Parser::Shapefile>();
		std::vector<Compound<PlanePolygon<>>> cs;
		std::vector<Compound<GP<>>> cgs;
	};

	// Parses input path into EditContext
	void ParseInput(EditContext& ctx);

	// Registers all interactive CLI menu commands
	void RegisterCliCommands(cli::Menu& rootMenu, EditContext& ctx);
}
