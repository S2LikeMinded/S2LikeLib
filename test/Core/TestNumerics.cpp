#include <catch2/catch_test_macros.hpp>
#include <CatchDouble.hpp>
#include <S2LL/Core/Numerics.hpp>
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

TEST_CASE("Catch2 StringMaker for Double", "[core][numerics]") {
	S2LL::Double val{ 3.141592653589793, 1.2345e-17 };
	std::string repr = Catch::StringMaker<S2LL::Double>::convert(val);

	REQUIRE(repr.find("Double{ hi:") != std::string::npos);
	REQUIRE(repr.find("lo:") != std::string::npos);
}

TEST_CASE("Double Inverse Trigonometric Functions", "[core][numerics]") {
	using namespace S2LL::Literals;

	SECTION("S2LL::acos bounds and domain errors") {
		REQUIRE(static_cast<double>(S2LL::acos(S2LL::Double::One)) == 0.0);
		REQUIRE(static_cast<double>(S2LL::acos(S2LL::Double::NegOne)) == 1_pi);
		REQUIRE(S2LL::acos(S2LL::Double::make(1.5)).isnan());
		REQUIRE(S2LL::acos(S2LL::Double::make(-1.5)).isnan());
	}

	SECTION("S2LL::atan2 special values") {
		REQUIRE(static_cast<double>(S2LL::atan2(S2LL::Double::Zero, S2LL::Double::Zero)) == 0.0);
		REQUIRE(static_cast<double>(S2LL::atan2(S2LL::Double::One, S2LL::Double::Zero)) == 0.5_pi);
		REQUIRE(static_cast<double>(S2LL::atan2(S2LL::Double::Zero, S2LL::Double::One)) == 0.0);
	}
}
