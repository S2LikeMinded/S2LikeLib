
#ifndef __UTILITIES_HPP__
#define __UTILITIES_HPP__

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#	define S2LL_WIN_OS
#elif __APPLE__
#	define S2LL_MAC_OS
#else
#	error "Compiler not suppoted"
#endif

#endif // !__UTILITIES_HPP__
