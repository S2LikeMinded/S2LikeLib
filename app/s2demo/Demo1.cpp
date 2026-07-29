#include <raylib.h>
#include <rlgl.h>
#include <rlImGui.h>
#include <imgui.h>

#include "Demos.hpp"
#include "DemoSignalGuard.hpp"

#include <S2LL/Core/Surfaces.hpp>

#include <cmath>
#include <algorithm>

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

namespace S2Demo
{
	inline Vector3 to_Vector3(const E3& e) noexcept
	{
		return Vector3{ static_cast<float>(e.x), static_cast<float>(e.y), static_cast<float>(e.z) };
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
		InitWindow(1024, 1024, "S2Demo - PiSP Paper");
		SetTargetFPS(60);

		// Initialize rlImGui
		rlImGuiSetup(true);

		// Setup 3D Camera positioned from direction v looking at origin
		double dist2cam = 12.0;
		Camera3D cam   = { 0 };
		cam.position   = to_Vector3(dist2cam * v);
		cam.target     = Vector3{ 0.0f, 0.0f, 0.0f };
		cam.up         = Vector3{ 0.0f, 0.0f, 1.0f };
		cam.fovy       = 45.0f;
		cam.projection = CAMERA_PERSPECTIVE;

		bool show_grid = false;
		bool show_axes = true;
		int sphere_model = 0;
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

			if (rotate_tool && !ImGui::GetIO().WantCaptureMouse)
			{
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					if (outside_sphere)
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
				cam_pitch  = std::clamp(cam_pitch, -1.5, 1.5);

				double cp = std::cos(cam_pitch);
				double sp = std::sin(cam_pitch);
				double cy = std::cos(cam_yaw);
				double sy = std::sin(cam_yaw);

				E3 cam_dir{ cp * cy, cp * sy, sp };
				cam.position = to_Vector3(dist2cam * cam_dir);
			}

			UpdateCamera(&cam, CAMERA_CUSTOM);

			BeginDrawing();
			ClearBackground(DARKGRAY);

			BeginMode3D(cam);

			// Reference Equatorial Grid (XY-plane, Z=0)
			if (show_grid)
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
			case 0:
				DrawSphereEx(origin, radius, 64, 64, RAYWHITE);
				break;
			case 1:
				DrawSphereWires(origin, radius, 24, 24, RAYWHITE);
				break;
			default:
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
			DrawLine3D(pA, pB, GOLD);
			DrawLine3D(pB, pC, GOLD);
			DrawLine3D(pC, pD, GOLD);
			DrawLine3D(pD, pA, GOLD);

			// Draw direction vector v ray
			// Vector3 dir_v = to_Vector3(v * (radius + 2.0f));
			// DrawLine3D(Vector3{ 0, 0, 0 }, dir_v, ORANGE);

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

			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Exit Demo", "ESC"))
					{
						exit_requested = true;
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("View"))
				{
					if (ImGui::MenuItem("Reset View"))
					{
						cam_pitch = std::asin(v.z);
						cam_yaw = std::atan2(v.y, v.x);
						cam.position = to_Vector3(dist2cam * v);
						cam.target = origin;
						cam.up = Vector3{ 0.0f, 0.0f, 1.0f };
					}
					ImGui::Separator();
					ImGui::MenuItem("Rotate View", nullptr, &rotate_tool);
					ImGui::MenuItem("Equatorial Grid (xy-plane)", nullptr, &show_grid);
					ImGui::MenuItem("Coordinate Axes", nullptr, &show_axes);
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Display"))
				{
					if (ImGui::BeginMenu("Sphere"))
					{
						ImGui::RadioButton("Solid", &sphere_model, 0);
						ImGui::RadioButton("Wireframe", &sphere_model, 1);
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Demos"))
				{
					ImGui::RadioButton("1: Spherical 4-gon (Fig. 1a-c)", &show_case, 0);
					ImGui::RadioButton("2: Spherical 4-gon (Fig. 1d-f)", &show_case, 1);
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			// Top Toolbar Panel (Rotate Icon Tool)
			ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
			ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 38.0f));
			ImGui::Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
			{
				bool pushed_color = rotate_tool;
				if (pushed_color)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
				}

				if (ImGui::Button((char*)u8"\u21BB Rotate View"))
				{
					rotate_tool = !rotate_tool;
				}

				if (pushed_color)
				{
					ImGui::PopStyleColor();
				}
			}
			ImGui::End();

			ImGui::Begin("S2Demo Controls");
			ImGui::Text("Sphere Radius: %.1f", s.major());
			ImGui::Separator();
			ImGui::Text("Vertices of spherical 4-gon:");
			ImGui::Text("  A: (%.1f, %.1f, %.1f)", A.x, A.y, A.z);
			ImGui::Text("  B: (%.1f, %.1f, %.1f)", B.x, B.y, B.z);
			ImGui::Text("  C: (%.1f, %.1f, %.1f)", C.x, C.y, C.z);
			ImGui::Text("  D: (%.1f, %.1f, %.1f)", D.x, D.y, D.z);
			ImGui::Separator();
			E3 v_curr{
				static_cast<double>(cam.position.x) / dist2cam,
				static_cast<double>(cam.position.y) / dist2cam,
				static_cast<double>(cam.position.z) / dist2cam
			};
			ImGui::Text("Query point:");
			ImGui::Text("  Q: (%.1f, %.1f, %.1f)", Q.x, Q.y, Q.z);
			ImGui::Separator();
			ImGui::Text("View Direction:");
			ImGui::Text("  v: (%.3f, %.3f, %.3f)", v_curr.x, v_curr.y, v_curr.z);
			ImGui::End();

			rlImGuiEnd();

			EndDrawing();
		}

		rlImGuiShutdown();
		CloseWindow();

		ost << "Demo 1 exited. Returned to CLI.\n";
	}
}
