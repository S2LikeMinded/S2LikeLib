#include "Hare.hpp"
#include "S2Raylib.hpp"

#include <S2LL/Core/Coordinates.hpp>
#include <S2LL/Core/Surfaces.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

using namespace S2LL;
using namespace S2LL::Literals;
using namespace ImGui;
using S2App::to_Vector3;
using S2App::to_quadric_matrix;
using S2App::TextFormatted;

namespace
{
	/// Radius of the sphere.
	constexpr float kSphereRadius = 1.5f;

	/// Radial distance between rendered trails and the sphere.
	constexpr double kTrailClearance = 0.02;

	/// Distance from the camera to the center of the sphere.
	constexpr float kCameraDist = 5.0f;

	/// Application state: creature, parsed program, UI.
	struct AppState
	{
		// Creature state.
		S2Hare::CreatureState creature;

		// Parsed logo program.
		std::vector<S2Hare::Step> steps;

		// Logo program counter indiciating the next step to execute.
		std::size_t pc = 0;

		// Remaining signed radians to complete in a forward command.
		Double anim_remaining{};

		// Is the parsed logo program running?
		bool running = false;

		// Creature speed in radians per second.
		Double speed = 120.0_Deg;

		// Currently selected creature.
		S2Hare::Creature creature_type = S2Hare::Creature::Hare;

		// Render the trail?
		bool show_trail = true;

		// Render the x-, y-, z-axes?
		bool show_axes = true;

		// How should the sphere be rendered?
		int sphere_mode = 3;

		// Orthographic view?
		bool orthographic = true;

		// Exit the program?
		bool exit_requested = false;

		// Stores a description of the final error.
		std::string last_error;

		// A default program to show.
		char program[4096] = "repeat 6 [ fd 90 rt 90 ]";

		// Current file the program was loaded from or saved to (empty = unsaved).
		std::string file_path;

		// File dialog state.
		bool file_dialog_open = false;
		bool file_dialog_save = false;
		bool show_about = false;
		std::filesystem::path dialog_dir;
		std::string dialog_error;
		char dialog_filename[1024] = {};
	};

	/// Parses the program text and starts executing it from the current state.
	void run_program(AppState& st)
	{
		st.last_error.clear();
		std::vector<S2Hare::Step> parsed;
		if (!S2Hare::parse_program(st.program, parsed, st.last_error))
		{
			st.running = false;
			st.steps.clear();
			st.pc = 0;
			st.anim_remaining = 0.0;
			return;
		}
		st.steps = std::move(parsed);
		st.pc = 0;
		st.anim_remaining = 0.0;
		st.running = true;
	}

	/// Advances the program by dt seconds; Forward steps animate at `speed`.
	void advance(AppState& st, double dt)
	{
		if (!st.running)
		{
			return;
		}
		// Speed is stored in radians per second (see AppState::speed); scale it
		// by the frame time to get the per-frame budget in radians, matching
		// anim_remaining and Step::arg.
		double budget = static_cast<double>(st.speed * dt);
		while (budget > 0.0 && st.running)
		{
			if (!st.anim_remaining.iszero())
			{
				const double rem = static_cast<double>(st.anim_remaining);
				if (std::abs(rem) <= budget)
				{
					// Consume budget exactly if the whole remaining arc fits.
					st.creature.forward(st.anim_remaining);
					st.anim_remaining = 0.0;
					++st.pc;
				}
				else
				{
					const double angle = std::copysign(budget, rem);
					st.creature.forward(Double::make(angle));
					st.anim_remaining = st.anim_remaining - angle;
					budget -= std::abs(angle);
				}
			}
			else if (st.pc < st.steps.size())
			{
				const S2Hare::Step& s = st.steps[st.pc];
				if (s.type == S2Hare::Step::Type::Forward)
				{
					st.anim_remaining = s.arg;
					if (st.anim_remaining.iszero())
					{
						++st.pc;
					}
				}
				else
				{
					st.creature.step(s);
					++st.pc;
				}
			}
			else
			{
				st.running = false;
			}
		}
		if (st.pc >= st.steps.size() && st.anim_remaining.iszero())
		{
			st.running = false;
		}
	}

	/// Executes exactly one pending step (used by the Step button).
	void step_once(AppState& st)
	{
		if (!st.anim_remaining.iszero())
		{
			st.creature.forward(st.anim_remaining);
			st.anim_remaining = 0.0;
			++st.pc;
		}
		else if (st.pc < st.steps.size())
		{
			st.creature.step(st.steps[st.pc]);
			++st.pc;
		}
	}

	/// Loads a preset, clears the sphere, and starts it running.
	void run_preset(AppState& st, const char* program)
	{
		std::snprintf(st.program, sizeof(st.program), "%s", program);
		st.creature.clear();
		st.creature.home();
		run_program(st);
	}

	/// Writes the current program text to `path`; returns true on success.
	bool save_to_path(AppState& st, const std::string& path)
	{
		return SaveFileText(path.c_str(), st.program);
	}

	/// Applies the file dialog's current selection: load (Open) or write (Save As).
	void file_dialog_accept(AppState& st)
	{
		st.dialog_error.clear();
		const std::filesystem::path path = st.dialog_dir / st.dialog_filename;
		if (st.file_dialog_save)
		{
			if (!save_to_path(st, path.string()))
			{
				st.dialog_error = "Could not write file: " + path.string();
				return;
			}
			st.file_path = path.string();
		}
		else
		{
			char* text = LoadFileText(path.string().c_str());
			if (!text)
			{
				st.dialog_error = "Could not open file: " + path.string();
				return;
			}
			std::snprintf(st.program, sizeof(st.program), "%s", text);
			UnloadFileText(text);
			st.file_path = path.string();
		}
		st.file_dialog_open = false;
	}

	/// Opens the file dialog in the given mode (true = Save As, false = Open).
	void open_file_dialog(AppState& st, bool save)
	{
		st.file_dialog_save = save;
		st.dialog_error.clear();
		if (st.dialog_dir.empty())
		{
			st.dialog_dir = GetWorkingDirectory();
		}
		st.file_dialog_open = true;
	}

	/// ImGui window for Open / Save As.
	void show_file_dialog(AppState& st)
	{
		auto& path = st.dialog_dir;
		if (path.empty())
		{
			path = GetWorkingDirectory();
		}

		ImGui::SetNextWindowSize(ImVec2(560, 420), ImGuiCond_Appearing);
		if (!Begin("File Dialog", &st.file_dialog_open))
		{
			End();
			return;
		}

		TextUnformatted(path.string().c_str());
		SameLine();
		bool has_parent = path.has_parent_path() && path.parent_path() != path;
		if (has_parent)
		{
			if (Button("\uf062"))
			{
				const char* parent = GetDirectoryPath(path.string().c_str());
				if (DirectoryExists(parent))
				{
					path = parent;
				}
			}
		}

		if (!st.dialog_error.empty())
		{
			PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
			TextUnformatted(st.dialog_error.c_str());
			PopStyleColor();
		}

		Separator();

		if (BeginChild("##filelist", ImVec2(0, 220), true))
		{
			FilePathList list = LoadDirectoryFiles(path.string().c_str());
			std::vector<std::string> dirs, files;
			dirs.reserve(list.count);
			files.reserve(list.count);
			for (int i = 0; i < list.count; ++i)
			{
				const std::string name = GetFileName(list.paths[i]);
				if (DirectoryExists(list.paths[i]))
				{
					dirs.push_back(name);
				}
				else
				{
					files.push_back(name);
				}
			}
			std::sort(dirs.begin(), dirs.end());
			std::sort(files.begin(), files.end());

			for (const std::string& d : dirs)
			{
				if (Selectable(("[dir] " + d).c_str(), false) && IsMouseDoubleClicked(0))
				{
					path = path / d;
				}
			}
			for (const std::string& fl : files)
			{
				const bool selected = (st.dialog_filename == fl);
				if (Selectable(fl.c_str(), selected))
				{
					std::snprintf(st.dialog_filename, sizeof(st.dialog_filename), "%s", fl.c_str());
					if (IsMouseDoubleClicked(0))
					{
						file_dialog_accept(st);
					}
				}
			}
			UnloadDirectoryFiles(list);
			EndChild();
		}

		InputText("File name", st.dialog_filename, sizeof(st.dialog_filename));

		if (Button(st.file_dialog_save ? "Save" : "Open"))
		{
			if (st.dialog_filename[0] != '\0')
			{
				file_dialog_accept(st);
			}
		}
		SameLine();
		if (Button("Cancel"))
		{
			st.file_dialog_open = false;
		}

		End();
	}

	/// Menu bar: File (Open / Save / Save As / Quit) and Help (About).
	void show_menu_bar(AppState& st)
	{
		if (BeginMainMenuBar())
		{
			if (BeginMenu("File"))
			{
				if (MenuItem("Open..."))
				{
					open_file_dialog(st, false);
				}
				if (MenuItem("Save", nullptr, false, !st.file_path.empty()))
				{
					save_to_path(st, st.file_path);
				}
				if (MenuItem("Save As..."))
				{
					open_file_dialog(st, true);
				}
				Separator();
				if (MenuItem("Quit"))
				{
					st.exit_requested = true;
				}
				EndMenu();
			}
			if (BeginMenu("Help"))
			{
				if (MenuItem("About"))
				{
					st.show_about = true;
				}
				EndMenu();
			}
			EndMainMenuBar();
		}
	}

	/// Modal About box.
	void show_about(AppState& st)
	{
		if (st.show_about)
		{
			OpenPopup("About S\u00B2Hare");
		}
		if (BeginPopupModal("About S\u00B2Hare", &st.show_about, ImGuiWindowFlags_AlwaysAutoResize))
		{
			TextUnformatted("S\u00B2Hare - Hare on a Sphere");
			Separator();
			TextUnformatted("A spherical turtle (Logo) interpreter: the creature walks");
			TextUnformatted("along great circles on the surface of a unit sphere,");
			TextUnformatted("drawing a trail as it goes.");
			TextUnformatted("");
			TextUnformatted("Commands: fd, bk, rt, lt, pu, pd, home, cs, repeat N [ ... ]");
			TextUnformatted("Load and save programs via File > Open / Save / Save As.");
			Separator();
			if (Button("OK"))
			{
				st.show_about = false;
			}
			EndPopup();
		}
	}

	/// Draws the creature as raylib primitives standing on the sphere surface.
	/// The local +x axis is the heading, +z is the surface normal ("up"),
	/// so the ears (creature) read as a heading indicator.
	void draw_creature(const S2Hare::CreatureState& h, S2Hare::Creature creature, float scale)
	{
		const E3 up = h.pos.normalized();
		const E3 forward = h.heading.normalized();
		const E3 left = up.cross(forward);

		const auto world = [&](double lx, double ly, double lz)
		{
			return to_Vector3(h.pos * scale + forward * lx + left * ly + up * lz);
		};

		if (creature == S2Hare::Creature::Hare)
		{
			// Tail
			DrawSphere(world(-0.1, 0.0, 0.06), 0.040f, WHITE);
			// Body
			DrawSphere(world(0.02, 0.0, 0.115), 0.105f, RAYWHITE);
			// Head
			DrawSphere(world(0.16, 0.0, 0.19), 0.065f, RAYWHITE);
			// Ears: two cones pointing along "up", bases on the head
			for (int side : { -1, 1 })
			{
				const double ly = side * 0.03;
				DrawCylinderEx(world(0.14, ly, 0.235), world(0.09, ly, 0.4),
					0.02f, 0.0f, 8, PINK);
			}
			// Eyes
			for (int side : { -1, 1 })
			{
				DrawSphere(world(0.21, side * 0.024, 0.175), 0.013f, BLACK);
			}
		}
		else // Tortoise
		{
			// Legs
			for (int sx : { -1, 1 })
			{
				for (int sz : { -1, 1 })
				{
					DrawSphere(world(sx * 0.06, sz * 0.11, 0.04), 0.028f, GREEN);
				}
			}
			// Head
			DrawSphere(world(0.19, 0.0, 0.12), 0.055f, GREEN);
			// Eyes
			for (int side : { -1, 1 })
			{
				DrawSphere(world(0.225, side * 0.020, 0.145), 0.011f, BLACK);
			}
			// Shell (dome over body)
			DrawSphere(world(0.0, 0.0, 0.145), 0.13f, DARKGREEN);
			DrawSphere(world(0.0, 0.0, 0.19), 0.09f, BROWN);
		}
	}
}

int main()
{
	constexpr float r = kSphereRadius;
	const double inv_r2 = 1.0 / (r * r);

	S2App::InitGuiApp("S\u00B2Hare - Hare on a Sphere", 1000, 760);

	// Load the smooth per-pixel shader
	S2App::SmoothSurfaceShader surface;
	const std::string vs_path = std::string(S2APP_SHADER_DIR) + "/smooth_surface.vs";
	const std::string fs_path = std::string(S2APP_SHADER_DIR) + "/smooth_surface.fs";
	surface.Load(vs_path, fs_path);

	// Unit-sphere quadric scaled to the scene radius: p^T M p = 1, M = diag(1/r^2)
	const BilinearForm quadric{
		inv_r2, 0.0, 0.0,
		0.0, inv_r2, 0.0,
		0.0, 0.0, inv_r2
	};
	// Align the raylib sphere's Y-axis with the scene's Z-axis
	const Matrix model_matrix = MatrixRotateX(90.0f * DEG2RAD);

	// Camera looking from v = (1,1,1) toward the origin, Z-up
	const E3 v = E3{ 1.0, 1.0, 1.0 }.normalize();
	S2App::OrbitCamera orbit;
	orbit.Init(v, kCameraDist, 7.0f, 45.0f);

	AppState st;

	while (!WindowShouldClose() && !st.exit_requested)
	{
		const float dt = GetFrameTime();
		advance(st, dt);

		orbit.SetOrthographic(st.orthographic);
		orbit.Update();
		const Camera3D& cam = orbit.camera();

		BeginDrawing();
		ClearBackground(DARKGRAY);
		BeginMode3D(cam);

		const Vector3 origin{ 0.0f, 0.0f, 0.0f };
		if (st.show_axes)
		{
			S2App::DrawAxesLines(2.0f);
		}

		// Sphere (transparent/solid via the smooth shader)
		if (st.sphere_mode == 2 || st.sphere_mode == 3)
		{
			surface.SetViewPos(cam.position);
			surface.SetAlpha(st.sphere_mode == 3 ? 1.0f : 0.45f);
			surface.SetQuadric(quadric);
			surface.SetModel(model_matrix);
			surface.Begin();
			DrawSphereEx(origin, r, 48, 48, LIGHTGRAY);
			surface.End();
		}
		else if (st.sphere_mode == 1)
		{
			DrawSphereWires(origin, r, 32, 32, PURPLE);
		}

		// Trail: polyline just above the surface to avoid z-fighting
		if (st.show_trail && st.creature.trail.size() > 1)
		{
			const float s = r + static_cast<float>(kTrailClearance);
			const Color trail_color =
				st.creature_type == S2Hare::Creature::Hare ? ORANGE : GREEN;
			for (std::size_t i = 1; i < st.creature.trail.size(); ++i)
			{
				DrawLine3D(to_Vector3(st.creature.trail[i - 1]) * s,
					to_Vector3(st.creature.trail[i]) * s, trail_color);
			}
		}

		// Home marker (the start point, currently at the north pole)
		DrawSphere(to_Vector3(E3{ 0.0, 0.0, 1.0 }) * (r + 0.02f), 0.035f, GOLD);

		// The creature itself
		draw_creature(st.creature, st.creature_type, r);

		EndMode3D();

		if (st.show_axes)
		{
			S2App::DrawAxesLabels(2.0f, cam);
		}

		rlImGuiBegin();

		show_menu_bar(st);

		if (Begin("S\u00B2Hare", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			const char* creatures[] = { "Hare", "Tortoise" };
			int creature_index = static_cast<int>(st.creature_type);
			if (Combo("Creature", &creature_index, creatures, 2))
			{
				st.creature_type = static_cast<S2Hare::Creature>(creature_index);
			}
			if (IsItemHovered())
			{
				SetTooltip("Settings: switch between the hare and the tortoise");
			}
			Separator();

			TextUnformatted("Program (fd, bk, rt, lt, pu, pd, home, cs, repeat N [ ... ])");
			InputTextMultiline("##program", st.program, sizeof(st.program),
				ImVec2(420, 110));

			if (Button("Run"))
			{
				run_program(st);
			}
			SameLine();
			if (Button("Stop"))
			{
				st.running = false;
			}
			SameLine();
			if (Button("Step"))
			{
				step_once(st);
			}
			SameLine();
			if (Button("Reset"))
			{
				st.creature.clear();
				st.creature.home();
				st.pc = 0;
				st.anim_remaining = Double{};
				st.running = false;
			}

			float speed_deg = static_cast<float>(ToDeg(st.speed));
			if (SliderFloat("Speed (deg/s)", &speed_deg, 15.0f, 360.0f, "%.0f"))
			{
				st.speed = FromDeg(speed_deg);
			}

			Separator();
			TextUnformatted("Presets");
			for (std::size_t i = 0; i < S2Hare::kPresetCount; ++i)
			{
				// Two per line
				if (i % 2 != 0)
				{
					SameLine();
				}
				if (Button(S2Hare::kPresets[i].name))
				{
					run_preset(st, S2Hare::kPresets[i].program);
				}
			}

			Separator();
			Checkbox("Show trail", &st.show_trail);
			Checkbox("Show axes", &st.show_axes);
			const char* sphere_modes[] = { "None", "Wireframe", "Transparent", "Solid" };
			Combo("Sphere display", &st.sphere_mode, sphere_modes, 4);
			Checkbox("Orthographic view", &st.orthographic);

			Separator();
			if (!st.last_error.empty())
			{
				PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
				TextUnformatted(st.last_error.c_str());
				PopStyleColor();
			}
			else
			{
				const LL ll = st.creature.pos.ll();
				TextFormatted("Position: lat {: .1f}\u00B0, lon {: .1f}\u00B0",
					static_cast<double>(ToDeg(ll.lat)), static_cast<double>(ToDeg(ll.lon)));
				TextFormatted("Step: {}/{}   Pen: {}   Trail: {} pts",
					st.pc, st.steps.size(),
					st.creature.pen_down ? "down" : "up", st.creature.trail.size());
				if (st.running)
				{
					TextUnformatted("Running...");
				}
			}
			Separator();
			TextUnformatted("Drag sphere to rotate view \u00B7 ESC to exit");
		}
		End();

		if (st.file_dialog_open)
		{
			show_file_dialog(st);
		}
		show_about(st);

		rlImGuiEnd();
		EndDrawing();
	}

	surface.Unload();
	S2App::ShutdownGuiApp();
	return EXIT_SUCCESS;
}
