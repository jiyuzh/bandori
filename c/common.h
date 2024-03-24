#pragma once

#define KB(x) (1024UL * (x))
#define MB(x) (1024UL * KB(x))
#define GB(x) (1024UL * MB(x))
#define TB(x) (1024UL * GB(x))
#define PB(x) (1024UL * TB(x))

#define fn static inline
#define let static const
#define mut static

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif
