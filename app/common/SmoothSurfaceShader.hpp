#pragma once

#include <S2LL/Core/Surfaces.hpp>

#include <raylib.h>

#include <string>

namespace S2App
{
	/// Wrapper around the shared smooth_surface.{v,f}s shader pair: per-pixel
	/// lighting with normals derived from the surface quadric. Used for the
	/// solid/transparent sphere passes in the demo apps.
	///
	/// NOTE: raylib's UnloadMaterial() also unloads a shader attached to the
	/// material by value. If you assign shader() to a material, clear that
	/// copy (material.shader = Shader{}) before UnloadMaterial(), otherwise
	/// the shared locations array is freed twice.
	class SmoothSurfaceShader
	{
	public:
		~SmoothSurfaceShader() { Unload(); }

		SmoothSurfaceShader() = default;
		SmoothSurfaceShader(const SmoothSurfaceShader&) = delete;
		SmoothSurfaceShader& operator=(const SmoothSurfaceShader&) = delete;

		/// Loads the shader pair and caches the uniform locations. Also seeds
		/// the constant center (origin) and light direction (+z) uniforms.
		bool Load(const std::string& vs_path, const std::string& fs_path);

		void Unload() noexcept;
		bool valid() const noexcept;
		/// The raw raylib Shader (e.g. for attaching to a Material)
		const Shader& shader() const noexcept;

		void SetQuadric(const S2LL::BilinearForm& q);
		void SetViewPos(const Vector3& position);
		void SetAlpha(float alpha);
		void SetModel(const Matrix& model);

		/// BeginShaderMode / EndShaderMode around the draw calls
		void Begin() const;
		void End() const;

	private:
		Shader _shader{};
		struct Locs
		{
			int center;
			int quadric;
			int light_dir;
			int view_pos;
			int model;
			int alpha;
		} _locs{ -1, -1, -1, -1, -1, -1 };
	};
}
