cmake_minimum_required(VERSION 3.10)
project(S2Edit VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(S2EditConfig.hpp.in S2EditConfig.hpp)

add_executable(S2Edit main.cpp)
target_include_directories(S2Edit PUBLIC ../../include)
target_include_directories(S2Edit PUBLIC ${PROJECT_BINARY_DIR})
target_link_libraries(S2Edit ../../../lib/Debug/S2LL)
