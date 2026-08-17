#include <catch2/catch_test_macros.hpp>
#include <CatchDouble.hpp>
#include <S2LL/Core/Numerics.hpp>
#include <cmath>
#include <numbers>
#include <type_traits>

TEST_CASE("Angle Literals", "[core][numerics]") {
	using namespace S2LL::Literals;

	SECTION("Primitive double literals return double") {
		static_assert(std::is_same_v<decltype(180_deg), double>);
		static_assert(std::is_same_v<decltype(180.0_deg), double>);
		static_assert(std::is_same_v<decltype(60_min), double>);
		static_assert(std::is_same_v<decltype(60.0_min), double>);
		static_assert(std::is_same_v<decltype(3600_sec), double>);
		static_assert(std::is_same_v<decltype(3600.0_sec), double>);

		REQUIRE(180.0_deg == 1_pi);
		REQUIRE(-180.0_deg == -1_pi);
		REQUIRE(60.0_min == 1.0_deg);
		REQUIRE(3600.0_sec == 1.0_deg);
	}

	SECTION("Double literals return S2LL::Double") {
		static_assert(std::is_same_v<decltype(180_Deg), S2LL::Double>);
		static_assert(std::is_same_v<decltype(180.0_Deg), S2LL::Double>);
		static_assert(std::is_same_v<decltype(60_Min), S2LL::Double>);
		static_assert(std::is_same_v<decltype(60.0_Min), S2LL::Double>);
		static_assert(std::is_same_v<decltype(3600_Sec), S2LL::Double>);
		static_assert(std::is_same_v<decltype(3600.0_Sec), S2LL::Double>);
		
		REQUIRE(-30.0_Deg == -S2LL::Double::Pi / 6);
		REQUIRE(30.0_Deg == S2LL::Double::Pi / 6);
		REQUIRE(45.0_Deg == 0.25_Pi);
		REQUIRE(90.0_Deg == 0.5_Pi);
		REQUIRE(180.0_Deg == S2LL::Double::Pi);
		REQUIRE(360.0_Deg == 2_Pi);
		REQUIRE(-180.0_Deg == -S2LL::Double::Pi);
	}
}

TEST_CASE("Degree Conversions", "[core][numerics]") {
	using namespace S2LL;
	using namespace S2LL::Literals;

	static_assert(std::is_same_v<decltype(FromDeg(90.0)), S2LL::Double>);
	static_assert(std::is_same_v<decltype(ToDeg(90.0)), S2LL::Double>);
	static_assert(std::is_same_v<decltype(FromDeg(90.0_Deg)), S2LL::Double>);
	static_assert(std::is_same_v<decltype(ToDeg(90.0_Deg)), S2LL::Double>);

	// FromDeg(x) computes the same product as the x_Deg literal, so the
	// exact equalities mirror the "Angle Literals" test case.
	REQUIRE(FromDeg(0.0) == S2LL::Double::Zero);
	REQUIRE(FromDeg(30.0) == S2LL::Double::Pi / 6);
	REQUIRE(FromDeg(45.0) == 0.25_Pi);
	REQUIRE(FromDeg(90.0) == 0.5_Pi);
	REQUIRE(FromDeg(180.0) == S2LL::Double::Pi);
	REQUIRE(FromDeg(360.0) == 2_Pi);
	REQUIRE(FromDeg(-180.0) == -S2LL::Double::Pi);

	// ToDeg multiplies by the precomputed Radians constant (degrees per
	// radian), so these round-trips are exact: the double result is exactly
	// 180.0 / 90.0.
	REQUIRE(static_cast<double>(ToDeg(S2LL::Double::Pi)) == 180.0);
	REQUIRE(static_cast<double>(ToDeg(0.5 * S2LL::Double::Pi)) == 90.0);
	REQUIRE(static_cast<double>(ToDeg(FromDeg(90.0))) == 90.0);
	REQUIRE(static_cast<double>(FromDeg(ToDeg(90.0))) == 90.0);
}

TEST_CASE("Catch2 StringMaker for Double", "[core][numerics]") {
	S2LL::Double val{ 3.141592653589793, 1.2345e-17 };
	std::string repr = Catch::StringMaker<S2LL::Double>::convert(val);

	REQUIRE(repr.find("Double{ hi:") != std::string::npos);
	REQUIRE(repr.find("lo:") != std::string::npos);
}

TEST_CASE("Double Inverse Trigonometric Functions", "[core][numerics]") {
	using namespace S2LL::Literals;

	SECTION("S2LL::Acos bounds and domain errors") {
		REQUIRE(static_cast<double>(S2LL::Acos(S2LL::Double::One)) == 0.0);
		REQUIRE(static_cast<double>(S2LL::Acos(S2LL::Double::NegOne)) == 1_pi);
		REQUIRE(S2LL::Acos(S2LL::Double::make(1.5)).isnan());
		REQUIRE(S2LL::Acos(S2LL::Double::make(-1.5)).isnan());
	}

	SECTION("S2LL::Atan2 special values") {
		REQUIRE(static_cast<double>(S2LL::Atan2(S2LL::Double::Zero, S2LL::Double::Zero)) == 0.0);
		REQUIRE(static_cast<double>(S2LL::Atan2(S2LL::Double::One, S2LL::Double::Zero)) == 0.5_pi);
		REQUIRE(static_cast<double>(S2LL::Atan2(S2LL::Double::Zero, S2LL::Double::One)) == 0.0);
	}
}
