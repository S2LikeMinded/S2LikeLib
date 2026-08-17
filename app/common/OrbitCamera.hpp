#pragma once

#include <S2LL/Core/Coordinates.hpp>

#include <raylib.h>

namespace S2App
{
	/// Orbit camera around the origin, driven by the latitude-longitude of the
	/// view direction. Left-drag rotates; the up vector is the latitude
	/// gradient so the horizon stays level. Shared by the 3D demo apps.
	class OrbitCamera
	{
	public:
		/// Initializes the camera looking from `view_direction` toward the
		/// origin, at the given distance, with orthographic/perspective fovy.
		void Init(const S2LL::E3& view_direction, float distance,
			float ortho_fovy = 7.0f, float persp_fovy = 45.0f);

		/// Resets the view direction (identity rotation)
		void Reset(const S2LL::E3& view_direction);

		/// Resets to an explicit latitude-longitude view direction
		void Reset(const S2LL::LL& view_angle);

		/// Applies mouse-drag rotation and refreshes the raylib camera.
		/// Rotation starts only when `drag_allowed` is true (and the mouse is
		/// not captured by ImGui); apps with extra conditions set it per frame.
		void Update();

		/// Sets and gets orthographic mode
		void SetOrthographic(bool orthographic) noexcept;
		bool orthographic() const noexcept;

		/// Gate for starting a drag; checked inside Update()
		bool drag_allowed = true;

		/// Getter for the camera.
		const Camera3D& camera() const noexcept;

	private:
		Camera3D _cam{};
		S2LL::LL _angle{ 0.0, 0.0 };
		float _distance = 5.0f;
		float _fovy_ortho = 7.0f;
		float _fovy_persp = 45.0f;
		bool _orthographic = true;
		bool _dragging = false;
		bool _view_needs_update = true;
	};
}
