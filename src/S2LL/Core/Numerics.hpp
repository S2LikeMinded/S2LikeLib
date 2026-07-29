#pragma once

// References:
// Dekker, T. J. (1971). A floating-point technique for extending the available precision. Numerische Mathematik, 18(3), 224-242. https://doi.org/10.1007/BF01397083
// Shewchuk, J. R. (1997). Adaptive precision floating-point arithmetic and fast robust geometric predicates. Discrete & Computational Geometry, 18(3), 305-363. https://doi.org/10.1007/PL00009321
// Thall, A. (2006). Extended-precision floating-point numbers for GPU computation. ACM SIGGRAPH 2006 Research Posters, 52-es. https://doi.org/10.1145/1179622.1179682
// Lu, M., He, B., Luo, Q., Ailamaki, A., & Boncz, P. A. (2010). Supporting extended precision on graphics processors. DaMoN '10, 1869389.1869392. https://doi.org/10.1145/1869389.1869392

#include <array>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <limits>
#include <numbers>
#include <type_traits>
#include <utility>
#include <S2LL/Core/Utilities.hpp>

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

		// Double-double equality if and only if both components are equal
		friend constexpr bool operator==(const Double& a, const Double& b) noexcept
		{
			return a.hi == b.hi && a.lo == b.lo;
		}

		// Double-double strictly-less-than ordering comparison
		friend constexpr bool operator<(const Double& a, const Double& b) noexcept
		{
			return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
		}

		// Double-double strictly-greater-than ordering comparison
		friend constexpr bool operator>(const Double& a, const Double& b) noexcept
		{
			return a.hi > b.hi || (a.hi == b.hi && a.lo > b.lo);
		}

		// Double-double less-than-or-equal ordering comparison
		friend constexpr bool operator<=(const Double& a, const Double& b) noexcept
		{
			return a.hi < b.hi || (a.hi == b.hi && a.lo <= b.lo);
		}

		// Double-double greater-than-or-equal ordering comparison
		friend constexpr bool operator>=(const Double& a, const Double& b) noexcept
		{
			return a.hi > b.hi || (a.hi == b.hi && a.lo >= b.lo);
		}

		// Double-double inequality if and only if some component is different
		friend constexpr bool operator!=(const Double& a, const Double& b) noexcept
		{
			return a.hi != b.hi || a.lo != b.lo;
		}

		// Unary negation operator
		constexpr Double operator-() const noexcept
		{
			return Double::make(-hi, -lo);
		}

		// Checks if either component is NaN
		inline bool isnan() const noexcept
		{
			return std::isnan(hi) || std::isnan(lo);
		}

		// Checks if either component is infinite
		inline bool isinf() const noexcept
		{
			return std::isinf(hi) || std::isinf(lo);
		}

		// Checks if the high component is negative
		inline bool isneg() const noexcept
		{
			return hi < 0.0;
		}

		// Checks if both components are zero
		inline bool iszero() const noexcept
		{
			return hi == 0.0 && lo == 0.0;
		}

		// Quick-Two-Sum algorithm (Shewchuk 1997 / Thall 2006)
		// Prerequisite: |a| >= |b|
		static constexpr Double quickTwoSum(double a, double b) noexcept
		{
			assert((a >= 0.0 ? a : -a) >= (b >= 0.0 ? b : -b));
			double s = a + b;
			double e = b - (s - a);
			return Double::make(s, e);
		}

		// Two-Sum algorithm (Thall 2006)
		static constexpr Double twoSum(double a, double b) noexcept
		{
			double s = a + b;
			double v = s - a;
			double e = (a - (s - v)) + (b - v);
			return Double::make(s, e);
		}

		// Quiet NaN representation
		static const Double NaN;

		// Zero representation
		static const Double Zero;

		// One representation
		static const Double One;

		// Pi representation
		static const Double Pi;

		// 1 degree, expressed in radians
		static const Double Degree;

		// 1 minute, expressed in radians
		static const Double Minute;

		// 1 second, expressed in radians
		static const Double Second;
	};

	inline const Double Double::NaN{
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN()
	};

	inline const Double Double::Zero{ 0.0, 0.0 };

	inline const Double Double::One{ 1.0, 0.0 };

	inline const Double Double::Pi{
		std::numbers::pi,
		1.2246467991473532e-16
	};

	// Compile-time POD & layout verification
	S2LL_ASSERT_POD(Double);

	// Lift helper to convert scalar or Double to Double
	template <typename T>
	constexpr Double Lift(const T& x) noexcept
	{
		if constexpr (std::is_same_v<std::decay_t<T>, Double>)
		{
			return x;
		}
		else
		{
			return Double::make(static_cast<double>(x), 0.0);
		}
	}

	// Double-double addition
	inline Double add(const Double& a, const Double& b)
	{
		Double s = Double::twoSum(a.hi, b.hi);
		return Double::make(s.hi, s.lo + a.lo + b.lo);
	}

	// Double-double subtraction
	inline Double sub(const Double& a, const Double& b)
	{
		double d = a.hi - b.hi;
		double q_virt = a.hi - d;
		double d_lo = q_virt - b.hi;
		return Double::make(d, (d_lo - b.lo) + a.lo);
	}

	// Double-double multiplication
	inline Double mul(const Double& a, const Double& b)
	{
		double p_hi = a.hi * b.hi;
		double p_lo = std::fma(a.hi, b.hi, -p_hi);
		p_lo += a.hi * b.lo + a.lo * b.hi;
		return Double::quickTwoSum(p_hi, p_lo);
	}

	// Double-double division
	inline Double div(const Double& a, const Double& b)
	{
		double q_hi = a.hi / b.hi;
		double p_hi = q_hi * b.hi;
		double p_lo = std::fma(q_hi, b.hi, -p_hi);
		double q_lo = (((a.hi - p_hi) - p_lo) + a.lo - q_hi * b.lo) / b.hi;
		return Double::quickTwoSum(q_hi, q_lo);
	}

	// Generic function overloads lifting double parameters to Double
#define S2LL_LIFT_BINOP(op) \
	template <typename A, typename B, \
		typename = std::enable_if_t<std::is_same_v<std::decay_t<A>, double> || std::is_same_v<std::decay_t<B>, double>>> \
	inline Double op(const A& a, const B& b) \
	{ \
		return ::S2LL::op(static_cast<Double>(Lift(a)), static_cast<Double>(Lift(b))); \
	}

	S2LL_LIFT_BINOP(add)
	S2LL_LIFT_BINOP(sub)
	S2LL_LIFT_BINOP(mul)
	S2LL_LIFT_BINOP(div)
#undef S2LL_LIFT_BINOP

	// Binary arithmetic operators (+, -, *, /) for S2LL::Double and scalars
	inline Double operator+(const Double& a, const Double& b) { return add(a, b); }
	inline Double operator-(const Double& a, const Double& b) { return sub(a, b); }
	inline Double operator*(const Double& a, const Double& b) { return mul(a, b); }
	inline Double operator/(const Double& a, const Double& b) { return div(a, b); }

#define S2LL_LIFT_BINOP_OPERATOR(op_symbol, func_name) \
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>> \
	inline Double operator op_symbol(const Double& a, const T& b) { return func_name(a, Lift(b)); } \
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>> \
	inline Double operator op_symbol(const T& a, const Double& b) { return func_name(Lift(a), b); }

	S2LL_LIFT_BINOP_OPERATOR(+, add)
	S2LL_LIFT_BINOP_OPERATOR(-, sub)
	S2LL_LIFT_BINOP_OPERATOR(*, mul)
	S2LL_LIFT_BINOP_OPERATOR(/, div)
#undef S2LL_LIFT_BINOP_OPERATOR

	inline const Double Double::Degree = div(Double::Pi, 180.0);
	inline const Double Double::Minute = div(Double::Pi, 10800.0);
	inline const Double Double::Second = div(Double::Pi, 648000.0);

	// Quadrant snapping for Double
	inline Double snap_quadrant(double x) noexcept
	{
		double hp = 0.5 * std::numbers::pi;
		int64_t k = static_cast<int64_t>(std::round(x / hp));
		if (x == static_cast<double>(k) * hp)
		{
			return mul(0.5 * k, Double::Pi);
		}
		return Double::make(x, 0.0);
	}

	// Double-double square (not square root!)
	inline Double sq(const Double& a)
	{
		double p_hi = a.hi * a.hi;
		double p_lo = std::fma(a.hi, a.hi, -p_hi);
		p_lo += 2.0 * a.hi * a.lo;
		return Double::quickTwoSum(p_hi, p_lo);
	}

	// Double-double square root
	inline Double sqrt(const Double& a)
	{
		if (a.isnan())
		{
			return Double::NaN;
		}
		if (a.isneg())
		{
			std::feraiseexcept(FE_INVALID);
			return Double::NaN;
		}
		if (a.iszero() || a.isinf())
		{
			return a;
		}

		double xn = 1.0 / std::sqrt(a.hi);
		double yn = a.hi * xn;
		Double ynsq = sq(Double::make(yn));
		double d = sub(a, ynsq).hi;
		Double p = mul(0.5 * xn, d);
		return add(Double::make(yn), p);
	}

	// High-precision simultaneous double-double sine and cosine
	inline std::pair<Double, Double> sin_and_cos(const Double& a)
	{
		if (a.isnan())
		{
			return std::make_pair(Double::NaN, Double::NaN);
		}

		double s = std::sin(a.hi);
		double c = std::cos(a.hi);

		// First-order Taylor series correction using a.lo
		Double sin_a = add(Double::make(s), mul(a.lo, c));
		Double cos_a = sub(Double::make(c), mul(a.lo, s));

		return std::make_pair(sin_a, cos_a);
	}

#define S2LL_LIFT_UNOP(op) \
	template <typename T, typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, double>>> \
	inline Double op(const T& x) \
	{ \
		return ::S2LL::op(static_cast<Double>(Lift(x))); \
	}

	S2LL_LIFT_UNOP(sq)
	S2LL_LIFT_UNOP(sqrt)
#undef S2LL_LIFT_UNOP

	inline std::pair<Double, Double> sin_and_cos(double x)
	{
		return sin_and_cos(snap_quadrant(x));
	}

	namespace Literals
	{
		// S2LL::Double extended-precision literals
		inline Double operator"" _Pi(long double p) { return mul(static_cast<double>(p), Double::Pi); }
		inline Double operator"" _Pi(unsigned long long p) { return mul(static_cast<double>(p), Double::Pi); }

		inline Double operator"" _Deg(long double d) { return mul(static_cast<double>(d), Double::Degree); }
		inline Double operator"" _Deg(unsigned long long d) { return mul(static_cast<double>(d), Double::Degree); }

		inline Double operator"" _Min(long double m) { return mul(static_cast<double>(m), Double::Minute); }
		inline Double operator"" _Min(unsigned long long m) { return mul(static_cast<double>(m), Double::Minute); }

		inline Double operator"" _Sec(long double s) { return mul(static_cast<double>(s), Double::Second); }
		inline Double operator"" _Sec(unsigned long long s) { return mul(static_cast<double>(s), Double::Second); }

		// Built-in double literals
		inline double operator"" _pi(long double p) { return static_cast<double>(p) * std::numbers::pi; }
		inline double operator"" _pi(unsigned long long p) { return static_cast<double>(p) * std::numbers::pi; }

		inline double operator"" _deg(long double d) { return static_cast<double>(d) * (std::numbers::pi / 180.0); }
		inline double operator"" _deg(unsigned long long d) { return static_cast<double>(d) * (std::numbers::pi / 180.0); }

		inline double operator"" _min(long double m) { return static_cast<double>(m) * (std::numbers::pi / 10800.0); }
		inline double operator"" _min(unsigned long long m) { return static_cast<double>(m) * (std::numbers::pi / 10800.0); }

		inline double operator"" _sec(long double s) { return static_cast<double>(s) * (std::numbers::pi / 648000.0); }
		inline double operator"" _sec(unsigned long long s) { return static_cast<double>(s) * (std::numbers::pi / 648000.0); }
	}
}
