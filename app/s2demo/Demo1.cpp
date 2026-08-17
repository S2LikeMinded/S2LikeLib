#include "S2DemoConfig.hpp"
#include "S2Raylib.hpp"
#include <rlgl.h>
#include <extras/IconsFontAwesome6.h>

#include "Demos.hpp"
#include "DemoSignalGuard.hpp"

#include <S2LL/Core/Regions.hpp>
#include <S2LL/Core/Surfaces.hpp>

#include <cmath>
#include <algorithm>
#include <format>
#include <optional>

using namespace S2LL;
using namespace S2LL::Literals;
using namespace ImGui;
using S2App::to_Vector3;
using S2App::to_quadric_matrix;
using S2App::TextFormatted;

namespace S2Demo
{
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
			const Double kx = Double::make(sx) / Double::make(sy);
			const Double kz = Double::make(sz) / Double::make(sy);
			shear_equation = std::format("(x,y,z) -> (x - ({:.3f})y, y, z - ({:.3f})y)",
				static_cast<double>(kx), static_cast<double>(kz));
			quadric_equation = std::format(
				"x\u00B2 + y\u00B2 + z\u00B2 + 2({:.3f})xy + 2({:.3f})yz + (({:.3f})\u00B2 + ({:.3f})\u00B2)y\u00B2 = {}",
				static_cast<double>(kx), static_cast<double>(kz),
				static_cast<double>(kx), static_cast<double>(kz),
				sphere_radius * sphere_radius);
		}
	};

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

	/// Builds a translucent mesh of the planar "segments": for each edge, the
	/// crescent between the chord and its great-elliptic arc, fanned from the
	/// first vertex. Vertex colors carry the translucent tint.
	template <typename Polygon>
	inline Mesh build_segment_mesh(const Polygon& poly, const Ellipsoid& e,
		const LinearTransformation& T, Color tint)
	{
		constexpr int arc_segments = 32;
		const size_t nv = poly.size();
		// Fan from A over the arc samples; the degenerate first triangle is skipped
		const size_t tris_per_edge = static_cast<size_t>(arc_segments) - 1;
		const size_t tri_count = nv * tris_per_edge;
		const int vertex_count = static_cast<int>(3 * tri_count);

		Mesh mesh{};
		mesh.vertexCount = vertex_count;
		mesh.triangleCount = static_cast<int>(tri_count);
		mesh.vertices = static_cast<float*>(MemAlloc(static_cast<size_t>(vertex_count) * 3 * sizeof(float)));
		mesh.normals = static_cast<float*>(MemAlloc(static_cast<size_t>(vertex_count) * 3 * sizeof(float)));
		mesh.colors = static_cast<unsigned char*>(MemAlloc(static_cast<size_t>(vertex_count) * 4 * sizeof(unsigned char)));

		size_t v = 0;
		for (size_t i = 0; i < nv; ++i)
		{
			const auto arc = poly.edge(static_cast<ptrdiff_t>(i), e, T);
			const auto pts = arc.sample(arc_segments);
			const E3 A = pts[0];
			for (int k = 1; k < arc_segments; ++k)
			{
				const E3 P = pts[static_cast<size_t>(k)];
				const E3 Q = pts[static_cast<size_t>(k + 1)];
				// Geometric normal of the (A, P, Q) fan triangle; the fill is
				// unlit and two-sided, so orientation is only cosmetic
				const E3 normal = (P - A).cross(Q - A).normalized();
				const E3 tri[3] = { A, P, Q };
				for (int j = 0; j < 3; ++j)
				{
					mesh.vertices[3 * v + 0] = static_cast<float>(tri[j].x);
					mesh.vertices[3 * v + 1] = static_cast<float>(tri[j].y);
					mesh.vertices[3 * v + 2] = static_cast<float>(tri[j].z);
					mesh.normals[3 * v + 0] = static_cast<float>(normal.x);
					mesh.normals[3 * v + 1] = static_cast<float>(normal.y);
					mesh.normals[3 * v + 2] = static_cast<float>(normal.z);
					mesh.colors[4 * v + 0] = tint.r;
					mesh.colors[4 * v + 1] = tint.g;
					mesh.colors[4 * v + 2] = tint.b;
					mesh.colors[4 * v + 3] = tint.a;
					++v;
				}
			}
		}
		UploadMesh(&mesh, false);
		return mesh;
	}

	/// Draws a line as alternating dashes (used for occluded interior chords)
	inline void draw_dashed_line(Vector3 a, Vector3 b, Color color, int dashes = 12)
	{
		for (int i = 0; i < dashes; ++i)
		{
			if (i % 2 != 0)
			{
				continue;
			}
			const float t0 = static_cast<float>(i) / dashes;
			const float t1 = static_cast<float>(i + 1) / dashes;
			DrawLine3D(Vector3Lerp(a, b, t0), Vector3Lerp(a, b, t1), color);
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
		auto [sin_a, cos_a] = SinCos(75_Deg);
		Double sqrt5 = Sqrt(5);
		Double qx = 3 * cos_a;
		Double qy = 6 * sin_a / sqrt5;
		Double qz = 3 * sin_a / sqrt5;

		// x: +/-(3/ 4)(Sqrt( 6)-Sqrt( 2))
		// y:    (3/10)(Sqrt(30)+Sqrt(10))
		// z:    (3/20)(Sqrt(30)+Sqrt(10))
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

		// Configure resizable Raylib window + rlImGui
		S2App::InitGuiApp("S2Demo - PiSP Paper", 800, 640);

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

		// Load the shared GLSL shader for smooth per-pixel shading
		S2App::SmoothSurfaceShader smooth_shader;
		const std::string vs_path = std::string(S2APP_SHADER_DIR) + "/smooth_surface.vs";
		const std::string fs_path = std::string(S2APP_SHADER_DIR) + "/smooth_surface.fs";
		smooth_shader.Load(vs_path, fs_path);

		// Quadric of the base sphere under each case's transformation
		const Ellipsoid base_sphere(static_cast<double>(cases[0].radius));
		std::array<BilinearForm, 3> quadrics{};
		for (size_t i = 0; i < cases.size(); ++i)
		{
			quadrics[i] = cases[i].transform.quadric(base_sphere);
		}

		// Model matrix aligning sphere poles with Z-axis in World Space
		Matrix model_matrix = MatrixRotateX(90.0f * DEG2RAD);

		// Surface meshes: identity (sphere, Demos 1a/1b) and sheared (Demo 1c)
		Mesh sphere_mesh = generate_surface_mesh(cases[0].radius, LinearTransformation{});
		Mesh sheared_mesh = generate_surface_mesh(cases[0].radius, cases[2].transform);
		Material sheared_material = LoadMaterialDefault();
		sheared_material.shader = smooth_shader.shader();
		sheared_material.maps[MATERIAL_MAP_DIFFUSE].color = LIGHTGRAY;

		// Translucent "segments": per-edge planar crescents between each chord
		// and its great-elliptic arc, one mesh per demo case
		std::array<Mesh, 3> segment_meshes{};
		const Color segment_tint{ 255, 200, 90, 110 };
		for (size_t i = 0; i < cases.size(); ++i)
		{
			segment_meshes[i] = build_segment_mesh(poly, s, cases[i].transform, segment_tint);
		}
		Material segment_material = LoadMaterialDefault();

		// Orbit camera looking from direction v at distance 12
		const double dist2cam = 12.0;
		S2App::OrbitCamera orbit;
		orbit.Init(v, static_cast<float>(dist2cam), 8.0f, 45.0f);
		bool orthographic = true;

		// View toggles
		bool show_xy_plane = false;
		bool show_axes = true;
		bool show_arc = true;
		bool show_chord = true;
		bool dashed_chords = true;  // solid mode: redraw occluded chords dashed
		bool show_segments = true;  // translucent crescents between chord and arc
		int ellipsoid_model = 3;    // default: solid view
		int show_case = 0;
		double vertex_size = 0.05f;

		// Interaction state
		bool rotate_tool = true;
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
			Ray ray = GetScreenToWorldRay(mouse_pos, orbit.camera());
			// The bilinear form M satisfies p^T M p = 1 on the surface
			const auto ray_hit_ellipsoid = ray_quadric_hit(ray, quadrics[static_cast<size_t>(show_case)], 1.0);
			bool mouse_outside_ellipsoid = !ray_hit_ellipsoid.has_value();

			smooth_shader.SetQuadric(quadrics[static_cast<size_t>(show_case)]);

			// Orbit camera: dragging starts only when the rotate tool is on
			// and the press is outside the surface (or the surface is hidden)
			orbit.drag_allowed = rotate_tool && (mouse_outside_ellipsoid || ellipsoid_model == 1);
			orbit.SetOrthographic(orthographic);
			orbit.Update();
			const Camera3D& cam = orbit.camera();

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
				S2App::DrawAxesLines(3.6f);
			}

			// Convert E3 vertices to Raylib Vector3
			std::array<Vector3, nv> pG;
			for (size_t i = 0; i < nv; ++i)
			{
				pG[i] = to_Vector3(view.transform(poly[i]));
			}
			Vector3 pQ = to_Vector3(query);

			// Opaque chord pass: drawn before any surface so that in
			// transparent mode the chords show through the surface. In solid
			// mode the chords are interior (occluded) and are handled as a
			// dashed overlay instead.
			if (show_chord && ellipsoid_model != 3)
			{
				for (size_t i = 0; i < nv; ++i)
				{
					DrawLine3D(pG[i], pG[(i + 1) % nv], GOLD);
				}
			}

			// Render sphere s
			switch (ellipsoid_model)
			{
			case 3: { // Solid view
				smooth_shader.SetViewPos(cam.position);
				smooth_shader.SetAlpha(1.0f);
				smooth_shader.SetModel(show_case == 2 ? MatrixIdentity() : model_matrix);
				smooth_shader.Begin();
				if (show_case == 2)
				{
					DrawMesh(sheared_mesh, sheared_material, MatrixIdentity());
				}
				else
				{
					DrawSphereEx(origin, radius, 64, 64, LIGHTGRAY);
				}
				smooth_shader.End();
			} break;
			case 0: { // Wireframe view
				draw_mesh_wireframe(show_case == 2 ? sheared_mesh : sphere_mesh, PURPLE);
				break;
			}
			case 1: // None: no surface drawn
				break;
			case 2: // Transparent view: drawn after the translucent fills
				break;
			}

			// Translucent edge segments (crescent between chord and arc):
			// skipped in solid mode to avoid clutter
			if (show_segments && ellipsoid_model != 3)
			{
				// Two-sided translucent fill: backface culling would hide one side
				rlDisableBackfaceCulling();
				DrawMesh(segment_meshes[static_cast<size_t>(show_case)], segment_material, MatrixIdentity());
				rlEnableBackfaceCulling();
			}

			// Transparent surface: drawn after the fills so the interior
			// chords and segments remain visible through it
			if (ellipsoid_model == 2)
			{
				smooth_shader.SetViewPos(cam.position);
				smooth_shader.SetAlpha(0.45f);
				smooth_shader.SetModel(show_case == 2 ? MatrixIdentity() : model_matrix);
				smooth_shader.Begin();
				if (show_case == 2)
				{
					DrawMesh(sheared_mesh, sheared_material, MatrixIdentity());
				}
				else
				{
					DrawSphereEx(origin, radius, 64, 64, LIGHTGRAY);
				}
				smooth_shader.End();
			}

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
				// Bias the arc slightly toward the camera so it clears the
				// surface depth (no z-fighting with the solid/transparent view)
				const E3 cam_pos{ cam.position.x, cam.position.y, cam.position.z };
				constexpr double surface_clearance = 0.02;
				for (size_t i = 0; i < nv; ++i)
				{
					const auto arc = poly.edge(i, s, view.transform);
					auto arc_pts = arc.sample(32);
					for (size_t k = 0; k + 1 < arc_pts.size(); ++k)
					{
						// Move toward the camera (subtract the view direction)
						const E3 b0 = arc_pts[k] - (arc_pts[k] - cam_pos).normalized() * surface_clearance;
						const E3 b1 = arc_pts[k + 1] - (arc_pts[k + 1] - cam_pos).normalized() * surface_clearance;
						Vector3 p0 = to_Vector3(b0);
						Vector3 p1 = to_Vector3(b1);

						DrawLine3D(p0, p1, BLACK);
					}
				}
			}

			// Solid mode: interior chords are occluded by the surface; when the
			// dashed-chords toggle is on, redraw them as a dashed overlay
			if (show_chord && ellipsoid_model == 3 && dashed_chords)
			{
				// rlgl batches line vertices and only applies GL state at flush
				// time, so flush with depth ON before disabling it, and flush
				// the dashes while depth is still OFF (EndMode3D would flush
				// them with depth re-enabled and cull them behind the surface)
				rlDrawRenderBatchActive();
				rlDisableDepthTest();
				for (size_t i = 0; i < nv; ++i)
				{
					draw_dashed_line(pG[i], pG[(i + 1) % nv], GOLD, 12);
				}
				rlDrawRenderBatchActive();
				rlEnableDepthTest();
			}

			EndMode3D();

			// Draw x, y, z axis labels at tips when axes are enabled
			if (show_axes)
			{
				S2App::DrawAxesLabels(3.6f, cam);
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
						orbit.Reset(v);
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
						RadioButton("Wireframe", &ellipsoid_model, 0);
						RadioButton("None", &ellipsoid_model, 1);
						RadioButton("Transparent", &ellipsoid_model, 2);
						RadioButton("Solid", &ellipsoid_model, 3);
						EndMenu();
					}
					if (BeginMenu("Edges"))
					{
						MenuItem("Sectional Arcs", nullptr, &show_arc);
						MenuItem("Straight Chords", nullptr, &show_chord);
						MenuItem("Shaded Segments", nullptr, &show_segments);
						MenuItem("Dashed Chords (Solid Ellipsoid Only)", nullptr, &dashed_chords);
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
					ellipsoid_model = 3;
					show_arc = true;
					show_chord = true;
					dashed_chords = true;
					show_segments = true;
					orthographic = true;
					orbit.Reset(v);
				}

				SameLine();
				if (Button(ICON_FA_LOCATION_CROSSHAIRS " View from Q"))
				{
					// The camera lives in world (sheared) space, so look from
					// the transformed query point T(Q0)
					auto coords = view.transform(view.Q0).ll();
					orbit.Reset(coords);
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
					ICON_FA_GLOBE " Wireframe " ICON_FA_GLOBE,
					ICON_FA_GLOBE " None " ICON_FA_GLOBE,
					ICON_FA_GLOBE " Transparent " ICON_FA_GLOBE,
					ICON_FA_GLOBE " Solid " ICON_FA_GLOBE,
				};
				PushItemWidth(120.0f);
				SliderInt("##SphereModel", &ellipsoid_model, 0, 3, model_labels[ellipsoid_model]);
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

		// raylib's UnloadMaterial() also unloads the shader attached to the
		// material (rmodels.c), and sheared_material.shader is a by-value copy
		// of the shared SmoothSurfaceShader. Clear the copy first so the RAII
		// owner frees the locations array exactly once.
		sheared_material.shader = Shader{};
		UnloadMaterial(sheared_material);
		UnloadMaterial(segment_material);
		UnloadMesh(sphere_mesh);
		UnloadMesh(sheared_mesh);
		for (Mesh& m : segment_meshes)
		{
			UnloadMesh(m);
		}
		S2App::ShutdownGuiApp();

		ost << "Demo 1 exited. Returned to CLI.\n";
	}
}
