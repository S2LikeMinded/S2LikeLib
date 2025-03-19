
#include <S2LL/Parser/Shapefile.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using S2LM::Parser::Shapefile;

unsigned
read_unsigned_small_endian(std::istream &ifs, char buffer[4])
{
	ifs.read(buffer, 4);
	return (unsigned char)buffer[0]
		| ((unsigned char)buffer[1]<<8)
		| ((unsigned char)buffer[2]<<16)
		| ((unsigned char)buffer[3]<<24);
}

unsigned
read_unsigned_big_endian(std::istream &ifs, char buffer[4])
{
	ifs.read(buffer, 4);
	return (unsigned char)buffer[3]
		| ((unsigned char)buffer[2]<<8)
		| ((unsigned char)buffer[1]<<16)
		| ((unsigned char)buffer[0]<<24);
}

double
read_double_small_endian(std::istream &ifs, char buffer[4])
{
	unsigned hi = read_unsigned_small_endian(ifs, buffer);
	unsigned lo = read_unsigned_small_endian(ifs, buffer);
	uint64_t dword = (uint64_t)hi | ((uint64_t)lo<<32);
	double value = *((double *)(&dword));
	return value;
}

Shapefile::SHPReader::SHPReader(const std::filesystem::path& path)
{
	std::ifstream ifs(path, std::ios::binary);

	// Byte buffer
	char buffer[4];

	// ==== Parse File Header (100 bytes) ================================== //
	header.fileCode = (int)read_unsigned_big_endian(ifs, buffer);
	ifs.ignore(5 * 4); // Reserved bytes 4 to 23
	header.fileLength = (int)read_unsigned_big_endian(ifs, buffer);
	header.version = (int)read_unsigned_small_endian(ifs, buffer);
	header.shapeType = (ShapeType)read_unsigned_small_endian(ifs, buffer);
	header.xMin = read_double_small_endian(ifs, buffer);
	header.yMin = read_double_small_endian(ifs, buffer);
	header.xMax = read_double_small_endian(ifs, buffer);
	header.yMax = read_double_small_endian(ifs, buffer);
	header.zMin = read_double_small_endian(ifs, buffer);
	header.zMax = read_double_small_endian(ifs, buffer);
	header.mMin = read_double_small_endian(ifs, buffer);
	header.mMax = read_double_small_endian(ifs, buffer);

	// ==== Parse Records: Record Header (8 bytes) + Record Contents ======= //
	Record record;
	while (ifs.peek() != EOF)
	{
		record.number = (int)read_unsigned_big_endian(ifs, buffer);
		record.contentLength = (int)read_unsigned_big_endian(ifs, buffer);
		record.shapeType = (ShapeType)read_unsigned_small_endian(ifs, buffer);
		record.content.resize((record.contentLength - 2) * 2); //
		ifs.read(record.content.data(), record.content.size());
		records.push_back(record);
	}
}

Shapefile::PRJReader::PRJReader(const std::filesystem::path& path)
{
	std::string content;
	{
		std::ifstream ifs(path);
		std::stringstream sst;
		sst << ifs.rdbuf();
		content = sst.str();
	}

	bool enquoted = false;
	std::string literal;
	std::vector<WKTree::NodePtr> ptrs;

	std::cout << "Start\n";

	for (char c : content)
	{
		std::cout << "Parsing '" << c << "':\n";

		if (enquoted)
		{
			literal += c;
			if (c == '\"')
			{
				enquoted = false;

				auto newKeywordPtr = std::make_shared<WKTree::Node>();
				ptrs.emplace_back(newKeywordPtr);

				newKeywordPtr->complete = true;
				newKeywordPtr->nameOrValue = literal;
				literal.clear();
			}
		}
		else if (c == '\"')
		{
			literal += c;
			enquoted = true;
		}
		else if (c == '[')
		{
			auto newKeywordPtr = std::make_shared<WKTree::Node>();
			ptrs.emplace_back(newKeywordPtr);

			newKeywordPtr->complete = false;
			newKeywordPtr->nameOrValue = literal;
			literal.clear();
		}
		else if (c == ']')
		{
			if (literal.empty())
			{
				auto childPtr = ptrs.back();
				ptrs.pop_back();

				auto parentPtr = ptrs.back();
				parentPtr->complete = true;
				parentPtr->values.push_back(childPtr);
			}
			else
			{
				auto newKeywordPtr = std::make_shared<WKTree::Node>();
				newKeywordPtr->complete = true;
				newKeywordPtr->nameOrValue = literal;
				literal.clear();

				auto childPtr = ptrs.back();
				childPtr->complete = true;
				childPtr->values.push_back(newKeywordPtr);
			}
		}
		else if (c == ',')
		{
			if (literal.empty())
			{
				auto childPtr = ptrs.back();
				ptrs.pop_back();

				auto parentPtr = ptrs.back();
				parentPtr->values.push_back(childPtr);
			}
			else
			{
				auto newKeywordPtr = std::make_shared<WKTree::Node>();
				newKeywordPtr->complete = true;
				newKeywordPtr->nameOrValue = literal;
				literal.clear();

				auto parentPtr = ptrs.back();
				parentPtr->values.push_back(newKeywordPtr);
			}
		}
		else
		{
			literal += c;
		}

		std::cout << "Literal|" << literal << "|\n";
		for (auto ptr : ptrs)
		{
			auto& node = *ptr;
			std::cout << "  " << node << "\n";
		}
	}
	wkt.setRoot(ptrs.front());

	// =====
	std::cout << "VICTORY\n";
}

Shapefile::Shapefile(const std::filesystem::path& path)
{
	// ==== Read the main file ============================================= //
	SHPReader shp(path);

	if (shp.header.fileCode != 9994)
	{
		throw std::runtime_error("magic number 9994 required");
	}

	// Only work with shape type polygon
	for (const auto& record : shp.records)
	{
		if (record.shapeType != SHPReader::ShapeType::Polygon)
		{
			throw std::runtime_error("shape type must be polygon ("
				+ std::to_string(SHPReader::ShapeType::Polygon) + ")");
		}
	}

	// Byte buffer
	char buffer[4];

	// Parse polygons
	E2 vertex;
	for (const auto& record : shp.records)
	{
		std::string data(record.content.cbegin(), record.content.cend());
		std::istringstream iss(data);
		double xMin = read_double_small_endian(iss, buffer);
		double yMin = read_double_small_endian(iss, buffer);
		double xMax = read_double_small_endian(iss, buffer);
		double yMax = read_double_small_endian(iss, buffer);
		int numParts = (int)read_unsigned_small_endian(iss, buffer);
		int numPoints = (int)read_unsigned_small_endian(iss, buffer);

		// Append parts array by the total number of points
		std::vector<int> parts(numParts + 1);
		for (int i = 0; i < numParts; ++i)
		{
			parts[i] = (int)read_unsigned_small_endian(iss, buffer);
		}
		parts[numParts] = numPoints;

		CompoundPolygon cpoly;
		cpoly.polygons.reserve(numParts);
		for (int i = 0; i < numParts; ++i)
		{
			Polygon poly;
			// (p. 9, J-7855) The rings are closed.
			poly.vertices.reserve((parts[i+1] - parts[i]) - 1);
			for (int j = parts[i]; j < parts[i+1] - 1; ++j)
			{
				vertex.x = read_double_small_endian(iss, buffer);
				vertex.y = read_double_small_endian(iss, buffer);
				poly.vertices.push_back(vertex);
			}
			cpoly.polygons.push_back(poly);
		}
		regions.push_back(cpoly);
	}

	// ==== Detect and read the projection file ============================ //
	auto prjPath = path;
	prjPath.replace_extension("prj");
	if (std::filesystem::is_regular_file(prjPath))
	{
		PRJReader prj(prjPath);
	}
}


Shapefile::~Shapefile()
{

}
