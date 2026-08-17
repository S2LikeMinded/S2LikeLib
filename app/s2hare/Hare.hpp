#pragma once

#include <S2LL/Core/Coordinates.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace S2Hare
{
	using S2LL::E3;

	enum class Creature
	{
		Hare = 0,
		Tortoise = 1
	};

	/// A parsed step. The program text uses degrees, but `arg` is stored in
	/// radians: the parser converts once, so all internal math stays radians.
	///
	/// For example, `fd 360` indicates walking along a great circle, back
	/// to where it started (one full lap, i.e. 2*pi radians).
	struct Step
	{
		enum class Type
		{
			Forward,   ///< arg = signed arc length in radians (neg = backwards)
			Turn,      ///< arg = signed turn in radians (neg = right)
			PenUp,
			PenDown,
			Home,      ///< reset position and heading, keep the trail
			Clear      ///< clear the trail
		} type;

		S2LL::Double arg{};
	};

	/// The spherical creature: a position on the unit sphere plus a tangent
	/// heading. `forward` walks along the great circle through the heading;
	/// `turn` rotates the heading around the surface normal (left positive).
	class CreatureState
	{
	public:
		E3 pos{ 0.0, 0.0, 1.0 };      ///< position on the unit sphere
		E3 heading{ 1.0, 0.0, 0.0 };  ///< tangent direction, unit length
		bool pen_down = true;
		std::vector<E3> trail;        ///< unit-sphere positions drawn while pen down

		/// Moves along a great circle by a signed arc length in radians.
		void forward(S2LL::Double rad);

		/// Turns in place by a signed angle in radians (left positive).
		void turn(S2LL::Double rad);

		/// Resets position to the north pole and heading to +x, keeping the trail.
		void home() noexcept;

		/// Removes all recorded trail points.
		void clear() noexcept;

		/// Executes a single parsed step.
		void step(const Step& s);
	};

	/// Parses a Logo-like program (fd/bk/rt/lt/pu/pd/home/cs/repeat N [ ... ]).
	/// Repeats are expanded at parse time. Returns false and fills `err` on
	/// failure; on success `out` receives the flat step list.
	bool parse_program(const std::string& text, std::vector<Step>& out, std::string& err);

	/// Existing recipes for moving the creature.
	struct Preset
	{
		const char* name;     ///< button label
		const char* program;  ///< Logo-like program text in degrees
	};

	/// The built-in presets; iterate up to kPresetCount entries.
	extern const Preset kPresets[];

	/// Number of entries in kPresets.
	extern const std::size_t kPresetCount;
}
