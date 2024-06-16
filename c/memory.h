#pragma once

#include "headers.h"
#include "common.h"
#include "kernel.h"
#include "random.h"
#include "interactive.h"
#include "printer.h"

//
// Private Anon MMAP
//

typedef struct mmap_alloc_handle {
	void *map;
	size_t size;
} mmap_alloc_handle;

let mmap_alloc_handle default_mmap_alloc_handle = { 0 };

fn void mmap_free(mmap_alloc_handle *handle)
{
	if (handle && (handle->map != MAP_FAILED) && handle->size) {
		munmap(handle->map, handle->size);
	}

	if (handle) {
		*handle = default_mmap_alloc_handle;
		free(handle);
	}
}

fn mmap_alloc_handle *mmap_alloc(size_t size)
{
	mmap_alloc_handle *handle = malloc(sizeof(mmap_alloc_handle));

	if (!handle)
		return NULL;

	*handle = default_mmap_alloc_handle;

	handle->size = size;
	handle->map = mmap(NULL, handle->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (handle->map == MAP_FAILED) {
		mmap_free(handle);
		return NULL;
	}

	return handle;
}

//
// Shared Device MMAP
//

typedef struct mmap_device_handle {
	int fd;
	void *map;
	size_t size;
} mmap_device_handle;

let mmap_device_handle default_mmap_device_handle = { 0 };

fn void munmap_device(mmap_device_handle *handle)
{
	if (handle && (handle->map != MAP_FAILED) && handle->size) {
		munmap(handle->map, handle->size);
	}

	if (handle && handle->fd >= 0) {
		close(handle->fd);
	}

	if (handle) {
		*handle = default_mmap_device_handle;
		free(handle);
	}
}

fn mmap_device_handle *mmap_device(char *dev, size_t size)
{
	mmap_device_handle *handle = malloc(sizeof(mmap_device_handle));
	int ret = 0;

	if (!handle)
		return NULL;

	*handle = default_mmap_device_handle;

	handle->fd = open(dev, O_RDWR);
	if (handle->fd < 0) {
		pr("Unable to open %s, map device failed: errno %d", dev, errno);
		munmap_device(handle);
		return NULL;
	}

	handle->size = size;
	handle->map = mmap(NULL, handle->size, PROT_READ | PROT_WRITE, MAP_SHARED_VALIDATE | MAP_SYNC, handle->fd, 0);
	if (handle->map == MAP_FAILED) {
		pr("Unable to mmap %s, map device failed: errno %d", dev, errno);
		munmap_device(handle);
		return NULL;
	}

	ret = madvise(handle->map, handle->size, MADV_HUGEPAGE);
	if (ret) {
		pr("Map %s (%s) cannot enable THP: errno %d", dev, format_size(handle->size), errno);
	}

	return handle;
}

//
// Shared File MMAP
//

typedef struct mmap_file_handle {
	int fd;
	void *map;
	size_t size;
} mmap_file_handle;

let mmap_file_handle default_mmap_file_handle = { 0 };

fn void munmap_file(mmap_file_handle *handle)
{
	if (handle && (handle->map != MAP_FAILED) && handle->size) {
		munmap(handle->map, handle->size);
	}

	if (handle && handle->fd >= 0) {
		close(handle->fd);
	}

	if (handle) {
		*handle = default_mmap_file_handle;
		free(handle);
	}
}

fn mmap_file_handle *mmap_file(char *file)
{
	mmap_file_handle *handle = malloc(sizeof(mmap_file_handle));
	struct stat sb;
	int ret;

	if (!handle)
		return NULL;

	*handle = default_mmap_file_handle;

	handle->fd = open(file, O_RDWR);
	if (handle->fd < 0) {
		pr("Unable to open %s, map file failed: errno %d", file, errno);
		munmap_file(handle);
		return NULL;
	}

	ret = fstat(handle->fd, &sb);
	if (ret) {
		munmap_file(handle);
		return NULL;
	}
	
	handle->size = sb.st_size;
	if (!handle->size) {
		pr("Unable to mmap %s: File has zero size", file);
		munmap_file(handle);
		return NULL;
	}

	handle->map = mmap(NULL, handle->size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);
	if (handle->map == MAP_FAILED) {
		pr("Unable to mmap %s, map file failed: errno %d", file, errno);
		munmap_file(handle);
		return NULL;
	}

	ret = madvise(handle->map, handle->size, MADV_HUGEPAGE);
	if (ret) {
		pr("Map %s (%s) cannot enable THP: errno %d", file, format_size(handle->size), errno);
	}

	return handle;
}

//
// Memory routines
//

// Fault in given memory range.
fn void memfault(void *ptr, size_t size)
{
	u8 *tmp = ptr;

	for (size_t proc = 0; proc < size; proc += PAGE_SIZE) {
		tmp[proc] = 0;
	}
}

// Fill given memory range with random bytes.
fn void memrandomize(void *ptr, size_t size)
{
	prng_bytes(NULL, ptr, size);
}

// Exchange the content of the two memory regions.
// This function will inherently generate bad result for overlapping regions.
fn void memxchg(void *a, void *b, size_t size)
{
	if (a == b)
		return;

	while (size >= sizeof(u64)) {
		u64 *aa = a;
		u64 *bb = b;
		u64 tmp = *aa;
		*aa = *bb;
		*bb = tmp;

		size -= sizeof(u64);
		a += sizeof(u64);
		b += sizeof(u64);
	}

	for (int i = 0; i < size; i++) {
		u8 *aa = a + i;
		u8 *bb = b + i;
		u8 tmp = *aa;
		*aa = *bb;
		*bb = tmp;
	}
}

// Declare a memshuffle operation on given type with given name suffix.
#define memshuffle_decl(name, type) \
fn void memshuffle_##name(type *list, size_t len) \
{ \
	size_t j; \
	type tmp; \
	while(len) { \
		j = prng_u64(NULL) % len; \
		len--; \
		tmp = list[j]; \
		list[j] = list[len]; \
		list[len] = tmp; \
	} \
}

memshuffle_decl(u64, u64)
memshuffle_decl(u32, u32)
memshuffle_decl(u16, u16)
memshuffle_decl(u8, u8)

#define memshuffle_64(list, len) memshuffle_u64((u64 *) (list), len)
#define memshuffle_32(list, len) memshuffle_u32((u32 *) (list), len)
#define memshuffle_16(list, len) memshuffle_u16((u16 *) (list), len)
#define memshuffle_8(list, len) memshuffle_u8((u8 *) (list), len)

fn void memshuffle(void *ptr, size_t elemsz, size_t elemcnt)
{
	size_t j;

	switch(elemsz) {
	case 8: return memshuffle_64(ptr, elemcnt);
	case 4: return memshuffle_32(ptr, elemcnt);
	case 2: return memshuffle_16(ptr, elemcnt);
	case 1: return memshuffle_8(ptr, elemcnt);
	default: break;
	}

	while(elemcnt) {
		j = prng_u64(NULL) % elemcnt;
		elemcnt--;
		memxchg(ptr + elemsz * elemcnt, ptr + elemsz * j, elemsz);
	}
}
