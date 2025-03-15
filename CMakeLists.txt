cmake_minimum_required(VERSION 3.10)
project(S2LikeLib VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#file(MAKE_DIRECTORY lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)

add_subdirectory(src)

#file(MAKE_DIRECTORY bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

add_subdirectory(app)

# get_cmake_property(all_vars VARIABLES)
# list(SORT all_vars)
# foreach(var ${all_vars})
#    message(STATUS "${var}='${${var}}'")
# endforeach()
