#pragma once

#include "headers.h"
#include "common.h"
#include "kernel.h"

//
// TRNG
//

// Extract a secure random via RDSEED instruction.
fn u64 trng_u64_rdseed(void)
{
	u64 val = 0;

	__asm__ __volatile__(
		"1:\n"
		"rdseed %%rax\n"
		"jnc 1b\n"
		: "=a" (val)
		: "a" (val)
	);

	return val;
}

#define __RNDGETENTCNT _IOR('R', 0x00, int)

// Extract a secure random via /dev/random.
fn u64 trng_u64_dev(void)
{
	u64 val = 0;
	ssize_t ret = 0;
	ssize_t got = 0;
	int entropy = 0;
	int fd = open("/dev/random", O_RDONLY);

	if (unlikely(fd < 0))
		return 0; // Unabe to open device

	if (ioctl(fd, __RNDGETENTCNT, &entropy))
		return 0; // /dev/random is not a random device

	if (entropy < sizeof(val) * 8)
		return 0; // Not enough entropy

	while (got < sizeof(val))
	{
		ret = read(fd, ((u8*) &val) + got, sizeof(val) - got);

		if (ret < 0) {
			close(fd);
			return 0; // Read from device failed
		}

		got += ret;
	}

	close(fd);
	return val;
}

#define __CPUID_RDRND_MASK (1ULL << 30)

fn u64 (*__resolve_trng_u64(void))(void)
{
	unsigned int eax = 1;
	unsigned int ebx = 0;
	unsigned int ecx = 0;
	unsigned int edx = 0;

	__asm__ __volatile__(
		"cpuid"
		: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		: "a"(eax)
	);

	if (ecx & __CPUID_RDRND_MASK)
		return trng_u64_rdseed;

	return trng_u64_dev;
}

// Extract a secure random.
fn u64 trng_u64(void) __attribute__((ifunc("__resolve_trng_u64")));

//
// PRNG
//

// based on https://source.dot.net/#System.Private.CoreLib/src/libraries/System.Private.CoreLib/src/System/Random.Xoshiro256StarStarImpl.cs,2b61079099d6bb53

typedef struct prng_state {
	u64 s[4];
} prng_state;

mut __thread prng_state ___global_prng_state = { 0 };
mut __thread prng_state *__global_prng_state = NULL;

// Check if a random state is valid.
fn bool prng_valid(prng_state *state)
{
	return state->s[0] | state->s[1] | state->s[2] | state->s[3];
}

// Seed a random state, return true if seeded by secure random source.
fn bool prng_seed(prng_state *state)
{
	u64 iter = 0;

	do
	{
		state->s[0] = trng_u64();
		state->s[1] = trng_u64();
		state->s[2] = trng_u64();
		state->s[3] = trng_u64();

		if (unlikely(++iter > 32)) {
			u64 *heap = malloc(16);
			u64 val = 0x01234567ULL;

			if (heap) {
				val = *(u64 *) ((char *) heap + 3);
				free(heap);
			}

			pr_err("TRNG source unavailable, fallback to insecure seed\n");
			state->s[0] = rand() * val;
			state->s[1] = rand() + (u64) heap;
			state->s[2] = rand() ^ (u64) state;
			state->s[3] = __builtin_ia32_rdtsc();

			return false;
		}
	}
	while (unlikely(!prng_valid(state)));

	return true;
}

// Rotate left by k bits.
fn u64 __prng_rotl(u64 x, int k)
{
	return (x << k) | (x >> (64 - k));
}

// Get the global state shared by instance-less calls.
fn prng_state *__prng_global(void)
{
	if (likely(__global_prng_state))
		return __global_prng_state;

	prng_seed(&___global_prng_state);
	__global_prng_state = &___global_prng_state;
	return __global_prng_state;
}

// Produces a value in the range [0, u64.MaxValue].
// The caller may optionally provide a RNG state, or NULL to use global state.
fn u64 prng_u64(prng_state *state)
{
	u64 s0, s1, s2, s3, result, t;
	prng_state *current = state ? state : __prng_global();

	s0 = current->s[0];
	s1 = current->s[1];
	s2 = current->s[2];
	s3 = current->s[3];

	result = __prng_rotl(s1 * 5, 7) * 9;
	t = s1 << 17;

	s2 ^= s0;
	s3 ^= s1;
	s1 ^= s2;
	s0 ^= s3;

	s2 ^= t;
	s3 = __prng_rotl(s3, 45);

	current->s[0] = s0;
	current->s[1] = s1;
	current->s[2] = s2;
	current->s[3] = s3;

	return result;
}

// Produces a value in the range [0, u32.MaxValue].
// The caller may optionally provide a RNG state, or NULL to use global state.
fn u32 prng_u32(prng_state *state)
{
	return (u32) (prng_u64(state) >> 32);
}

// Fill the buffer with random bytes.
// The caller may optionally provide a RNG state, or NULL to use global state.
fn void prng_bytes(prng_state *state, u8 *buf, size_t len)
{
	u64 s0, s1, s2, s3, t;
	prng_state *current = state ? state : __prng_global();

	s0 = current->s[0];
	s1 = current->s[1];
	s2 = current->s[2];
	s3 = current->s[3];

	while (len >= sizeof(u64)) {
		u64 *result = (u64 *) buf;

		*result = __prng_rotl(s1 * 5, 7) * 9;
		t = s1 << 17;

		s2 ^= s0;
		s3 ^= s1;
		s1 ^= s2;
		s0 ^= s3;

		s2 ^= t;
		s3 = __prng_rotl(s3, 45);

		buf += sizeof(u64);
		len -= sizeof(u64);
	}

	if (len) {
		u64 result = __prng_rotl(s1 * 5, 7) * 9;
		u8 *bytes = (u8 *)&result;

		for (int i = 0; i < len; i++) {
			buf[i] = bytes[i];
		}

		t = s1 << 17;
		s2 ^= s0;
		s3 ^= s1;
		s1 ^= s2;
		s0 ^= s3;

		s2 ^= t;
		s3 = __prng_rotl(s3, 45);
	}

	current->s[0] = s0;
	current->s[1] = s1;
	current->s[2] = s2;
	current->s[3] = s3;
}

//
// Kernel compatibility
//

#define prandom_u32() prng_u32(NULL)
#define prandom_u64() prng_u64(NULL)
#define prandom_bytes(buf, bytes) prng_bytes(NULL, buf, bytes)
