#include "Hare.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace S2Hare
{
	using namespace S2LL::Numerics;

	namespace
	{
		/// Max arc step between trail samples (~1.1 degrees), keeps curves smooth
		constexpr double kArcStep = 0.02;

		/// Upper bound on the expanded (repeat-free) program length
		constexpr std::size_t kMaxSteps = 1'000'000;

		/// Projects h onto the tangent plane at p and normalizes it
		inline E3 orthogonalize(const E3& p, E3 h)
		{
			// Assumes that the surface is a sphere.
			h = h - p * p.dot(h);
			h.normalize();
			return h;
		}

		/// Linear combination a*w + b*v. Every product and sum completes in
		/// extended precision; each component is converted to double last.
		inline E3 weighted_sum(const E3& a, const E3& b, const Double& w, const Double& v)
		{
			// TODO: Should E3 support an extended precision type?
			return E3{
				static_cast<double>(a.x * w + b.x * v),
				static_cast<double>(a.y * w + b.y * v),
				static_cast<double>(a.z * w + b.z * v)
			};
		}
	}

	void CreatureState::forward(Double rad)
	{
		if (rad.iszero()) return;

		const E3 p0 = pos;
		const E3 h0 = heading;
		const int n = std::clamp(
			static_cast<int>(static_cast<double>(Ceil(rad.abs() / kArcStep))), 1, 512);

		// Sample the great-circle arc; recompute each sample from the origin
		// so accumulated round-off cannot bend the path.
		for (int i = 1; i <= n; ++i)
		{
			const Double t = rad * (Double::make(i) / n);
			const auto [sin_t, cos_t] = SinCos(t);
			pos = weighted_sum(p0, h0, cos_t, sin_t);
			heading = weighted_sum(h0, p0, cos_t, sin_t);
			if (pen_down)
			{
				trail.push_back(pos);
			}
		}
		pos.normalize();
		heading = orthogonalize(pos, heading);
	}

	void CreatureState::turn(Double rad)
	{
		const E3 outwards = pos.normalized();
		const E3 left_side = outwards.cross(heading);
		const auto [sin_phi, cos_phi] = SinCos(rad);
		heading = weighted_sum(heading, left_side, cos_phi, sin_phi);
		heading = orthogonalize(pos, heading);
	}

	void CreatureState::home() noexcept
	{
		pos = E3{ 0.0, 0.0, 1.0 };
		heading = E3{ 1.0, 0.0, 0.0 };
	}

	void CreatureState::clear() noexcept
	{
		trail.clear();
	}

	void CreatureState::step(const Step& s)
	{
		switch (s.type)
		{
		case Step::Type::Forward: forward(s.arg);   break;
		case Step::Type::Turn:    turn(s.arg);      break;
		case Step::Type::PenUp:   pen_down = false; break;
		case Step::Type::PenDown: pen_down = true;  break;
		case Step::Type::Home:    home();           break;
		case Step::Type::Clear:   clear();          break;
		}
	}

	namespace
	{
		bool parse_number(const std::string& w, double& v)
		{
			if (w.empty())
				return false;
			char* end = nullptr;
			v = std::strtod(w.c_str(), &end);
			return end != w.c_str() && *end == '\0';
		}

		std::vector<std::string> tokenize(const std::string& text)
		{
			std::vector<std::string> toks;
			std::string cur;
			for (char c : text)
			{
				if (c == '[' || c == ']')
				{
					if (!cur.empty())
					{
						toks.push_back(cur);
						cur.clear();
					}
					toks.push_back(std::string(1, c));
				}
				else if (std::isspace(static_cast<unsigned char>(c)))
				{
					if (!cur.empty())
					{
						toks.push_back(cur);
						cur.clear();
					}
				}
				else
				{
					cur.push_back(static_cast<char>(
						std::tolower(static_cast<unsigned char>(c))));
				}
			}
			if (!cur.empty())
			{
				toks.push_back(cur);
			}
			return toks;
		}

		bool parse_sequence(const std::vector<std::string>& toks, std::size_t& i,
			std::vector<Step>& out, std::string& err, int depth)
		{
			if (depth > 32)
			{
				err = "repeat nesting too deep (max 32)";
				return false;
			}

			while (i < toks.size())
			{
				const std::string& w = toks[i];
				if (w == "]")
				{
					return true;
				}

				auto require_number = [&](double& v) -> bool
				{
					++i;
					if (i >= toks.size() || !parse_number(toks[i], v))
					{
						err = "expected a number after '" + w + "'";
						return false;
					}
					return true;
				};

				if (w == "repeat")
				{
					++i;
					double count_d = 0.0;
					if (i >= toks.size() || !parse_number(toks[i], count_d))
					{
						err = "repeat: expected a count";
						return false;
					}
					const int count = static_cast<int>(std::llround(count_d));
					if (count < 0 || count > 100'000)
					{
						err = "repeat: count unsupported";
						return false;
					}
					++i;
					if (i >= toks.size() || toks[i] != "[")
					{
						err = "repeat: expected '['";
						return false;
					}
					++i;

					std::vector<Step> inner;
					if (!parse_sequence(toks, i, inner, err, depth + 1))
					{
						return false;
					}
					if (i >= toks.size() || toks[i] != "]")
					{
						err = "repeat: missing ']'";
						return false;
					}
					++i;

					for (int k = 0; k < count; ++k)
					{
						out.insert(out.end(), inner.begin(), inner.end());
						if (out.size() > kMaxSteps)
						{
							err = "program too long after expanding repeats";
							return false;
						}
					}
					continue;
				}

				double v = 0.0;
				if (w == "fd" || w == "forward" || w == "hop" || w == "fw")
				{
					if (!require_number(v))
					{
						return false;
					}
					out.push_back({ Step::Type::Forward, FromDeg(v) });
				}
				else if (w == "bk" || w == "back" || w == "backward")
				{
					if (!require_number(v))
					{
						return false;
					}
					out.push_back({ Step::Type::Forward, FromDeg(-v) });
				}
				else if (w == "rt" || w == "right" || w == "tr")
				{
					if (!require_number(v))
					{
						return false;
					}
					out.push_back({ Step::Type::Turn, FromDeg(-v) });
				}
				else if (w == "lt" || w == "left" || w == "tl")
				{
					if (!require_number(v))
					{
						return false;
					}
					out.push_back(Step{ Step::Type::Turn, FromDeg(v) });
				}
				else if (w == "pu" || w == "penup")
				{
					out.push_back(Step{ Step::Type::PenUp, Double::Zero });
				}
				else if (w == "pd" || w == "pendown")
				{
					out.push_back(Step{ Step::Type::PenDown, Double::Zero });
				}
				else if (w == "home")
				{
					out.push_back(Step{ Step::Type::Home, Double::Zero });
				}
				else if (w == "cs" || w == "clear")
				{
					out.push_back(Step{ Step::Type::Clear, Double::Zero });
				}
				else
				{
					err = "unknown command '" + w + "'";
					return false;
				}
				++i;
			}
			return true;
		}
	}

	bool parse_program(const std::string& text, std::vector<Step>& out, std::string& err)
	{
		const std::vector<std::string> toks = tokenize(text);
		std::size_t i = 0;
		std::vector<Step> steps;
		if (!parse_sequence(toks, i, steps, err, 0))
		{
			return false;
		}
		if (i < toks.size())
		{
			err = "unexpected ']'";
			return false;
		}
		out = std::move(steps);
		return true;
	}

	const Preset kPresets[] = {
		{ "Great Circle",       "fd 360" },
		{ "Pole to Pole",       "fd 180" },
		{ "Spherical Triangle", "repeat 3 [ fd 90 lt 90 ]" },
	};
	const std::size_t kPresetCount = std::size(kPresets);
}
