#ifndef PLATONIX_COMMON_HPP
#define PLATONIX_COMMON_HPP

#include <cassert>

#ifndef __has_builtin
	#define __has_builtin(x) 0
#endif

#ifndef __has_attribute
	#define __has_attribute(x) 0
#endif

/// Define unreachable()

#if __has_builtin(__builtin_unreachable)
	#define unreachable() __builtin_unreachable()
#elif __has_attribute(unreachable)
	// Include a trailing semicolon so the error isn't incomprehensible if it's forgotten
	#define unreachable() [[unreachable]];
#else
	#ifdef NDEBUG
		#define unreachable()
	#else
		#define unreachable() do { assert("unreachable" && 0); throw 0; } while(0);
	#endif
#endif

#endif
