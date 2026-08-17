# Shared support library for the GUI applications: raylib<->S2LL bridging
# (S2Raylib.hpp), the orbit camera, the smooth-surface shader wrapper, and a
# single build of Dear ImGui + rlImGui shared by every app instead of
# recompiling the same sources in each executable.
add_library(S2LL_AppSupport STATIC
	OrbitCamera.cpp
	SmoothSurfaceShader.cpp
	${rlimgui_SOURCE_DIR}/rlImGui.cpp
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

# MSVC needs /utf-8 so the UTF-8 in shared headers and the app itself are
# interpreted correctly.
if(MSVC)
	target_compile_options(S2LL_AppSupport PRIVATE /utf-8)
endif()

target_include_directories(S2LL_AppSupport PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${rlimgui_SOURCE_DIR}
	${imgui_SOURCE_DIR}
)

# Absolute path to the shared shader pair, usable from every app
target_compile_definitions(S2LL_AppSupport PUBLIC
	S2APP_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

target_link_libraries(S2LL_AppSupport PUBLIC
	S2LL
	raylib
)
