#pragma once

// Shared raylib / Dear ImGui / rlImGui support for the 3D demo applications.
// Including this header brings in the GUI stack, applies the raylib/Windows
// macro fixes both apps used to repeat, and provides the S2LL<->raylib
// bridging helpers.

#include <raylib.h>
#include <raymath.h>
#include <imgui.h>
#include <rlImGui.h>

// Raylib defines a few names that collide with Windows APIs / Dear ImGui;
// the demo apps used to repeat this #undef list in every file. Kept here so
// every app gets the same fix automatically.
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

#include <S2LL/Core/Coordinates.hpp>
#include <S2LL/Core/Surfaces.hpp>

#include "OrbitCamera.hpp"
#include "SmoothSurfaceShader.hpp"

#include <format>
#include <utility>

namespace S2App
{
	/// Converts an S2LL::E3 vector into a raylib Vector3
	inline Vector3 to_Vector3(const S2LL::E3& e) noexcept
	{
		return Vector3{ static_cast<float>(e.x), static_cast<float>(e.y), static_cast<float>(e.z) };
	}

	/// 4x4 matrix carrying the 3x3 bilinear form (row-major block) for the
	/// smooth surface shader's uQuadric uniform
	inline Matrix to_quadric_matrix(const S2LL::BilinearForm& q)
	{
		return Matrix{
			static_cast<float>(q.m[0]), static_cast<float>(q.m[1]), static_cast<float>(q.m[2]), 0.0f,
			static_cast<float>(q.m[3]), static_cast<float>(q.m[4]), static_cast<float>(q.m[5]), 0.0f,
			static_cast<float>(q.m[6]), static_cast<float>(q.m[7]), static_cast<float>(q.m[8]), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	/// ImGui text with std::format-style arguments
	template <typename... Args>
	inline void TextFormatted(std::format_string<Args...> fmt, Args&&... args)
	{
		ImGui::TextUnformatted(std::format(fmt, std::forward<Args>(args)...).c_str());
	}

	/// Resizable raylib window + rlImGui bootstrap used by every app
	inline void InitGuiApp(const char* title, int width, int height)
	{
		SetConfigFlags(FLAG_WINDOW_RESIZABLE);
		InitWindow(width, height, title);
		SetTargetFPS(60);
		rlImGuiSetup(true);
	}

	/// Teardown matching InitGuiApp
	inline void ShutdownGuiApp()
	{
		rlImGuiShutdown();
		CloseWindow();
	}

	/// Draws the X/Y/Z axes (inside BeginMode3D)
	inline void DrawAxesLines(float length)
	{
		const Vector3 origin{ 0.0f, 0.0f, 0.0f };
		DrawLine3D(origin, Vector3{ length, 0.0f, 0.0f }, RED);
		DrawLine3D(origin, Vector3{ 0.0f, length, 0.0f }, GREEN);
		DrawLine3D(origin, Vector3{ 0.0f, 0.0f, length }, BLUE);
	}

	/// Draws the x/y/z labels at the axis tips (after EndMode3D)
	inline void DrawAxesLabels(float length, const Camera3D& cam)
	{
		constexpr int font_size = 20;
		const float tip = length + 0.4f;
		Vector2 pX = GetWorldToScreen(Vector3{ tip, 0.0f, 0.0f }, cam);
		Vector2 pY = GetWorldToScreen(Vector3{ 0.0f, tip, 0.0f }, cam);
		Vector2 pZ = GetWorldToScreen(Vector3{ 0.0f, 0.0f, tip }, cam);
		DrawText("x", static_cast<int>(pX.x), static_cast<int>(pX.y), font_size, RED);
		DrawText("y", static_cast<int>(pY.x), static_cast<int>(pY.y), font_size, GREEN);
		DrawText("z", static_cast<int>(pZ.x), static_cast<int>(pZ.y), font_size, BLUE);
	}
}
