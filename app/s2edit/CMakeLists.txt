
add_executable(S2Edit main.cpp)
add_dependencies(S2Edit S2LL)

configure_file(S2EditConfig.hpp.in S2EditConfig.hpp)

get_cmake_property(all_vars VARIABLES)

target_include_directories(S2Edit PUBLIC ${CMAKE_HOME_DIRECTORY}/include)
target_include_directories(S2Edit PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
	target_link_libraries(S2Edit
		PUBLIC ${CMAKE_HOME_DIRECTORY}/lib/libS2LL.a
		argparse
		cli
	)
else()
	target_link_libraries(S2Edit
		PUBLIC ${CMAKE_HOME_DIRECTORY}/lib/${CMAKE_BUILD_TYPE_INIT}/S2LL.lib
		argparse
		cli
	)
endif()
