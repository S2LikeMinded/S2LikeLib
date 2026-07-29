# S<sup>2</sup>LikeLib

*Under active development.*
A lightweight, efficient, and robust suite of C++20 tools and library for spheres and sphere-like surfaces.


## Dependencies

 - `p-ranav/argparse` [[GitHub]](https://github.com/p-ranav/argparse) [[vcpkg]](https://vcpkg.io/en/package/argparse) (through CMake)
 - `daniele77/cli` [[GitHub]](https://github.com/daniele77/cli) [[vcpkg]](https://vcpkg.io/en/package/cli) (through CMake)
 - `catchorg/Catch2` [[GitHub]](https://github.com/catchorg/Catch2) [[vcpkg]](https://vcpkg.io/en/package/catch2) (through CMake)


## Build & Installation Instructions

### Standard Build

```bash
git clone https://github.com/S2LikeMinded/S2LikeLib.git
cd S2LikeLib
cmake -B build
cmake --build build
```

### Using CMake Presets

```bash
# Configure and build Debug configuration
cmake --preset debug
cmake --build --preset debug

# Configure and build Release configuration
cmake --preset release
cmake --build --preset release
```

### Installation

To install `S2LL` library, headers, and apps (`S2Edit`, etc.):

```bash
# Local directory (No Admin rights needed):
cmake --install build --prefix ./install

# System-wide to Program Files or /usr/local (Requires Admin / sudo):
cmake --install build
```


## Modules

 - Shapefile parser, containing:
   - [shp](https://en.wikipedia.org/wiki/Shapefile#Shapefile_shape_format_(.shp)) by `S2LL::Parser::SHPReader` for Polygon (shape type 5) only
   - [prj](https://docs.ogc.org/is/18-010r11/18-010r11.pdf) by `S2LL::Parser::PRJReader` as a [tree](https://en.wikipedia.org/wiki/Tree_(abstract_data_type))

# Apps

Applications are implemented in individual folders under `./app` and compiled into `./bin`.

## S<sup>2</sup>Edit

An interactive CLI tool for inspecting, processing, and editing ellipsoidal and spherical geographic regions.
