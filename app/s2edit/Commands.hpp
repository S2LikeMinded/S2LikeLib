#pragma once

#include <S2LL/Parser/Shapefile.hpp>
#include <S2LL/Core/Regions.hpp>
#include <S2LL/Core/Surfaces.hpp>

#include <cli/cli.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace S2Edit
{
	// Application state context
	struct EditContext
	{
		std::string inputPath;
		std::string inputExtension;
		bool asShapefile = false;
		bool isShapefile = false;
		std::unique_ptr<S2LL::Parser::Shapefile> shapefilePtr = std::make_unique<S2LL::Parser::Shapefile>();
		std::vector<S2LL::Compound<S2LL::Polygon>> cs;
		std::vector<S2LL::Compound<S2LL::GLPolygon>> cgs;
	};

	// Parses input path into EditContext
	void ParseInput(EditContext& ctx);

	// Registers all interactive CLI menu commands
	void RegisterCliCommands(cli::Menu& rootMenu, EditContext& ctx);
}
