#include "S2DemoConfig.hpp"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <rlImGui.h>
#include <extras/IconsFontAwesome6.h>
#include <imgui.h>

#include "Demos.hpp"
#include "DemoSignalGuard.hpp"

#include <S2LL/Core/Surfaces.hpp>

#include <cmath>
#include <algorithm>
#include <format>

#undef CloseWindow
#undef ShowCursor
#undef Rectangle
#undef DrawText
#undef DrawTextA
#undef DrawTextW
#undef LoadImage
#undef LoadImageA
#undef LoadImageW
#undef PlaySound
#undef PlaySoundA
#undef PlaySoundW

using namespace S2LL;
using namespace S2LL::Literals;
using namespace ImGui;

namespace S2Demo
{
	inline Vector3 to_Vector3(const E3& e) noexcept
	{
		return Vector3{ static_cast<float>(e.x), static_cast<float>(e.y), static_cast<float>(e.z) };
	}

	template <typename... Args>
	inline void TextFormatted(std::format_string<Args...> fmt, Args&&... args)
	{
		TextUnformatted(std::format(fmt, std::forward<Args>(args)...).c_str());
	}

	void RunDemo1(std::ostream& ost, DemoContext& ctx)
	{
		DemoSignalGuard guard;

		// Sphere of radius 3
		const Ellipsoid s(3);
		// Vertices of a spherical 4-gon
		const E3 A(0, 0, 3), B(3, 0, 0), C(2, 2, 1), D(0, 3, 0);
		// Query point
		const E3 Q(0.776457, 2.59185, 1.29593);
		// Direction of view (normalized)
		E3 v(1, 1, 1);
		v.normalize();

		// Configure resizable Raylib window
		SetConfigFlags(FLAG_WINDOW_RESIZABLE);
		InitWindow(800, 640, "S2Demo - PiSP Paper");
		SetTargetFPS(60);

		// Initialize rlImGui
		rlImGuiSetup(true);

		// Load GLSL shader for smooth per-pixel shading from standalone files
		const std::string vs_path = std::string(S2DEMO_SHADER_DIR) + "/smooth_ellipsoid.vs";
		const std::string fs_path = std::string(S2DEMO_SHADER_DIR) + "/smooth_ellipsoid.fs";
		Shader smooth_shader = LoadShader(vs_path.c_str(), fs_path.c_str());
		int center_loc = GetShaderLocation(smooth_shader, "uCenter");
		int axes_loc = GetShaderLocation(smooth_shader, "uSemiAxes");
		int is_sphere_loc = GetShaderLocation(smooth_shader, "uIsSphere");
		int light_dir_loc = GetShaderLocation(smooth_shader, "uLightDir");
		int view_pos_loc = GetShaderLocation(smooth_shader, "uViewPos");
		int model_loc = GetShaderLocation(smooth_shader, "uModel");

		float center_val[3] = { 0.0f, 0.0f, 0.0f };
		float axes_val[3] = {
			static_cast<float>(s.major()),
			static_cast<float>(s.median()),
			static_cast<float>(s.minor())
		};
		int is_sphere_val = s.is_sphere() ? 1 : 0;
		// Light coming from +z direction in world space
		float light_dir_val[3] = { 0.0f, 0.0f, 1.0f };

		// Model matrix aligning sphere poles with Z-axis in World Space
		Matrix model_matrix = MatrixRotateX(90.0f * DEG2RAD);

		SetShaderValue(smooth_shader, center_loc, center_val, SHADER_UNIFORM_VEC3);
		SetShaderValue(smooth_shader, axes_loc, axes_val, SHADER_UNIFORM_VEC3);
		SetShaderValue(smooth_shader, is_sphere_loc, &is_sphere_val, SHADER_UNIFORM_INT);
		SetShaderValue(smooth_shader, light_dir_loc, light_dir_val, SHADER_UNIFORM_VEC3);
		SetShaderValueMatrix(smooth_shader, model_loc, model_matrix);

		// Setup 3D Camera positioned from direction v looking at origin
		double dist2cam = 12.0;
		Camera3D cam   = { 0 };
		cam.position   = to_Vector3(dist2cam * v);
		cam.target     = Vector3{ 0.0f, 0.0f, 0.0f };
		cam.up         = Vector3{ 0.0f, 0.0f, 1.0f };
		cam.fovy       = 45.0f;
		cam.projection = CAMERA_PERSPECTIVE;

		bool view_needs_update = true;
		bool show_xy_plane = false;
		bool show_axes = true;
		int sphere_model = 1;
		bool show_chord = false;
		int show_case = 0;
		bool rotate_tool = true;
		bool is_dragging_rotation = false;
		double cam_yaw = std::atan2(v.y, v.x);
		double cam_pitch = std::asin(v.z);
		bool exit_requested = false;
		double vertex_size = 0.05f;

		ost << "3D Window Launched (Arbitrarily Resizable).\n";
		ost << "Press ESC, close window, or Ctrl+C to return to CLI.\n";

		Vector3 origin{ 0.0f, 0.0f, 0.0f };

		while (!WindowShouldClose() && !DemoSignalGuard::isInterrupted() && !exit_requested)
		{
			float radius = static_cast<float>(s.major());
			Vector2 mouse_pos = GetMousePosition();
			Ray ray = GetScreenToWorldRay(mouse_pos, cam);
			RayCollision collision = GetRayCollisionSphere(ray, origin, radius);
			bool outside_sphere = !collision.hit;

			if (rotate_tool && !GetIO().WantCaptureMouse)
			{
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					if (outside_sphere || sphere_model == 0)
					{
						is_dragging_rotation = true;
					}
				}
			}

			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
			{
				is_dragging_rotation = false;
			}

			if (is_dragging_rotation && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			{
				Vector2 delta = GetMouseDelta();
				cam_yaw   -= static_cast<double>(delta.x) * 0.005;
				cam_pitch += static_cast<double>(delta.y) * 0.005;
				cam_pitch  = std::clamp(cam_pitch, -90_deg, 90_deg);
				view_needs_update = true;
			}

			if (view_needs_update)
			{
				double cp = std::cos(cam_pitch);
				double sp = std::sin(cam_pitch);
				double cy = std::cos(cam_yaw);
				double sy = std::sin(cam_yaw);

				E3 cam_dir{ cp * cy, cp * sy, sp };
				cam.position = to_Vector3(dist2cam * cam_dir);
				view_needs_update = false;
			}

			UpdateCamera(&cam, CAMERA_CUSTOM);

			BeginDrawing();
			ClearBackground(DARKGRAY);

			BeginMode3D(cam);

			// Reference Equatorial Grid (XY-plane, Z=0)
			if (show_xy_plane)
			{
				rlPushMatrix();
				rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
				DrawGrid(20, 1.0f);
				rlPopMatrix();
			}

			// Draw E3 Coordinate Axes (X=Red, Y=Green, Z=Blue)
			if (show_axes)
			{
				DrawLine3D(origin, Vector3{ 3.6f, 0, 0 }, RED);
				DrawLine3D(origin, Vector3{ 0, 3.6f, 0 }, GREEN);
				DrawLine3D(origin, Vector3{ 0, 0, 3.6f }, BLUE);
			}

			// Render sphere s
			switch (sphere_model)
			{
			case 1:
			{
				float view_pos_val[3] = { cam.position.x, cam.position.y, cam.position.z };
				SetShaderValue(smooth_shader, view_pos_loc, view_pos_val, SHADER_UNIFORM_VEC3);
				BeginShaderMode(smooth_shader);
				DrawSphereEx(origin, radius, 64, 64, LIGHTGRAY);
				EndShaderMode();
				break;
			}
			case 2:
				rlPushMatrix();
				rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
				DrawSphereWires(origin, radius, 24, 24, RAYWHITE);
				rlPopMatrix();
				break;
			}

			// Convert E3 vertices to Raylib Vector3
			Vector3 pA = to_Vector3(A);
			Vector3 pB = to_Vector3(B);
			Vector3 pC = to_Vector3(C);
			Vector3 pD = to_Vector3(D);
			Vector3 pQ = to_Vector3(Q);

			// Draw 4-gon vertices
			DrawSphere(pA, vertex_size, BLACK);
			DrawSphere(pB, vertex_size, BLACK);
			DrawSphere(pC, vertex_size, BLACK);
			DrawSphere(pD, vertex_size, BLACK);
			DrawSphere(pQ, vertex_size, BLUE);

			// Draw 4-gon edges: A -> B -> C -> D -> A
			if (show_chord)
			{
				DrawLine3D(pA, pB, GOLD);
				DrawLine3D(pB, pC, GOLD);
				DrawLine3D(pC, pD, GOLD);
				DrawLine3D(pD, pA, GOLD);
			}

			// Draw direction vector v ray
			// Vector3 dir_v = to_Vector3(v * (radius + 2.0f));
			// DrawLine3D(origin, dir_v, ORANGE);

			EndMode3D();

			// Draw x, y, z axis labels at tips when axes are enabled
			if (show_axes)
			{
				Vector2 pX = GetWorldToScreen(Vector3{ 4.0f, 0.0f, 0.0f }, cam);
				Vector2 pY = GetWorldToScreen(Vector3{ 0.0f, 4.0f, 0.0f }, cam);
				Vector2 pZ = GetWorldToScreen(Vector3{ 0.0f, 0.0f, 4.0f }, cam);

				DrawText("x", static_cast<int>(pX.x), static_cast<int>(pX.y), 20, RED);
				DrawText("y", static_cast<int>(pY.x), static_cast<int>(pY.y), 20, GREEN);
				DrawText("z", static_cast<int>(pZ.x), static_cast<int>(pZ.y), 20, BLUE);
			}

			// Setup ImGui overlay & main menu bar
			rlImGuiBegin();

			if (BeginMainMenuBar())
			{
				if (BeginMenu("File"))
				{
					if (MenuItem("Exit Demo", "ESC"))
					{
						exit_requested = true;
					}
					EndMenu();
				}
				if (BeginMenu("View"))
				{
					if (MenuItem("Reset View"))
					{
						cam_pitch = std::asin(v.z);
						cam_yaw = std::atan2(v.y, v.x);
						cam.position = to_Vector3(dist2cam * v);
						cam.target = origin;
						cam.up = Vector3{ 0.0f, 0.0f, 1.0f };
						view_needs_update = true;
					}
					Separator();
					MenuItem("Rotate View", nullptr, &rotate_tool);
					MenuItem("Coordinate Axes", nullptr, &show_axes);
					EndMenu();
				}
				if (BeginMenu("Display"))
				{
					if (BeginMenu("Sphere"))
					{
						RadioButton("None", &sphere_model, 0);
						RadioButton("Solid", &sphere_model, 1);
						RadioButton("Wireframe", &sphere_model, 2);
						EndMenu();
					}
					if (BeginMenu("Edges"))
					{
						MenuItem("Show Chord", nullptr, &show_chord);
						EndMenu();
					}
					if (BeginMenu("Planes"))
					{
						MenuItem("xy-plane", nullptr, &show_xy_plane);
						EndMenu();
					}
					EndMenu();
				}
				if (BeginMenu("Demos"))
				{
					RadioButton("1a: Spherical 4-gon (Fig. 1a-c)", &show_case, 0);
					RadioButton("1b: Spherical 4-gon (Fig. 1d-f)", &show_case, 1);
					EndMenu();
				}
				EndMainMenuBar();
			}

			// Top Toolbar Panel (Rotate Icon Tool)
			SetNextWindowPos(ImVec2(0, GetFrameHeight()));
			SetNextWindowSize(ImVec2(GetIO().DisplaySize.x, 38.0f));
			Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
			{
				bool pushed_color;

				if (Button(ICON_FA_ROTATE " Reset"))
				{
					sphere_model = 1;
					show_chord = false;
					cam_pitch = std::asin(v.z);
					cam_yaw = std::atan2(v.y, v.x);
					view_needs_update = true;
				}

				SameLine();
				pushed_color = rotate_tool;
				if (pushed_color)
				{
					PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
				}
				if (Button(ICON_FA_CAMERA_ROTATE " Rotate View"))
				{
					rotate_tool = !rotate_tool;
				}
				if (pushed_color)
				{
					PopStyleColor();
				}

				SameLine();
				if (Button(ICON_FA_LOCATION_CROSSHAIRS " View from Q"))
				{
					auto coords = s.to_S2(Q).to_LL();
					cam_pitch = coords.lat;
					cam_yaw = coords.lon;
					view_needs_update = true;
				}

				SameLine();
				if (Button(ICON_FA_DRAW_POLYGON " Chords Only"))
				{
					sphere_model = 0;
					show_chord = true;
				}
			}
			End();

			SetNextWindowPos(ImVec2(10.0f, GetFrameHeight() + 48.0f), ImGuiCond_FirstUseEver);
			Begin("S2Demo Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			TextFormatted("Sphere Radius: {}", s.major());
			Separator();
			TextUnformatted("Vertices of G = [A,B,C,D]:");
			TextFormatted("  A: ({}, {}, {})", A.x, A.y, A.z);
			TextFormatted("  B: ({}, {}, {})", B.x, B.y, B.z);
			TextFormatted("  C: ({}, {}, {})", C.x, C.y, C.z);
			TextFormatted("  D: ({}, {}, {})", D.x, D.y, D.z);
			Separator();
			E3 v_curr{
				static_cast<double>(cam.position.x) / dist2cam,
				static_cast<double>(cam.position.y) / dist2cam,
				static_cast<double>(cam.position.z) / dist2cam
			};
			TextUnformatted("Query point:");
			TextFormatted("  Q: ({:.3f}, {:.3f}, {:.3f})", Q.x, Q.y, Q.z);
			Separator();
			TextUnformatted("View Direction:");
			TextFormatted("  v: ({:.3f}, {:.3f}, {:.3f})", v_curr.x, v_curr.y, v_curr.z);
			End();

			rlImGuiEnd();

			EndDrawing();
		}

		rlImGuiShutdown();
		UnloadShader(smooth_shader);
		CloseWindow();

		ost << "Demo 1 exited. Returned to CLI.\n";
	}
}
