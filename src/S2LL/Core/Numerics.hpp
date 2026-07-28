#pragma once

// References:
// Shewchuk, J. R. (1997). Adaptive precision floating-point arithmetic and fast robust geometric predicates. Discrete & Computational Geometry, 18(3), 305-363. https://doi.org/10.1007/PL00009321
// Thall, A. (2006). Extended-precision floating-point numbers for GPU computation. ACM SIGGRAPH 2006 Research Posters, 52-es. https://doi.org/10.1145/1179622.1179682
// Lu, M., He, B., Luo, Q., Ailamaki, A., & Boncz, P. A. (2010). Supporting extended precision on graphics processors. DaMoN '10, 1869389.1869392. https://doi.org/10.1145/1869389.1869392

#include <array>
#include <cmath>
#include <type_traits>
#include <utility>

namespace S2LL
{
	struct Double
	{
		double hi;
		double lo;

		// Convert from double using static factory pattern
		static constexpr Double make(double hi, double lo = 0.0) noexcept
		{
			return Double{ hi, lo };
		}

		// Conversion operator to double
		explicit constexpr operator double() const noexcept
		{
			return hi + lo;
		}
	};

	// Compile-time POD verification
	static_assert(std::is_pod_v<Double>, "Double must be a POD type.");
	static_assert(std::is_standard_layout_v<Double>, "Double must have standard layout.");
	static_assert(std::is_trivially_copyable_v<Double>, "Double must be trivially copyable.");

	// Lift helper to convert double to Double or pass-through Double
	template <typename T>
	constexpr decltype(auto) Lift(const T& x) noexcept
	{
		if constexpr (std::is_same_v<std::decay_t<T>, double>)
		{
			return Double::make(x, 0.0);
		}
		else
		{
			return x;
		}
	}

	// Double-double addition
	inline Double add(const Double& a, const Double& b)
	{
		double s = a.hi + b.hi;
		double a_virt = s - b.hi;
		double b_virt = s - a_virt;
		double a_err = a.hi - a_virt;
		double b_err = b.hi - b_virt;
		return Double::make(s, (a_err + b_err) + a.lo + b.lo);
	}

	// Double-double subtraction
	inline Double sub(const Double& a, const Double& b)
	{
		double d = a.hi - b.hi;
		double q_virt = a.hi - d;
		double d_lo = q_virt - b.hi;
		return Double::make(d, (d_lo - b.lo) + a.lo);
	}

	// Double-double division
	inline Double div(const Double& a, const Double& b)
	{
		double q_hi = a.hi / b.hi;
		double p_hi = q_hi * b.hi;
		double p_lo = std::fma(q_hi, b.hi, -p_hi);
		double q_lo = (((a.hi - p_hi) - p_lo) + a.lo - q_hi * b.lo) / b.hi;
		return Double::make(q_hi, q_lo);
	}

	// Generic overloads lifting double parameters to Double
	template <typename A, typename B,
		typename = std::enable_if_t<std::is_same_v<std::decay_t<A>, double> || std::is_same_v<std::decay_t<B>, double>>>
	inline Double add(const A& a, const B& b)
	{
		return add(Lift(a), Lift(b));
	}

	template <typename A, typename B,
		typename = std::enable_if_t<std::is_same_v<std::decay_t<A>, double> || std::is_same_v<std::decay_t<B>, double>>>
	inline Double sub(const A& a, const B& b)
	{
		return sub(Lift(a), Lift(b));
	}

	template <typename A, typename B,
		typename = std::enable_if_t<std::is_same_v<std::decay_t<A>, double> || std::is_same_v<std::decay_t<B>, double>>>
	inline Double div(const A& a, const B& b)
	{
		return div(Lift(a), Lift(b));
	}
}
