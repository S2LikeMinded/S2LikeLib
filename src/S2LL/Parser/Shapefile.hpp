
#ifndef __SHAPEFILE_HPP__
#define __SHAPEFILE_HPP__

#include <S2LL/Core/Regions.hpp>

#include <filesystem>
#include <memory>

namespace S2LM
{

	namespace Parser
	{

		struct Shapefile
		{
			// Auxiliary struct for parsing .SHP files
			struct SHPReader
			{
				enum ShapeType
				{

					Null = 0,

					Point  =  1, PolyLine  =  3, Polygon  =  5, MultiPoint  =  8,

					PointZ = 11, PolyLineZ = 13, PolygonZ = 15, MultiPointZ = 18,

					PointM = 21, PolyLineM = 23, PolygonM = 25, MultiPointM = 28,

					MultiPatch = 31
				};

				struct Header
				{
					// File code (byte 0, value 9994 (?), big-endian)
					int fileCode;

					// File length (byte 24, big-endian)
					int fileLength;

					// Version (byte 28, value 1000, little-endian)
					int version;

					// Shape type (byte 32, little-endian)
					ShapeType shapeType;

					// Bounding box (byte 36, little-endian)
					double xMin;

					// Bounding box (byte 44, little-endian)
					double yMin;

					// Bounding box (byte 52, little-endian)
					double xMax;

					// Bounding box (byte 60, little-endian)
					double yMax;

					// Z type bounding box (byte 68, little-endian)
					double zMin;

					// Z type bounding box (byte 76, little-endian)
					double zMax;

					// Measured type bounding box (byte 84, little-endian)
					double mMin;

					// Measured type bounding box (byte 92, little-endian)
					double mMax;
				};

				struct Record
				{
					// Record number (header byte 0, big-endian)
					int number;

					// Content length (header byte 4, big-endian)
					int contentLength;

					// Shape type (content byte 0, little-endian)
					ShapeType shapeType;

					// Remaining content (content byte 4)
					std::vector<char> content;
				};

				SHPReader(const std::filesystem::path& path);

				Header header;

				std::vector<Record> records;
			};

			// Auxiliary struct for parsing .CPG files
			struct CPGReader
			{

			};

			// Auxiliary struct for parsing .DBF files
			struct DBFReader
			{

			};

			// Auxiliary struct for parsing .PRJ files
			struct PRJReader
			{
				// A tree data structure for the well-known text
				class WKTree
				{
				public:

					struct Node
					{
						// Is complete?
						bool complete;

						// Keyword name, or value string.
						std::string nameOrValue;

						// Children
						std::vector<std::shared_ptr<Node>> values;

						//
						friend std::ostream& operator<<(std::ostream &ost, const Node& node)
						{
							ost << node.nameOrValue;
							if (node.complete && node.values.empty())
							{
								return ost;
							}
							ost << "[";
							for (size_t i = 0; i < node.values.size(); ++i)
							{
								auto& child = *(node.values[i]);
								if (i > 0)
								{
									ost << ",";
								}
								ost << child;
							}
							if (node.complete)
							{
								ost << "]";
							}
							else
							{
								ost << "(" << node.nameOrValue << " ...)";
							}
							return ost;
						}
					};

					typedef std::shared_ptr<Node> NodePtr;

				public:


					WKTree() {}


					~WKTree() {};


				public:

					inline void setRoot(NodePtr &ptr)
					{
						rootPtr = ptr;
					}

				private:

					NodePtr rootPtr;
				};


				PRJReader(const std::filesystem::path& path);


				WKTree wkt;
			};

			// Auxiliary struct for parsing .SHX files
			struct SHXReader
			{

			};

			// Construct shapefile from a .shp file
			Shapefile(const std::filesystem::path& path);


			~Shapefile();


			std::vector<S2LM::CompoundPolygon> regions;
		};
	}
}

#endif // !__SHAPEFILE_HPP__
