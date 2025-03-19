
#ifndef __SHAPEFILE_HPP__
#define __SHAPEFILE_HPP__

#include <S2LL/Core/Regions.hpp>

#include <filesystem>
#include <memory>

namespace S2LM
{

	namespace Parser
	{
		// Auxiliary struct for parsing .SHP files
		struct SHPReader
		{
			// Shape type ID (Z: Z type, M: measured type)
			enum ShapeType
			{

				Null = 0,

				Point  =  1, PolyLine  =  3, Polygon  =  5, MultiPoint  =  8,

				PointZ = 11, PolyLineZ = 13, PolygonZ = 15, MultiPointZ = 18,

				PointM = 21, PolyLineM = 23, PolygonM = 25, MultiPointM = 28,

				MultiPatch = 31
			};

			// SHP file header
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

			// SHP file record header and content
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

		public:

			SHPReader() {};


			SHPReader(const std::filesystem::path& path);

		public:

			void parse(const std::filesystem::path& path);


		public:

			Header header;


			std::vector<Record> records;
		};

		// Auxiliary struct for parsing .PRJ files
		struct PRJReader
		{
		public:

			struct WKTNode
			{
			public:

				typedef std::shared_ptr<WKTNode> Ptr;

			public:

				inline const std::string& key() const { return nameOrValue; }

				// Remove double quotes from name string
				inline std::string name() const
				{
					const auto &enquotedName = values[0]->key();
					return enquotedName.substr(1, enquotedName.size() - 2);
				}

				// Convert value string to double
				inline int valueAsInt() const { return std::stoi(nameOrValue); }

				// Convert value string to double
				inline double valueAsDouble() const { return std::stod(nameOrValue); }

				// Find child node with the same name
				const WKTNode& operator[](std::string const& name) const
				{
					for (const auto ptr : values)
					{
						const auto& node = *ptr;
						if (!values.empty() && node.nameOrValue == name)
						{
							return node;
						}
					}
					throw std::runtime_error("child " + name + " not found");
				}

				// Find child node using the index
				const WKTNode& operator[](size_t index) const
				{
					return *values[index];
				}

				//
				friend std::ostream& operator<<(std::ostream &ost, const WKTNode& node)
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

			public:
				// Is complete?
				bool complete;

				// Keyword name, or value string.
				std::string nameOrValue;

				// Children
				std::vector<Ptr> values;
			};

		public:

			PRJReader() {};


			PRJReader(const std::filesystem::path& path);


		public:

			void parse(const std::filesystem::path& path);

		public:

			WKTNode wkt;


			std::string system;


			E3 ellipsoid;


			double prime;


			double unit;
		};

		// Main parser for Shapefile formats (SHP, PRJ)
		struct Shapefile
		{
		public:

			Shapefile() {};


			Shapefile(const std::filesystem::path& path);

		public:

			void parse(const std::filesystem::path& path);

		public:

			SHPReader shp;


			PRJReader prj;


			std::vector<S2LM::Compound<S2LM::Polygon>> regions;
		};
	}
}

#endif // !__SHAPEFILE_HPP__
