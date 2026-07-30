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
