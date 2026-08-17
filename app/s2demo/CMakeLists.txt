add_executable(S2Demo
	main.cpp
	Commands.cpp
	Demo1.cpp
)

# MSVC needs /utf-8 so the UTF-8 in shared headers and the app itself are
# interpreted correctly.
if(MSVC)
	target_compile_options(S2Demo PRIVATE /utf-8)
endif()

# ProggyClean.ttf ships with the fetched Dear ImGui dependency (original by
# Tristan Grimmer, 2004/2005, MIT, https://github.com/bluescan/proggyfonts).
# Copy it into this target's build directory; S2DEMO_RESOURCE_DIR points there.
file(COPY "${imgui_SOURCE_DIR}/misc/fonts/ProggyClean.ttf"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/resources")

configure_file(S2DemoConfig.hpp.in S2DemoConfig.hpp)

target_include_directories(S2Demo PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
)

if(argparse_SOURCE_DIR)
	target_include_directories(S2Demo BEFORE PRIVATE ${argparse_SOURCE_DIR}/include)
endif()

target_link_libraries(S2Demo PRIVATE
	S2LL
	S2LL_AppSupport
	argparse::argparse
	cli
)

install(TARGETS S2Demo
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
