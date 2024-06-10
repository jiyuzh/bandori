#pragma once

#define KB(x) (1024UL * (x))
#define MB(x) (1024UL * KB(x))
#define GB(x) (1024UL * MB(x))
#define TB(x) (1024UL * GB(x))
#define PB(x) (1024UL * TB(x))

#define Thousand(x) (1000UL * (x))
#define Million(x) (1000UL * Thousand(x))
#define Billion(x) (1000UL * Million(x))
#define Trillion(x) (1000UL * Billion(x))

#define fn static inline
#define let static const
#define mut static

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif
