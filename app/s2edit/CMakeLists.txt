add_executable(S2Edit main.cpp)

configure_file(S2EditConfig.hpp.in S2EditConfig.hpp)

target_include_directories(S2Edit PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(S2Edit PRIVATE
	S2LL
	argparse
	cli
)

install(TARGETS S2Edit
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
