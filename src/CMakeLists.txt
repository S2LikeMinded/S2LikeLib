cmake_minimum_required(VERSION 3.10)
project(S2LL VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(S2LL Point.cpp)
target_include_directories(S2LL PUBLIC ../include)
target_include_directories(S2LL PUBLIC ${PROJECT_BINARY_DIR})
