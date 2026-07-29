cmake_minimum_required(VERSION 3.11.4)
project(S2LikeLib VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

find_package(argparse 3.0 CONFIG QUIET)
if(NOT argparse_FOUND)
	FetchContent_Declare(argparse
		GIT_REPOSITORY https://github.com/p-ranav/argparse.git
		GIT_TAG 3eda91b2e1ce7d569f84ba295507c4cd8fd96910 # v3.2
	)
	FetchContent_MakeAvailable(argparse)
endif()

find_package(cli CONFIG QUIET)
if(NOT cli_FOUND)
	FetchContent_Declare(cli
		GIT_REPOSITORY https://github.com/daniele77/cli.git
		GIT_TAG 80541c47bc4c7a6baa205136ca558860cb5e61af # v2.2.0
	)
	FetchContent_MakeAvailable(cli)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)

add_subdirectory(src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

add_subdirectory(app)

# Package Export Configuration
set(S2LIKELIB_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/S2LikeLib)

install(EXPORT S2LikeLibTargets
	FILE S2LikeLibTargets.cmake
	NAMESPACE S2LL::
	DESTINATION ${S2LIKELIB_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/S2LikeLibConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/S2LikeLibConfig.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/S2LikeLibConfig.cmake"
	INSTALL_DESTINATION ${S2LIKELIB_INSTALL_CMAKEDIR}
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/S2LikeLibConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/S2LikeLibConfigVersion.cmake"
	DESTINATION ${S2LIKELIB_INSTALL_CMAKEDIR}
)

option(BUILD_TESTING "Build the unit tests" ON)
if(BUILD_TESTING)
	enable_testing()
	add_subdirectory(test)
endif()

# Print all cmake variables for debugging purposes.
#
# get_1cmake_property(all_vars VARIABLES)
# list(SORT all_vars)
# foreach(var ${all_vars})
#   message(STATUS "${var}='${${var}}'")
# endforeach()
