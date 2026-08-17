cmake_minimum_required(VERSION 3.11.4)
unset(CMAKE_POLICY_VERSION_MINIMUM CACHE)
unset(CMAKE_POLICY_VERSION_MINIMUM)
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
	set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(cli
		GIT_REPOSITORY https://github.com/daniele77/cli.git
		GIT_TAG 80541c47bc4c7a6baa205136ca558860cb5e61af # v2.2.0
		SYSTEM
	)
	FetchContent_MakeAvailable(cli)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)

add_subdirectory(src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

# Shared GUI dependencies for the 3D demo applications (S2Demo, S2Hare).
# Fetched once at the top level so every app links against the same
# raylib / Dear ImGui / rlImGui build.
FetchContent_Declare(
	raylib
	GIT_REPOSITORY https://github.com/raysan5/raylib.git
	GIT_TAG 5.5
)

FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG docking
)

FetchContent_Declare(
	rlImGui
	GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
	GIT_TAG main
)

# Disable Raylib examples to keep Visual Studio Startup Items list clean
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)

# Disable audio module
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "Disable raylib audio module" FORCE)

FetchContent_MakeAvailable(raylib imgui rlImGui)

# Applications
add_subdirectory(app/common)
add_subdirectory(app/s2edit)
add_subdirectory(app/s2demo)
add_subdirectory(app/s2hare)

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

option(BUILD_DOCS "Build documentation using Doxygen" OFF)
if(BUILD_DOCS)
	find_package(Doxygen QUIET)
	if(DOXYGEN_FOUND)
		set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
		set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

		configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

		add_custom_target(doc
			COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating API documentation with Doxygen..."
			VERBATIM
		)
	else()
		message(STATUS "Doxygen not found. 'doc' target will not be created.")
	endif()
endif()

# Print all cmake variables for debugging purposes.
#
# get_1cmake_property(all_vars VARIABLES)
# list(SORT all_vars)
# foreach(var ${all_vars})
#   message(STATUS "${var}='${${var}}'")
# endforeach()
