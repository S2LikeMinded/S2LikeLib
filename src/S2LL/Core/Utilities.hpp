#pragma once

#include <numbers>
#include <type_traits>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#	define S2LL_WIN_OS
#elif __APPLE__
#	define S2LL_MAC_OS
#else
#	error "Compiler not suppoted"
#endif

// Single-line compile-time POD & layout verification macro
#if __cplusplus < 202002L && (!defined(_MSVC_LANG) || _MSVC_LANG < 202002L)
#	define S2LL_ASSERT_POD(T) \
		static_assert(std::is_pod_v<T>, #T " must be a POD type."); \
		static_assert(std::is_standard_layout_v<T>, #T " must have standard layout."); \
		static_assert(std::is_trivially_copyable_v<T>, #T " must be trivially copyable.")
#else
#	define S2LL_ASSERT_POD(T) \
		static_assert(std::is_standard_layout_v<T>, #T " must have standard layout."); \
		static_assert(std::is_trivially_copyable_v<T>, #T " must be trivially copyable.")
#endif
