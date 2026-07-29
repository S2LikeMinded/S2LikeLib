add_executable(S2Edit
	main.cpp
	Commands.cpp
)

configure_file(S2EditConfig.hpp.in S2EditConfig.hpp)

target_include_directories(S2Edit PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(argparse_SOURCE_DIR)
	target_include_directories(S2Edit BEFORE PRIVATE ${argparse_SOURCE_DIR}/include)
endif()

target_link_libraries(S2Edit PRIVATE
	S2LL
	argparse::argparse
	cli
)

install(TARGETS S2Edit
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
