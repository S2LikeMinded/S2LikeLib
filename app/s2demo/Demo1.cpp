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

	inline std::vector<Vector3> generate_great_circle_arc(const E3& A, const E3& B, float R, int num_segments = 32)
	{
		std::vector<Vector3> arc_points;
		arc_points.reserve(num_segments + 1);

		E3 u = A.normalized();
		E3 v = B.normalized();

		double d = std::clamp(dot(u, v), -1.0, 1.0);
		double theta = std::acos(d);

		E3 w = (v - u * d).normalized();

		for (int i = 0; i <= num_segments; ++i)
		{
			double t = static_cast<double>(i) / num_segments;
			double angle = t * theta;
			E3 p = (std::cos(angle) * u + std::sin(angle) * w) * R;
			arc_points.push_back(to_Vector3(p));
		}

		return arc_points;
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
		const E3::Loop<4> G{ {0, 0, 3}, {3, 0, 0}, {2, 2, 1}, {0, 3, 0} };
		const auto nv = G.size();
		// Query points for Demos 1a & 1b: Q lies on sphere R=3 and on the same great circle
		// as G's vertices[1] (3,0,0) and vertices[2] (2,2,1), at angle theta = 5*pi/12 (75 deg).
		auto [sin_a, cos_a] = sin_and_cos(75_Deg);
		Double sqrt5 = sqrt(Lift(5));
		Double qx = 3 * cos_a;
		Double qy = 6 * sin_a / sqrt5;
		Double qz = 3 * sin_a / sqrt5;

		// x: +/-(3/ 4)(sqrt( 6)-sqrt( 2))
		// y:    (3/10)(sqrt(30)+sqrt(10))
		// z:    (3/20)(sqrt(30)+sqrt(10))
		const E3 Q_1a(static_cast<double>(qx), static_cast<double>(qy), static_cast<double>(qz));
		const E3 Q_1b(static_cast<double>(-qx), static_cast<double>(qy), static_cast<double>(qz));
		// Direction of view (viewing-from direction, normalized)
		const E3 v = E3{ 1, 1, 1 }.normalize();

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
		bool orthographic = true;
		cam.fovy       = orthographic ? 8.0f : 45.0f;
		cam.projection = orthographic ? CAMERA_ORTHOGRAPHIC : CAMERA_PERSPECTIVE;

		bool view_needs_update = true;
		bool show_xy_plane = false;
		bool show_axes = true;
		int sphere_model = 1;
		bool show_arc = true;
		bool show_chord = true;
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
			E3 Q = (show_case == 0) ? Q_1a : Q_1b;
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

			cam.projection = orthographic ? CAMERA_ORTHOGRAPHIC : CAMERA_PERSPECTIVE;
			cam.fovy       = orthographic ? 8.0f : 45.0f;
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
			case 1: { // Solid view
				float view_pos_val[3] = { cam.position.x, cam.position.y, cam.position.z };
				SetShaderValue(smooth_shader, view_pos_loc, view_pos_val, SHADER_UNIFORM_VEC3);
				BeginShaderMode(smooth_shader);
				DrawSphereEx(origin, radius, 64, 64, LIGHTGRAY);
				EndShaderMode();
			} break;
			case 2: { // Wireframe view (switch poles to z-axis)
				rlPushMatrix();
				rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
				DrawSphereWires(origin, radius, 24, 24, RAYWHITE);
				rlPopMatrix();
			} break;
			}

			// Convert E3 vertices to Raylib Vector3
			std::array<Vector3, nv> pG;
			for (size_t i = 0; i < nv; ++i)
			{
				pG[i] = to_Vector3(G.vertices[i]);
			}
			Vector3 pQ = to_Vector3(Q);

			// Ray-sphere collision detection for vertex hovering
			int hovered_idx = -1;
			float pick_radius = static_cast<float>(vertex_size * 2.0);
			if (!GetIO().WantCaptureMouse)
			{
				float closest_dist = 1e9f;
				for (size_t i = 0; i < nv; ++i)
				{
					RayCollision col = GetRayCollisionSphere(ray, pG[i], pick_radius);
					if (col.hit && col.distance < closest_dist)
					{
						closest_dist = col.distance;
						hovered_idx = static_cast<int>(i);
					}
				}
				RayCollision colQ = GetRayCollisionSphere(ray, pQ, pick_radius);
				if (colQ.hit && colQ.distance < closest_dist)
				{
					hovered_idx = static_cast<int>(nv);
				}
			}

			// Draw n-gon vertices
			for (size_t i = 0; i < nv; ++i)
			{
				bool is_hovered = (hovered_idx == static_cast<int>(i));
				Color fill_color = is_hovered ? YELLOW : BLACK;
				Color outline_color = is_hovered ? WHITE : LIGHTGRAY;
				float r = static_cast<float>(vertex_size);
				float outline_r = is_hovered ? r * 1.35f : r * 1.15f;

				DrawSphere(pG[i], r, fill_color);
			}

			// Draw Query Point Q
			{
				bool is_hovered = (hovered_idx == static_cast<int>(nv));
				Color fill_color = is_hovered ? SKYBLUE : BLUE;
				Color outline_color = is_hovered ? WHITE : LIGHTGRAY;
				float r = static_cast<float>(vertex_size);
				float outline_r = is_hovered ? r * 1.35f : r * 1.15f;

				DrawSphere(pQ, r, fill_color);
			}

			// Draw Great-Circle Arcs on sphere surface
			if (show_arc)
			{
				for (size_t i = 0; i < nv; ++i)
				{
					double radius_lifted = radius * 1.002f;
					auto arc_pts = generate_great_circle_arc(G.vertices[i], G.vertices[(i + 1) % nv], radius_lifted, 32);
					for (size_t k = 0; k + 1 < arc_pts.size(); ++k)
					{
						Vector3 p0 = arc_pts[k];
						Vector3 p1 = arc_pts[k + 1];

						DrawLine3D(p0, p1, BLACK);
					}
				}
			}

			// Draw n-gon edges: V_0 -> V_1 -> ... -> V_{nv-1} -> V_0
			if (show_chord)
			{
				for (size_t i = 0; i < nv; ++i)
				{
					DrawLine3D(pG[i], pG[(i + 1) % nv], GOLD);
				}
			}

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
					MenuItem("Orthographic View", nullptr, &orthographic);
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
						MenuItem("Great-Circle Arcs", nullptr, &show_arc);
						MenuItem("Straight Chords", nullptr, &show_chord);
						EndMenu();
					}
					if (BeginMenu("Coordinate System"))
					{
						MenuItem("Axes", nullptr, &show_axes);
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

			// Top Toolbar Panel (Rotate Icon Tool & Ortho View)
			SetNextWindowPos(ImVec2(0, GetFrameHeight()));
			SetNextWindowSize(ImVec2(GetIO().DisplaySize.x, 38.0f));
			Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
			{
				bool pushed_color;

				if (Button(ICON_FA_ROTATE " Reset"))
				{
					sphere_model = 1;
					show_arc = true;
					show_chord = true;
					orthographic = true;
					cam_pitch = std::asin(v.z);
					cam_yaw = std::atan2(v.y, v.x);
					view_needs_update = true;
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
				pushed_color = rotate_tool;
				if (pushed_color)
				{
					PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
				}
				if (Button((std::string(ICON_FA_CAMERA_ROTATE " Rotate View ") +
					(rotate_tool ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF)).c_str()))
				{
					rotate_tool = !rotate_tool;
				}
				if (pushed_color)
				{
					PopStyleColor();
				}

				SameLine();
				pushed_color = show_arc;
				if (pushed_color)
				{
					PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
				}
				if (Button((std::string(ICON_FA_BEZIER_CURVE " Arcs ") +
					(show_arc ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF)).c_str()))
				{
					show_arc = !show_arc;
				}
				if (pushed_color)
				{
					PopStyleColor();
				}

				SameLine();
				pushed_color = show_chord;
				if (pushed_color)
				{
					PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
				}
				if (Button((std::string(ICON_FA_DRAW_POLYGON " Chords ") +
					(show_chord ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF)).c_str()))
				{
					show_chord = !show_chord;
				}
				if (pushed_color)
				{
					PopStyleColor();
				}

				SameLine();
				pushed_color = orthographic;
				if (pushed_color)
				{
					PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
				}
				if (Button((std::string(ICON_FA_CUBE " Ortho View ") +
					(orthographic ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF)).c_str()))
				{
					orthographic = !orthographic;
				}
				if (pushed_color)
				{
					PopStyleColor();
				}

				SameLine();
				const char* model_labels[] =
				{
					ICON_FA_GLOBE " None " ICON_FA_GLOBE,
					ICON_FA_GLOBE " Solid " ICON_FA_GLOBE,
					ICON_FA_GLOBE " Wireframe " ICON_FA_GLOBE
				};
				PushItemWidth(120.0f);
				SliderInt("##SphereModel", &sphere_model, 0, 2, model_labels[sphere_model]);
				PopItemWidth();
			}
			End();

			SetNextWindowPos(ImVec2(10.0f, GetFrameHeight() + 48.0f), ImGuiCond_FirstUseEver);
			std::string controls_title = std::format("S2Demo - Demo 1{}###Controls", show_case == 0 ? "a" : "b");
			Begin(controls_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			TextUnformatted("Ellipsoid");
			TextFormatted("  Spherical radius: {}", s.major());
			Separator();
			TextFormatted("Spherical {}-gon G", nv);
			for (size_t i = 0; i < nv; ++i)
			{
				const char* prefix = (hovered_idx == static_cast<int>(i)) ? ICON_FA_CHEVRON_RIGHT : " ";
				TextFormatted("{} {}: ({}, {}, {})", prefix, "ABCD"[i],
					G.vertices[i].x, G.vertices[i].y, G.vertices[i].z);
			}
			Separator();
			E3 v_curr{
				static_cast<double>(cam.position.x) / dist2cam,
				static_cast<double>(cam.position.y) / dist2cam,
				static_cast<double>(cam.position.z) / dist2cam
			};
			TextUnformatted("Query point");
			{
				const char* prefix = (hovered_idx == static_cast<int>(nv)) ? ICON_FA_CHEVRON_RIGHT : " ";
				TextFormatted("{} Q: ({: .3f}, {: .3f}, {: .3f})", prefix, Q.x, Q.y, Q.z);
			}
			Separator();
			TextUnformatted("Viewing From Direction");
			TextFormatted("  v: ({: .3f}, {: .3f}, {: .3f})", v_curr.x, v_curr.y, v_curr.z);
			End();

			// Display hover tooltip inside active ImGui frame scope
			if (hovered_idx >= 0 && static_cast<size_t>(hovered_idx) < nv)
			{
				BeginTooltip();
				TextFormatted("{}({}, {}, {})", "ABCD"[hovered_idx], G.vertices[hovered_idx].x, G.vertices[hovered_idx].y, G.vertices[hovered_idx].z);
				EndTooltip();
			}
			else if (hovered_idx == static_cast<int>(nv))
			{
				BeginTooltip();
				TextFormatted("Q({}, {}, {})", Q.x, Q.y, Q.z);
				EndTooltip();
			}

			rlImGuiEnd();

			EndDrawing();
		}

		rlImGuiShutdown();
		UnloadShader(smooth_shader);
		CloseWindow();

		ost << "Demo 1 exited. Returned to CLI.\n";
	}
}
