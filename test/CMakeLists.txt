include(FetchContent)

# Fetch Catch2 v3
FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

# Create unit test executable
add_executable(S2LL_Tests
	Core/TestCoordinates.cpp
	Core/TestPolygons.cpp
	Core/TestSurfaces.cpp
	Parser/TestShapefile.cpp)

target_link_libraries(S2LL_Tests PRIVATE
	S2LL
	Catch2::Catch2WithMain
)

# Register with CTest / Visual Studio Test Explorer
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(S2LL_Tests)
