include(FetchContent)

# Fetch Raylib
FetchContent_Declare(
	raylib
	GIT_REPOSITORY https://github.com/raysan5/raylib.git
	GIT_TAG 5.5
)

# Fetch Dear ImGui
FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG docking
)

# Fetch rlImGui
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

add_executable(S2Demo
	main.cpp
	Commands.cpp
	Demo1.cpp
	${rlimgui_SOURCE_DIR}/rlImGui.cpp
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

# Demo source embeds UTF-8 literals (e.g. U+00B2 superscript two); MSVC needs /utf-8 to interpret
# them as UTF-8 instead of the active code page.
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
	${rlimgui_SOURCE_DIR}
	${imgui_SOURCE_DIR}
)

if(argparse_SOURCE_DIR)
	target_include_directories(S2Demo BEFORE PRIVATE ${argparse_SOURCE_DIR}/include)
endif()

target_link_libraries(S2Demo PRIVATE
	S2LL
	argparse::argparse
	cli
	raylib
)

install(TARGETS S2Demo
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
