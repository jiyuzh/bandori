#pragma once

/*
Magic: Defer execution of code
It has a similar semantic as JTC1/SC22/WG14 N2895 or scope(exit) in D

It relies on several GCC specific features:
1. Nested function
2. Nested function declaration (with auto keyword)
3. __attribute__((cleanup))
4. _Pragma (so that the kernel compiler can shut up)

Facts:
1. According to GCC, the defers are called in reverse order as definition.
2. According to GCC, variables in defer are "captured" by reference. The scope is transparent (like an if but with fancy execution order).
3. According to test, defers not yet declared (in execution order) will never be called.
4. According to test, defers are well optimized - it has no overhead when compared to Linux goto style under GCC O2.

*/

#define _DEFER_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define _DEFER_MERGE(a,b) a##b
#define _DEFER_VARNAME(a) _DEFER_MERGE(____defer_scopevar_, a)
#define _DEFER_FUNCNAME(a) _DEFER_MERGE(____defer_scopefunc_, a)
#define _DEFER_WRAPNAME(a) _DEFER_MERGE(____defer_scopewrap_, a)
#define _DEFER_PRAGMA(x) _Pragma (#x)
#define _DEFER(cond, n) \
	_DEFER_PRAGMA(GCC diagnostic push) \
	_DEFER_PRAGMA(GCC diagnostic ignored "-Wdeclaration-after-statement") \
	auto void _DEFER_FUNCNAME(n)(int *a); \
	void _DEFER_WRAPNAME(n)(int *a) { if(cond) { _DEFER_FUNCNAME(n)(a); } } \
	__attribute__((cleanup(_DEFER_WRAPNAME(n)))) int _DEFER_VARNAME(n); \
	_DEFER_PRAGMA(GCC diagnostic pop) void _DEFER_FUNCNAME(n)(int *a)

/// @brief Defer the exection to the end of the scope.
/// @attention This is not a closure. Be cautious of mutable variables.
#define defer _DEFER(1, __COUNTER__)

/// @brief Defer the exection to the end of the scope conditionally.
/// @param cond Run the defer if this evaluates to true.
/// @attention This is not a closure. Be cautious of mutable variables.
#define defer_on(cond) _DEFER(_DEFER_UNLIKELY(cond), __COUNTER__)
