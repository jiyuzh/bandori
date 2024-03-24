#pragma once

#include "headers.h"
#include "common.h"
#include "kernel.h"

fn u64 get_current_ns(void)
{
	struct timespec spec;

	if (clock_gettime(CLOCK_MONOTONIC, &spec))
		return 0;

	return ((u64) spec.tv_sec) * NSEC_PER_SEC + ((u64) spec.tv_nsec);
}

#define ktime_get_ns() get_current_ns()
