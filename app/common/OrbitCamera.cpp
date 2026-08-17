#include "OrbitCamera.hpp"
#include "S2Raylib.hpp"

#include <algorithm>
#include <numbers>

namespace S2App
{
	void OrbitCamera::Init(const S2LL::E3& view_direction, float distance,
		float ortho_fovy, float persp_fovy)
	{
		_distance = distance;
		_fovy_ortho = ortho_fovy;
		_fovy_persp = persp_fovy;
		_cam.target = Vector3{ 0.0f, 0.0f, 0.0f };
		_cam.up = Vector3{ 0.0f, 0.0f, 1.0f };
		Reset(view_direction);
	}

	void OrbitCamera::Reset(const S2LL::E3& view_direction)
	{
		_angle = view_direction.ll();
		_view_needs_update = true;
	}

	void OrbitCamera::Reset(const S2LL::LL& view_angle)
	{
		_angle = view_angle;
		_view_needs_update = true;
	}

	void OrbitCamera::Update()
	{
		// Left-drag rotation (unless ImGui owns the mouse)
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			if (drag_allowed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				_dragging = true;
			}
		}
		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
		{
			_dragging = false;
		}
		if (_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			Vector2 delta = GetMouseDelta();
			_angle.lon -= static_cast<double>(delta.x) * 0.005;
			_angle.lat += static_cast<double>(delta.y) * 0.005;
			_angle.lat = std::clamp(
				_angle.lat, -std::numbers::pi / 2.0, std::numbers::pi / 2.0);
			_view_needs_update = true;
		}

		if (_view_needs_update)
		{
			_cam.position = to_Vector3(_distance * _angle.e3());
			// Up vector = latitude gradient, always perpendicular to the view
			_cam.up = to_Vector3(
				S2LL::LL{ _angle.lat + std::numbers::pi / 2.0, _angle.lon }.e3());
			_view_needs_update = false;
		}

		_cam.projection = _orthographic ? CAMERA_ORTHOGRAPHIC : CAMERA_PERSPECTIVE;
		_cam.fovy = _orthographic ? _fovy_ortho : _fovy_persp;
		UpdateCamera(&_cam, CAMERA_CUSTOM);
	}

	void OrbitCamera::SetOrthographic(bool orthographic) noexcept
	{
		_orthographic = orthographic;
	}

	bool OrbitCamera::orthographic() const noexcept
	{
		return _orthographic;
	}

	const Camera3D& OrbitCamera::camera() const noexcept
	{
		return _cam;
	}
}
