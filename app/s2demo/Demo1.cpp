#include "S2DemoConfig.hpp"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <rlImGui.h>
#include <extras/IconsFontAwesome6.h>
#include <imgui.h>

#include "Demos.hpp"
#include "DemoSignalGuard.hpp"

#include <S2LL/Core/Regions.hpp>
#include <S2LL/Core/Surfaces.hpp>

#include <cmath>
#include <algorithm>
#include <format>
#include <optional>

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

	/// One demo scene: a base surface (sphere) possibly sheared into a general
	/// ellipsoid, the world-space query point, and the equation strings shown
	/// in the info panel. The shear doubles only appear in the ellipsoid
	/// equation, never in the shear mapping itself.
	///
	/// The shear is (x,y,z) -> (x - (a/b)y, y, z - (c/b)y), where (a,b,c) are
	/// the coordinates of the base query point Q0 (Demo 1c uses Q_1b).
	struct CaseView
	{
		LinearTransformation transform;
		E3 Q;            // query point in world (sheared) space
		E3 Q0;           // query point in the pre-image (base) space
		float radius;    // base sphere radius
		std::string base_equation;     // e.g. x^2 + y^2 + z^2 = 9
		std::string shear_equation;    // e.g. (x,y,z) -> (x - (a/b)y, y, z - (c/b)y)
		std::string quadric_equation;  // e.g. x^2 + y^2 + z^2 + 2(a/b)xy + 2(c/b)yz + ((a/b)^2 + (c/b)^2)y^2 = 9

		CaseView(float sphere_radius, double sx, double sy, double sz, E3 q0)
			: transform(sx, sy, sz)
			, Q(transform(q0))
			, Q0(q0)
			, radius(sphere_radius)
			, base_equation(std::format("x\u00B2 + y\u00B2 + z\u00B2 = {}", sphere_radius * sphere_radius))
		{
			// Compute the shear factors with extended precision (the inputs are
			// themselves Double-derived), then format as decimal strings.
			const Double kx = Lift(sx) / Lift(sy);
			const Double kz = Lift(sz) / Lift(sy);
			shear_equation = std::format("(x,y,z) -> (x - ({:.3f})y, y, z - ({:.3f})y)",
				static_cast<double>(kx), static_cast<double>(kz));
			quadric_equation = std::format(
				"x\u00B2 + y\u00B2 + z\u00B2 + 2({:.3f})xy + 2({:.3f})yz + (({:.3f})\u00B2 + ({:.3f})\u00B2)y\u00B2 = {}",
				static_cast<double>(kx), static_cast<double>(kz),
				static_cast<double>(kx), static_cast<double>(kz),
				sphere_radius * sphere_radius);
		}
	};

	/// 4x4 matrix carrying the 3x3 bilinear form (row-major block) for the shader
	inline Matrix to_quadric_matrix(const BilinearForm& q)
	{
		return Matrix{
			static_cast<float>(q.m[0]), static_cast<float>(q.m[1]), static_cast<float>(q.m[2]), 0.0f,
			static_cast<float>(q.m[3]), static_cast<float>(q.m[4]), static_cast<float>(q.m[5]), 0.0f,
			static_cast<float>(q.m[6]), static_cast<float>(q.m[7]), static_cast<float>(q.m[8]), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	/// Builds the mesh of the image of the base sphere under the (possibly
	/// shearing) transformation. The sphere is rotationally symmetric, so
	/// transforming its vertices by T yields the ellipsoid in world space.
	inline Mesh generate_surface_mesh(
		float radius, const LinearTransformation& T, int rings = 64, int slices = 64)
	{
		Mesh mesh = GenMeshSphere(radius, rings, slices);
		// GenMeshSphere already uploaded the (sphere) mesh to the GPU; reset the
		// GPU binding so the sheared vertices below actually get uploaded.
		rlUnloadVertexArray(mesh.vaoId);
		if (mesh.vboId != nullptr)
		{
			// raylib's MAX_MESH_VERTEX_BUFFERS is 7 without GPU skinning
			for (int i = 0; i < 7; ++i)
			{
				rlUnloadVertexBuffer(mesh.vboId[i]);
			}
		}
		RL_FREE(mesh.vboId);
		mesh.vaoId = 0;
		mesh.vboId = nullptr;
		for (int i = 0; i < mesh.vertexCount; ++i)
		{
			const E3 p{
				mesh.vertices[3 * i + 0],
				mesh.vertices[3 * i + 1],
				mesh.vertices[3 * i + 2]
			};
			const E3 q = T(p);
			mesh.vertices[3 * i + 0] = static_cast<float>(q.x);
			mesh.vertices[3 * i + 1] = static_cast<float>(q.y);
			mesh.vertices[3 * i + 2] = static_cast<float>(q.z);
		}
		if (mesh.colors == nullptr)
		{
			mesh.colors = static_cast<unsigned char*>(MemAlloc(static_cast<size_t>(mesh.vertexCount) * 4));
		}
		for (int i = 0; i < mesh.vertexCount; ++i)
		{
			mesh.colors[4 * i + 0] = LIGHTGRAY.r;
			mesh.colors[4 * i + 1] = LIGHTGRAY.g;
			mesh.colors[4 * i + 2] = LIGHTGRAY.b;
			mesh.colors[4 * i + 3] = LIGHTGRAY.a;
		}
		UploadMesh(&mesh, false);
		return mesh;
	}

	/// Draws the edges of the surface mesh as a wireframe. Used to verify that
	/// the rendered solid surface is the sheared ellipsoid (the great-elliptic
	/// polygon arcs must lie on it).
	inline void draw_mesh_wireframe(const Mesh& mesh, Color color)
	{
		if (mesh.indices != nullptr)
		{
			for (int i = 0; i < mesh.triangleCount; ++i)
			{
				const unsigned short* tri = mesh.indices + 3 * static_cast<size_t>(i);
				const Vector3 p0{ mesh.vertices[3 * tri[0] + 0], mesh.vertices[3 * tri[0] + 1], mesh.vertices[3 * tri[0] + 2] };
				const Vector3 p1{ mesh.vertices[3 * tri[1] + 0], mesh.vertices[3 * tri[1] + 1], mesh.vertices[3 * tri[1] + 2] };
				const Vector3 p2{ mesh.vertices[3 * tri[2] + 0], mesh.vertices[3 * tri[2] + 1], mesh.vertices[3 * tri[2] + 2] };
				DrawLine3D(p0, p1, color);
				DrawLine3D(p1, p2, color);
				DrawLine3D(p2, p0, color);
			}
		}
		else
		{
			// Non-indexed mesh: vertices are consecutive triangle triples
			for (int i = 0; i < mesh.triangleCount; ++i)
			{
				const int v = 3 * i;
				const Vector3 p0{ mesh.vertices[3 * v + 0], mesh.vertices[3 * v + 1], mesh.vertices[3 * v + 2] };
				const Vector3 p1{ mesh.vertices[3 * v + 3], mesh.vertices[3 * v + 4], mesh.vertices[3 * v + 5] };
				const Vector3 p2{ mesh.vertices[3 * v + 6], mesh.vertices[3 * v + 7], mesh.vertices[3 * v + 8] };
				DrawLine3D(p0, p1, color);
				DrawLine3D(p1, p2, color);
				DrawLine3D(p2, p0, color);
			}
		}
	}

	/// Closest ray intersection with the quadric p^T M p = r^2 (p relative to
	/// the origin-centered surface), used for surface picking.
	inline std::optional<E3> ray_quadric_hit(
		const Ray& ray, const BilinearForm& M, double rhs)
	{
		return M.intersect_ray(
			E3{ ray.position.x, ray.position.y, ray.position.z },
			E3{ ray.direction.x, ray.direction.y, ray.direction.z },
			rhs);
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
		// Great-elliptic 4-gon on the sphere
		const GEP<4> poly{ {0, 0, 3}, {3, 0, 0}, {2, 2, 1}, {0, 3, 0} };
		const auto nv = poly.size();
		// Query points for Demos 1a & 1b: Q lies on sphere R=3 and on the same great circle
		// as poly's vertices[1] (3,0,0) and vertices[2] (2,2,1), at angle theta = 5*pi/12 (75 deg).
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

		// Demo 1c shears the whole scene (sphere, polygon, query point) by
		// (x,y,z) -> (x - (a/b)y, y, z - (c/b)y), where (a,b,c) = Q_1b.
		const std::array<CaseView, 3> cases{
			CaseView{ 3.0f, 0.0, 1.0, 0.0, Q_1a }, // 1a: identity, Q = Q_1a
			CaseView{ 3.0f, 0.0, 1.0, 0.0, Q_1b }, // 1b: identity, Q = Q_1b
			CaseView{ 3.0f, static_cast<double>(-qx), static_cast<double>(qy), static_cast<double>(qz), Q_1b } // 1c: shear
		};

		// Direction of view (viewing-from direction, normalized)
		const E3 v = E3{ 1, 1, 1 }.normalize();

		// Configure resizable Raylib window
		SetConfigFlags(FLAG_WINDOW_RESIZABLE);
		InitWindow(800, 640, "S2Demo - PiSP Paper");
		SetTargetFPS(60);

		// Initialize rlImGui
		rlImGuiSetup(true);

		// rlImGui's default font is the embedded ProggyClean bitmap or the
		// ProggyForever subset (depending on display scale), with Font Awesome
		// icons merged into it. Merge the Latin-1 glyphs of the full
		// ProggyClean.ttf (e.g. U+00B2s, superscript two) into that same font
		// so the info panel can render superscripts; the ASCII and icon glyphs
		// are left untouched.
		const std::string font_path = std::string(S2DEMO_RESOURCE_DIR) + "/ProggyClean.ttf";
		ImFontConfig font_cfg;
		font_cfg.MergeMode = true;
		font_cfg.PixelSnapH = true;
		font_cfg.SizePixels = 13.0f;
#if !defined(__APPLE__)
		if (!IsWindowState(FLAG_WINDOW_HIGHDPI))
			font_cfg.SizePixels = ceilf(font_cfg.SizePixels * GetWindowScaleDPI().y);
		font_cfg.RasterizerMultiply = GetWindowScaleDPI().y;
#endif
		static const ImWchar latin1_supplement[] = { 0xA0, 0xFF, 0 };
		GetIO().Fonts->AddFontFromFileTTF(font_path.c_str(), font_cfg.SizePixels,
			&font_cfg, latin1_supplement);

		// Load GLSL shader for smooth per-pixel shading from standalone files
		const std::string vs_path = std::string(S2DEMO_SHADER_DIR) + "/smooth_ellipsoid.vs";
		const std::string fs_path = std::string(S2DEMO_SHADER_DIR) + "/smooth_ellipsoid.fs";
		Shader smooth_shader = LoadShader(vs_path.c_str(), fs_path.c_str());
		struct ShaderLocations
		{
			int center;
			int quadric;
			int light_dir;
			int view_pos;
			int model;
		} locs{
			GetShaderLocation(smooth_shader, "uCenter"),
			GetShaderLocation(smooth_shader, "uQuadric"),
			GetShaderLocation(smooth_shader, "uLightDir"),
			GetShaderLocation(smooth_shader, "uViewPos"),
			GetShaderLocation(smooth_shader, "uModel")
		};

		float center_val[3] = { 0.0f, 0.0f, 0.0f };
		// Quadric of the base sphere under each case's transformation
		const Ellipsoid base_sphere(static_cast<double>(cases[0].radius));
		std::array<BilinearForm, 3> quadrics{};
		for (size_t i = 0; i < cases.size(); ++i)
		{
			quadrics[i] = cases[i].transform.quadric(base_sphere);
		}
		// Light coming from +z direction in world space
		float light_dir_val[3] = { 0.0f, 0.0f, 1.0f };

		// Model matrix aligning sphere poles with Z-axis in World Space
		Matrix model_matrix = MatrixRotateX(90.0f * DEG2RAD);

		SetShaderValue(smooth_shader, locs.center, center_val, SHADER_UNIFORM_VEC3);
		SetShaderValue(smooth_shader, locs.light_dir, light_dir_val, SHADER_UNIFORM_VEC3);

		// Surface meshes: identity (sphere, Demos 1a/1b) and sheared (Demo 1c)
		Mesh sphere_mesh = generate_surface_mesh(cases[0].radius, LinearTransformation{});
		Mesh sheared_mesh = generate_surface_mesh(cases[0].radius, cases[2].transform);
		Material sheared_material = LoadMaterialDefault();
		sheared_material.shader = smooth_shader;
		sheared_material.maps[MATERIAL_MAP_DIFFUSE].color = LIGHTGRAY;

		// Setup 3D Camera positioned from direction v looking at origin
		double dist2cam = 12.0;
		Camera3D cam   = { 0 };
		cam.position   = to_Vector3(dist2cam * v);
		cam.target     = Vector3{ 0.0f, 0.0f, 0.0f };
		cam.up         = Vector3{ 0.0f, 0.0f, 1.0f };
		bool orthographic = true;
		cam.fovy       = orthographic ? 8.0f : 45.0f;
		cam.projection = orthographic ? CAMERA_ORTHOGRAPHIC : CAMERA_PERSPECTIVE;

		// Camera state
		LL cam_angle = v.ll();
		bool view_needs_update = true;

		// View toggles
		bool show_xy_plane = false;
		bool show_axes = true;
		bool show_arc = true;
		bool show_chord = true;
		int sphere_model = 1;
		int show_case = 0;
		double vertex_size = 0.05f;

		// Interaction state
		bool rotate_tool = true;
		bool is_dragging_rotation = false;
		bool exit_requested = false;

		ost << "3D Window Launched (Arbitrarily Resizable).\n"
			<< "Press ESC, close window, or Ctrl+C to return to CLI.\n";

		Vector3 origin{ 0.0f, 0.0f, 0.0f };

		while (!WindowShouldClose() && !DemoSignalGuard::isInterrupted() && !exit_requested)
		{
			const CaseView& view = cases[static_cast<size_t>(show_case)];
			const E3 query = view.Q;
			float radius = view.radius;

			Vector2 mouse_pos = GetMousePosition();
			Ray ray = GetScreenToWorldRay(mouse_pos, cam);
			// The bilinear form M satisfies p^T M p = 1 on the surface
			const auto surface_hit = ray_quadric_hit(ray, quadrics[static_cast<size_t>(show_case)], 1.0);
			bool outside_sphere = !surface_hit.has_value();

			SetShaderValueMatrix(smooth_shader, locs.quadric,
				to_quadric_matrix(quadrics[static_cast<size_t>(show_case)]));

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
				cam_angle.lon -= static_cast<double>(delta.x) * 0.005;
				cam_angle.lat += static_cast<double>(delta.y) * 0.005;
				cam_angle.lat  = std::clamp(cam_angle.lat, -90_deg, 90_deg);
				view_needs_update = true;
			}

			if (view_needs_update)
			{
				cam.position = to_Vector3(dist2cam * cam_angle.e3());
				// Up vector = latitude gradient, always perpendicular to view direction
				cam.up = to_Vector3(LL{ cam_angle.lat + 0.5_pi, cam_angle.lon }.e3());
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
				SetShaderValue(smooth_shader, locs.view_pos, view_pos_val, SHADER_UNIFORM_VEC3);
				SetShaderValueMatrix(smooth_shader, locs.model,
					show_case == 2 ? MatrixIdentity() : model_matrix);
				BeginShaderMode(smooth_shader);
				if (show_case == 2)
				{
					DrawMesh(sheared_mesh, sheared_material, MatrixIdentity());
				}
				else
				{
					DrawSphereEx(origin, radius, 64, 64, LIGHTGRAY);
				}
				EndShaderMode();
			} break;
			case 2: { // Wireframe view
				draw_mesh_wireframe(show_case == 2 ? sheared_mesh : sphere_mesh, PURPLE);
				break;
			}
			}

			// Convert E3 vertices to Raylib Vector3
			std::array<Vector3, nv> pG;
			for (size_t i = 0; i < nv; ++i)
			{
				pG[i] = to_Vector3(view.transform(poly[i]));
			}
			Vector3 pQ = to_Vector3(query);

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
					const auto arc = poly.edge(i, s, view.transform);
					auto arc_pts = arc.sample(32);
					for (size_t k = 0; k + 1 < arc_pts.size(); ++k)
					{
						Vector3 p0 = to_Vector3(arc_pts[k]);
						Vector3 p1 = to_Vector3(arc_pts[k + 1]);

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
				const int axes_label_font_size = 20;
				const int axes_label_font_spacing = 0;

				Vector2 pX = GetWorldToScreen(Vector3{ 4.0f, 0.0f, 0.0f }, cam);
				Vector2 pY = GetWorldToScreen(Vector3{ 0.0f, 4.0f, 0.0f }, cam);
				Vector2 pZ = GetWorldToScreen(Vector3{ 0.0f, 0.0f, 4.0f }, cam);

				Vector2 label_size = MeasureTextEx(GetFontDefault(),
					"x", axes_label_font_size, axes_label_font_spacing);
				pX.x -= 0.5f * label_size.x;
				pX.y -= 0.5f * label_size.y;
				pY.x -= 0.5f * label_size.x;
				pY.y -= 0.5f * label_size.y;
				pZ.x -= 0.5f * label_size.x;
				pZ.y -= 0.5f * label_size.y;

				DrawText("x", static_cast<int>(pX.x), static_cast<int>(pX.y), axes_label_font_size, RED);
				DrawText("y", static_cast<int>(pY.x), static_cast<int>(pY.y), axes_label_font_size, GREEN);
				DrawText("z", static_cast<int>(pZ.x), static_cast<int>(pZ.y), axes_label_font_size, BLUE);
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
						cam_angle = v.ll();
						view_needs_update = true;
					}
					Separator();
					MenuItem("Orthographic View", nullptr, &orthographic);
					MenuItem("Rotate View", nullptr, &rotate_tool);
					EndMenu();
				}
				if (BeginMenu("Display"))
				{
					if (BeginMenu("Ellipsoid"))
					{
						RadioButton("None", &sphere_model, 0);
						RadioButton("Solid", &sphere_model, 1);
						RadioButton("Wireframe", &sphere_model, 2);
						EndMenu();
					}
					if (BeginMenu("Edges"))
					{
						MenuItem("Sectional Arcs", nullptr, &show_arc);
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
					RadioButton("1c: Ellipsoidal 4-gon (Fig. 1hi)", &show_case, 2);
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
					cam_angle = v.ll();
					view_needs_update = true;
				}

				SameLine();
				if (Button(ICON_FA_LOCATION_CROSSHAIRS " View from Q"))
				{
					// The camera lives in world (sheared) space, so look from
					// the transformed query point T(Q0)
					auto coords = view.transform(view.Q0).ll();
					cam_angle = coords;
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
					ICON_FA_GLOBE " Wireframe " ICON_FA_GLOBE,
				};
				PushItemWidth(120.0f);
				SliderInt("##SphereModel", &sphere_model, 0, 2, model_labels[sphere_model]);
				PopItemWidth();
			}
			End();

			SetNextWindowPos(ImVec2(10.0f, GetFrameHeight() + 48.0f), ImGuiCond_FirstUseEver);
			std::string controls_title = std::format("S2Demo - Demo 1{}###Controls", "abc"[show_case]);
			Begin(controls_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			if (show_case == 2)
			{
				TextUnformatted("Sheared ellipsoid (image of sphere)");
				TextFormatted("  base: {}", view.base_equation);
				TextFormatted("  shear: {}", view.shear_equation);
				TextFormatted("  quadric: {}", view.quadric_equation);
			}
			else
			{
				TextUnformatted("Sphere");
				TextFormatted("  {}", view.base_equation);
			}
			Separator();
			if (show_case == 2)
			{
				TextFormatted("Spherical {}-gon \u2192 Ellipsoidal {}-gon", nv, nv);
			}
			else
			{
				TextFormatted("Spherical {}-gon", nv);
			}
			for (size_t i = 0; i < nv; ++i)
			{
				const char* prefix = (hovered_idx == static_cast<int>(i)) ? ICON_FA_CHEVRON_RIGHT : " ";
				if (show_case == 2)
				{
					// 1c: show the pre-image and sheared (world) coordinates
					const E3 sheared = view.transform(poly[i]);
					TextFormatted("{} {}: ({: .3f},{: .3f},{: .3f}) \u2192 ({: .3f},{: .3f},{: .3f})",
						prefix, "ABCD"[i],
						poly[i].x, poly[i].y, poly[i].z,
						sheared.x, sheared.y, sheared.z);
				}
				else
				{
					TextFormatted("{} {}: ({: },{: },{: })", prefix, "ABCD"[i],
						poly[i].x, poly[i].y, poly[i].z);
				}
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
				if (show_case == 2)
				{
					TextFormatted("{} Q: ({: .3f},{: .3f},{: .3f}) \u2192 ({: .3f},{: .3f},{: .3f})",
						prefix,
						view.Q0.x, view.Q0.y, view.Q0.z,
						view.Q.x, view.Q.y, view.Q.z);
				}
				else
				{
					TextFormatted("{} Q: ({: .3f},{: .3f},{: .3f})", prefix, view.Q.x, view.Q.y, view.Q.z);
				}
			}
			Separator();
			TextUnformatted("Viewing From Direction");
			TextFormatted("  v: ({: .3f},{: .3f},{: .3f})", v_curr.x, v_curr.y, v_curr.z);
			End();

			// Display hover tooltip inside active ImGui frame scope
			if (hovered_idx >= 0 && static_cast<size_t>(hovered_idx) < nv)
			{
				BeginTooltip();
				// In 1c the markers are drawn at their sheared (world) positions
				const E3 hovered_vertex = (show_case == 2)
					? view.transform(poly[hovered_idx])
					: poly[hovered_idx];
				TextFormatted("{}({},{},{})", "ABCD"[hovered_idx],
					hovered_vertex.x, hovered_vertex.y, hovered_vertex.z);
				EndTooltip();
			}
			else if (hovered_idx == static_cast<int>(nv))
			{
				BeginTooltip();
				TextFormatted("Q({},{},{})", query.x, query.y, query.z);
				EndTooltip();
			}

			rlImGuiEnd();

			EndDrawing();
		}

		rlImGuiShutdown();
		UnloadMaterial(sheared_material);
		UnloadMesh(sphere_mesh);
		UnloadMesh(sheared_mesh);
		CloseWindow();

		ost << "Demo 1 exited. Returned to CLI.\n";
	}
}
