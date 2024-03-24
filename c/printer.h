#pragma once

#include "kernel.h"

#define pr(fmt, ...) do { \
	if (__builtin_constant_p(fmt)) \
		pr_warn(fmt "%s(at %s %s:%d)\n", ##__VA_ARGS__, \
			fmt[sizeof(fmt) - 2] == '\n' ? "    " : " ", __FUNCTION__ , __FILE__, __LINE__); \
	else {\
		pr_warn(fmt, ##__VA_ARGS__); \
		pr_warn("\t(at %s %s:%d)\n", __FUNCTION__, __FILE__, __LINE__); \
	} \
} while(0)
