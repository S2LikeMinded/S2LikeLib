cmake_minimum_required(VERSION 3.11.4)
project(S2LikeLib VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
include(FetchContent)

FetchContent_Declare(argparse
	GIT_REPOSITORY https://github.com/p-ranav/argparse.git
	GIT_TAG 3eda91b2e1ce7d569f84ba295507c4cd8fd96910 # v3.2
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(cli
	GIT_REPOSITORY https://github.com/daniele77/cli.git
	GIT_TAG 80541c47bc4c7a6baa205136ca558860cb5e61af # v2.2.0
)
FetchContent_MakeAvailable(cli)

#file(MAKE_DIRECTORY lib)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)

add_subdirectory(src)

#file(MAKE_DIRECTORY bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

add_subdirectory(app)

#get_cmake_property(all_vars VARIABLES)
#list(SORT all_vars)
#foreach(var ${all_vars})
#	message(STATUS "${var}='${${var}}'")
#endforeach()
