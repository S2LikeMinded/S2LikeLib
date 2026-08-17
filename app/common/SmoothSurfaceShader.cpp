#include "SmoothSurfaceShader.hpp"
#include "S2Raylib.hpp"

namespace S2App
{
	bool SmoothSurfaceShader::Load(const std::string& vs_path, const std::string& fs_path)
	{
		_shader = LoadShader(vs_path.c_str(), fs_path.c_str());
		if (_shader.id == 0)
		{
			return false;
		}
		_locs = Locs{
			GetShaderLocation(_shader, "uCenter"),
			GetShaderLocation(_shader, "uQuadric"),
			GetShaderLocation(_shader, "uLightDir"),
			GetShaderLocation(_shader, "uViewPos"),
			GetShaderLocation(_shader, "uModel"),
			GetShaderLocation(_shader, "uAlpha")
		};

		const float center[3] = { 0.0f, 0.0f, 0.0f };
		const float light_dir[3] = { 0.0f, 0.0f, 1.0f };
		SetShaderValue(_shader, _locs.center, center, SHADER_UNIFORM_VEC3);
		SetShaderValue(_shader, _locs.light_dir, light_dir, SHADER_UNIFORM_VEC3);
		return true;
	}

	void SmoothSurfaceShader::Unload() noexcept
	{
		if (_shader.id != 0)
		{
			UnloadShader(_shader);
			_shader = Shader{};
		}
	}

	bool SmoothSurfaceShader::valid() const noexcept
	{
		return _shader.id != 0;
	}

	const Shader& SmoothSurfaceShader::shader() const noexcept
	{
		return _shader;
	}

	void SmoothSurfaceShader::SetQuadric(const S2LL::BilinearForm& q)
	{
		SetShaderValueMatrix(_shader, _locs.quadric, to_quadric_matrix(q));
	}

	void SmoothSurfaceShader::SetViewPos(const Vector3& position)
	{
		const float view_pos[3] = { position.x, position.y, position.z };
		SetShaderValue(_shader, _locs.view_pos, view_pos, SHADER_UNIFORM_VEC3);
	}

	void SmoothSurfaceShader::SetAlpha(float alpha)
	{
		SetShaderValue(_shader, _locs.alpha, &alpha, SHADER_UNIFORM_FLOAT);
	}

	void SmoothSurfaceShader::SetModel(const Matrix& model)
	{
		SetShaderValueMatrix(_shader, _locs.model, model);
	}

	void SmoothSurfaceShader::Begin() const
	{
		BeginShaderMode(_shader);
	}

	void SmoothSurfaceShader::End() const
	{
		EndShaderMode();
	}
}
