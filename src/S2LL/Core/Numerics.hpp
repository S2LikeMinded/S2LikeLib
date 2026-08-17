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

		/// Convert from double using static factory pattern
		static constexpr Double make(double hi, double lo = 0.0) noexcept
		{
			return Double{ hi, lo };
		}

		/// Conversion operator to double
		explicit constexpr operator double() const noexcept
		{
			return hi + lo;
		}

		/// Conversion operator to float
		explicit constexpr operator float() const noexcept
		{
			return static_cast<float>(hi + lo);
		}

		/// Assigns a plain double; the low (error) component is cleared,
		/// matching Lift's interpretation of a bare double as exact
		constexpr Double& operator=(double x) noexcept
		{
			hi = x;
			lo = 0.0;
			return *this;
		}

		/// Double-double equality if and only if both components are equal
		friend constexpr bool operator==(const Double& a, const Double& b) noexcept
		{
			return a.hi == b.hi && a.lo == b.lo;
		}

		/// Double-double strictly-less-than ordering comparison
		friend constexpr bool operator<(const Double& a, const Double& b) noexcept
		{
			return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
		}

		/// Double-double strictly-greater-than ordering comparison
		friend constexpr bool operator>(const Double& a, const Double& b) noexcept
		{
			return a.hi > b.hi || (a.hi == b.hi && a.lo > b.lo);
		}

		/// Double-double less-than-or-equal ordering comparison
		friend constexpr bool operator<=(const Double& a, const Double& b) noexcept
		{
			return a.hi < b.hi || (a.hi == b.hi && a.lo <= b.lo);
		}

		/// Double-double greater-than-or-equal ordering comparison
		friend constexpr bool operator>=(const Double& a, const Double& b) noexcept
		{
			return a.hi > b.hi || (a.hi == b.hi && a.lo >= b.lo);
		}

		/// Double-double inequality if and only if some component is different
		friend constexpr bool operator!=(const Double& a, const Double& b) noexcept
		{
			return a.hi != b.hi || a.lo != b.lo;
		}

		/// Unary negation operator
		constexpr Double operator-() const noexcept
		{
			return Double::make(-hi, -lo);
		}

		/// Checks if either component is NaN
		inline bool isnan() const noexcept
		{
			return std::isnan(hi) || std::isnan(lo);
		}

		/// Checks if either component is infinite
		inline bool isinf() const noexcept
		{
			return std::isinf(hi) || std::isinf(lo);
		}

		/// Checks if the high component is negative
		inline bool isneg() const noexcept
		{
			return hi < 0.0;
		}

		/// Checks if both components are zero
		inline bool iszero() const noexcept
		{
			return hi == 0.0 && lo == 0.0;
		}

		/// Absolute value: the magnitude of the extended-precision value
		constexpr Double abs() const noexcept
		{
			return isneg() ? -(*this) : *this;
		}

		/// \brief Quick-Two-Sum algorithm (Shewchuk 1997 / Thall 2006)
		/// \details Prerequisite: \f$|a| \ge |b|\f$
		static constexpr Double quickTwoSum(double a, double b) noexcept
		{
			assert((a >= 0.0 ? a : -a) >= (b >= 0.0 ? b : -b));
			double s = a + b;
			double e = b - (s - a);
			return Double::make(s, e);
		}

		/// Two-Sum algorithm (Thall 2006)
		static constexpr Double twoSum(double a, double b) noexcept
		{
			double s = a + b;
			double v = s - a;
			double e = (a - (s - v)) + (b - v);
			return Double::make(s, e);
		}

		/// Quiet NaN representation
		static const Double NaN;

		/// Zero representation
		static const Double Zero;

		/// One representation
		static const Double One;

		/// Negative one representation
		static const Double NegOne;

		/// Pi representation
		static const Double Pi;

		/// 1 radian, expressed in degrees
		static const Double Radians;

		/// 1 degree, expressed in radians
		static const Double Degrees;

		/// 1 minute, expressed in radians
		static const Double Minutes;

		/// 1 second, expressed in radians
		static const Double Seconds;
	};

	inline const Double Double::NaN{
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN()
	};

	inline const Double Double::Zero{ 0.0, 0.0 };

	inline const Double Double::One{ 1.0, 0.0 };

	inline const Double Double::NegOne{ -1.0, 0.0 };

	inline const Double Double::Pi{
		std::numbers::pi,
		1.2246467991473532e-16
	};

	/// Compile-time POD & layout verification
	S2LL_ASSERT_POD(Double);

	/// Lift helper to convert scalar or Double to Double
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

	/// Double-double addition
	inline Double Add(const Double& a, const Double& b)
	{
		Double s = Double::twoSum(a.hi, b.hi);
		return Double::make(s.hi, s.lo + a.lo + b.lo);
	}

	/// Double-double subtraction
	inline Double Sub(const Double& a, const Double& b)
	{
		double d = a.hi - b.hi;
		double q_virt = a.hi - d;
		double d_lo = q_virt - b.hi;
		return Double::make(d, (d_lo - b.lo) + a.lo);
	}

	/// Double-double multiplication
	inline Double Mul(const Double& a, const Double& b)
	{
		double p_hi = a.hi * b.hi;
		double p_lo = std::fma(a.hi, b.hi, -p_hi);
		p_lo += a.hi * b.lo + a.lo * b.hi;
		return Double::quickTwoSum(p_hi, p_lo);
	}

	/// Double-double division
	inline Double Div(const Double& a, const Double& b)
	{
		double q_hi = a.hi / b.hi;
		double p_hi = q_hi * b.hi;
		double p_lo = std::fma(q_hi, b.hi, -p_hi);
		double q_lo = (((a.hi - p_hi) - p_lo) + a.lo - q_hi * b.lo) / b.hi;
		return Double::quickTwoSum(q_hi, q_lo);
	}

	/// Double-double two-argument arctangent (Atan2)
	inline Double Atan2(const Double& y, const Double& x)
	{
		if (y.isnan() || x.isnan())
		{
			return Double::NaN;
		}
		if (y == Double::Zero && x == Double::Zero)
		{
			return Double::Zero;
		}

		double y0 = std::atan2(static_cast<double>(y), static_cast<double>(x));
		double r2 = x.hi * x.hi + y.hi * y.hi;
		if (r2 == 0.0)
		{
			return Double::make(y0);
		}

		double dy = (x.hi * y.lo - y.hi * x.lo) / r2;
		return Add(Double::make(y0), Double::make(dy));
	}

	/// Function overloads lifting scalar operands to Double. At least one
	/// operand must be arithmetic (the other may be Double); Lift performs
	/// the conversion so the computation stays in extended precision.
	template <typename A, typename B,
		typename = std::enable_if_t<std::is_arithmetic_v<A> || std::is_arithmetic_v<B>>>
	inline Double Add(const A& a, const B& b) { return Add(Lift(a), Lift(b)); }

	template <typename A, typename B,
		typename = std::enable_if_t<std::is_arithmetic_v<A> || std::is_arithmetic_v<B>>>
	inline Double Sub(const A& a, const B& b) { return Sub(Lift(a), Lift(b)); }

	template <typename A, typename B,
		typename = std::enable_if_t<std::is_arithmetic_v<A> || std::is_arithmetic_v<B>>>
	inline Double Mul(const A& a, const B& b) { return Mul(Lift(a), Lift(b)); }

	template <typename A, typename B,
		typename = std::enable_if_t<std::is_arithmetic_v<A> || std::is_arithmetic_v<B>>>
	inline Double Div(const A& a, const B& b) { return Div(Lift(a), Lift(b)); }

	template <typename A, typename B,
		typename = std::enable_if_t<std::is_arithmetic_v<A> || std::is_arithmetic_v<B>>>
	inline Double Atan2(const A& y, const B& x) { return Atan2(Lift(y), Lift(x)); }

	/// Binary arithmetic operators (+, -, *, /) for S2LL::Double and scalars
	inline Double operator+(const Double& a, const Double& b) { return Add(a, b); }
	inline Double operator-(const Double& a, const Double& b) { return Sub(a, b); }
	inline Double operator*(const Double& a, const Double& b) { return Mul(a, b); }
	inline Double operator/(const Double& a, const Double& b) { return Div(a, b); }

	/// Operators for S2LL::Double and scalar operands; the scalar is lifted
	/// to Double so the operation completes in extended precision.
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double operator+(const Double& a, const T& b) { return Add(a, Lift(b)); }
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double operator-(const Double& a, const T& b) { return Sub(a, Lift(b)); }
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double operator*(const Double& a, const T& b) { return Mul(a, Lift(b)); }
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double operator/(const Double& a, const T& b) { return Div(a, Lift(b)); }

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double operator+(const T& a, const Double& b) { return Add(Lift(a), b); }
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double operator-(const T& a, const Double& b) { return Sub(Lift(a), b); }
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double operator*(const T& a, const Double& b) { return Mul(Lift(a), b); }
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double operator/(const T& a, const Double& b) { return Div(Lift(a), b); }

	inline const Double Double::Degrees = Div(Double::Pi, 180.0);
	inline const Double Double::Radians = Div(180.0, Double::Pi);
	inline const Double Double::Minutes = Div(Double::Pi, 10800.0);
	inline const Double Double::Seconds = Div(Double::Pi, 648000.0);

	/// Converts degrees to radians in extended precision.
	inline Double FromDeg(const Double& deg) { return Mul(deg, Double::Degrees); }

	/// Converts radians to degrees in extended precision.
	inline Double ToDeg(const Double& rad) { return Mul(rad, Double::Radians); }

	/// Scalar-lifting overloads, mirroring Lift.
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double FromDeg(const T& deg) { return FromDeg(Lift(deg)); }

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double ToDeg(const T& rad) { return ToDeg(Lift(rad)); }

	/// Quadrant snapping for Double
	inline Double SnapQuadrant(double x) noexcept
	{
		double hp = 0.5 * std::numbers::pi;
		int64_t k = static_cast<int64_t>(std::round(x / hp));
		if (x == static_cast<double>(k) * hp)
		{
			return Mul(0.5 * k, Double::Pi);
		}
		return Double::make(x, 0.0);
	}

	/// Double-double square (not square root!)
	inline Double Sq(const Double& a)
	{
		double p_hi = a.hi * a.hi;
		double p_lo = std::fma(a.hi, a.hi, -p_hi);
		p_lo += 2.0 * a.hi * a.lo;
		return Double::quickTwoSum(p_hi, p_lo);
	}

	/// Double-double square root
	inline Double Sqrt(const Double& a)
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
		Double ynsq = Sq(Double::make(yn));
		double d = Sub(a, ynsq).hi;
		Double p = Mul(0.5 * xn, d);
		return Add(Double::make(yn), p);
	}

	/// High-precision simultaneous double-double sine and cosine
	inline std::pair<Double, Double> SinCos(const Double& a)
	{
		if (a.isnan())
		{
			return std::make_pair(Double::NaN, Double::NaN);
		}

		double s = std::sin(a.hi);
		double c = std::cos(a.hi);

		// First-order Taylor series correction using a.lo
		Double sin_a = Add(Double::make(s), Mul(a.lo, c));
		Double cos_a = Sub(Double::make(c), Mul(a.lo, s));

		return std::make_pair(sin_a, cos_a);
	}

	/// Double-double inverse cosine (Acos)
	inline Double Acos(const Double& a)
	{
		if (a.isnan())
		{
			return Double::NaN;
		}
		if (a > Double::One || a < Double::NegOne)
		{
			std::feraiseexcept(FE_INVALID);
			return Double::NaN;
		}

		double y0 = std::acos(static_cast<double>(a));
		double s = 1.0 - a.hi * a.hi;
		if (s <= 0.0)
		{
			return Double::make(y0);
		}

		double dy = -a.lo / std::sqrt(s);
		return Add(Double::make(y0), Double::make(dy));
	}

	/// Ceiling: the smallest integer-valued Double not less than a.
	inline Double Ceil(const Double& a)
	{
		double c = std::ceil(a.hi);
		Double r = Sub(a, Double::make(c));
		if (r > Double::Zero)
		{
			return Add(Double::make(c), Double::One);
		}
		return Double::make(c);
	}

	/// Floor: the greatest integer-valued Double not greater than a.
	inline Double Floor(const Double& a)
	{
		double c = std::floor(a.hi);
		Double r = Sub(a, Double::make(c));
		if (r < Double::Zero)
		{
			return Sub(Double::make(c), Double::One);
		}
		return Double::make(c);
	}

	/// Unary function overloads lifting scalar parameters to Double.
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double Sq(const T& x) { return Sq(Lift(x)); }

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double Sqrt(const T& x) { return Sqrt(Lift(x)); }

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double Acos(const T& x) { return Acos(Lift(x)); }

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double Ceil(const T& x) { return Ceil(Lift(x)); }

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	inline Double Floor(const T& x) { return Floor(Lift(x)); }

	inline std::pair<Double, Double> SinCos(double x)
	{
		return SinCos(SnapQuadrant(x));
	}

	namespace Literals
	{
		/// S2LL::Double extended-precision literals
		inline Double operator""_Pi(long double p) { return Mul(static_cast<double>(p), Double::Pi); }
		inline Double operator""_Pi(unsigned long long p) { return Mul(static_cast<double>(p), Double::Pi); }

		inline Double operator""_Deg(long double d) { return Mul(static_cast<double>(d), Double::Degrees); }
		inline Double operator""_Deg(unsigned long long d) { return Mul(static_cast<double>(d), Double::Degrees); }

		inline Double operator""_Min(long double m) { return Mul(static_cast<double>(m), Double::Minutes); }
		inline Double operator""_Min(unsigned long long m) { return Mul(static_cast<double>(m), Double::Minutes); }

		inline Double operator""_Sec(long double s) { return Mul(static_cast<double>(s), Double::Seconds); }
		inline Double operator""_Sec(unsigned long long s) { return Mul(static_cast<double>(s), Double::Seconds); }

		/// Built-in double literals
		inline double operator""_pi(long double p) { return static_cast<double>(p) * std::numbers::pi; }
		inline double operator""_pi(unsigned long long p) { return static_cast<double>(p) * std::numbers::pi; }

		inline double operator""_deg(long double d) { return static_cast<double>(d) * (std::numbers::pi / 180.0); }
		inline double operator""_deg(unsigned long long d) { return static_cast<double>(d) * (std::numbers::pi / 180.0); }

		inline double operator""_min(long double m) { return static_cast<double>(m) * (std::numbers::pi / 10800.0); }
		inline double operator""_min(unsigned long long m) { return static_cast<double>(m) * (std::numbers::pi / 10800.0); }

		inline double operator""_sec(long double s) { return static_cast<double>(s) * (std::numbers::pi / 648000.0); }
		inline double operator""_sec(unsigned long long s) { return static_cast<double>(s) * (std::numbers::pi / 648000.0); }
	}

	/// Import symbols related to extended-precision computation.
	namespace Numerics
	{
		using ::S2LL::Double;
		using ::S2LL::Lift;
		using ::S2LL::Add;
		using ::S2LL::Sub;
		using ::S2LL::Mul;
		using ::S2LL::Div;
		using ::S2LL::FromDeg;
		using ::S2LL::ToDeg;
		using ::S2LL::Atan2;
		using ::S2LL::Sq;
		using ::S2LL::Sqrt;
		using ::S2LL::SinCos;
		using ::S2LL::Acos;
		using ::S2LL::Ceil;
		using ::S2LL::Floor;
		using ::S2LL::SnapQuadrant;
		using ::S2LL::operator+;
		using ::S2LL::operator-;
		using ::S2LL::operator*;
		using ::S2LL::operator/;
		using namespace ::S2LL::Literals;
	}
}
