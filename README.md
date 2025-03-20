# S2LikeLib
A lightweight, efficient, and robust suite of C++17 tools and library for spheres and sphere-like surfaces.

## Dependencies

 - `p-ranav/argparse` [[GitHub]](https://github.com/p-ranav/argparse) [[vcpkg]](https://vcpkg.io/en/package/argparse) (thru [CMake](https://cmake.org/cmake/help/latest/module/FetchContent.html#overview))
 - `daniele77/cli` [[GitHub]](https://github.com/daniele77/cli) [[vcpkg]](https://vcpkg.io/en/package/cli) (thru CMake)

## Build instructions

### Windows Command Prompt & MacOS Shell

```batch
git clone https://github.com/S2LikeMinded/S2LikeLib.git
cd S2LikeLib && mkdir build
cmake -B build
cmake --build build
```

## Modules

 - Shapefile parser, containing:
   - [shp](https://en.wikipedia.org/wiki/Shapefile#Shapefile_shape_format_(.shp)) by `S2LM::Parser::SHPReader` for Polygon (shape type 5) only
   - [prj](https://docs.ogc.org/is/18-010r11/18-010r11.pdf) by `S2LM::Parser::PRJReader` as a [tree](https://en.wikipedia.org/wiki/Tree_(abstract_data_type))
