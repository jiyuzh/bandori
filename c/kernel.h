#pragma once

#include <stdbool.h>
#include <stdint.h>

//
// uapi/asm-generic/int-ll64.h
//

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#ifdef __GNUC__
__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
#else
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif

//
// asm-generic/int-ll64.h
//

typedef __s8 s8;
typedef __u8 u8;
typedef __s16 s16;
typedef __u16 u16;
typedef __s32 s32;
typedef __u32 u32;
typedef __s64 s64;
typedef __u64 u64;

//
// linux/printk.h (Modified)
//

#define printk(...) do { printf(__VA_ARGS__); fflush(stdout); } while(0)
#define pr_emerg(...) printk(__VA_ARGS__)
#define pr_alert(...) printk(__VA_ARGS__)
#define pr_crit(...) printk(__VA_ARGS__)
#define pr_err(...) printk(__VA_ARGS__)
#define pr_warn(...) printk(__VA_ARGS__)
#define pr_notice(...) printk(__VA_ARGS__)
#define pr_info(...) printk(__VA_ARGS__)
#define pr_cont(...) printk(__VA_ARGS__)
#define pr_devel(...) printk(__VA_ARGS__)
#define pr_debug(...) printk(__VA_ARGS__)

//
// uapi/linux/const.h
//

#define __AC(X,Y) (X##Y)
#define _AC(X,Y) __AC(X,Y)
#define _AT(T,X) ((T)(X))

#define _UL(x) (_AC(x, UL))
#define _ULL(x) (_AC(x, ULL))

#define _BITUL(x) (_UL(1) << (x))
#define _BITULL(x) (_ULL(1) << (x))

#define __ALIGN_KERNEL(x, a) __ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask) (((x) + (mask)) & ~(mask))

#define __KERNEL_DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

//
// vdso/const.h
//

#define UL(x) (_UL(x))
#define ULL(x) (_ULL(x))

//
// vdso/bits.h
//

#define BIT(nr) (UL(1) << (nr))

//
// linux/align.h
//

#define ALIGN(x, a) __ALIGN_KERNEL((x), (a))
#define ALIGN_DOWN(x, a) __ALIGN_KERNEL((x) - ((a) - 1), (a))
#define __ALIGN_MASK(x, mask) __ALIGN_KERNEL_MASK((x), (mask))
#define PTR_ALIGN(p, a) ((typeof(p))ALIGN((unsigned long)(p), (a)))
#define PTR_ALIGN_DOWN(p, a) ((typeof(p))ALIGN_DOWN((unsigned long)(p), (a)))
#define IS_ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)

//
// include/linux/compiler.h (Modified)
//

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

//
// linux/compiler_attributes.h
//

#if __has_attribute(__error__)
# define __compiletime_error(msg) __attribute__((__error__(msg)))
#else
# define __compiletime_error(msg)
#endif

//
// linux/compiler_types.h
//

#define __user

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

# define __compiletime_assert(condition, msg, prefix, suffix) \
	do { \
		extern void prefix ## suffix(void) __compiletime_error(msg); \
		if (!(condition)) \
			prefix ## suffix(); \
	} while (0)
#define _compiletime_assert(condition, msg, prefix, suffix) \
	__compiletime_assert(condition, msg, prefix, suffix)
#define compiletime_assert(condition, msg) \
	_compiletime_assert(condition, msg, __compiletime_assert_, __COUNTER__)

//
// linux/build_bug.h
//

#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)
#define BUILD_BUG_ON(condition) \
	BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)

//
// linux/stddef.h
//

#undef NULL
#define NULL ((void *)0)

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE, MEMBER) __compiler_offsetof(TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)
#endif

//
// linux/kernel.h
//

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({ \
	void *__mptr = (void *)(ptr); \
	BUILD_BUG_ON_MSG(!__same_type(*(ptr), ((type *)0)->member) && \
			 !__same_type(*(ptr), void), \
			 "pointer type mismatch in container_of()"); \
	((type *)(__mptr - offsetof(type, member))); })

//
// arch/x86/include/asm/pgtable_64_types.h
//

#ifdef CONFIG_X86_5LEVEL

#define PGDIR_SHIFT	pgdir_shift
#define PTRS_PER_PGD	512

#define P4D_SHIFT		39
#define MAX_PTRS_PER_P4D	512
#define PTRS_PER_P4D		ptrs_per_p4d
#define P4D_SIZE		(_AC(1, UL) << P4D_SHIFT)
#define P4D_MASK		(~(P4D_SIZE - 1))

#define MAX_POSSIBLE_PHYSMEM_BITS	52

#else /* CONFIG_X86_5LEVEL */

#define PGDIR_SHIFT		39
#define PTRS_PER_PGD		512
#define MAX_PTRS_PER_P4D	1

#endif /* CONFIG_X86_5LEVEL */

#define PUD_SHIFT	30
#define PTRS_PER_PUD	512

#define PMD_SHIFT	21
#define PTRS_PER_PMD	512

#define PTRS_PER_PTE	512

#define PMD_SIZE	(_AC(1, UL) << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE - 1))
#define PUD_SIZE	(_AC(1, UL) << PUD_SHIFT)
#define PUD_MASK	(~(PUD_SIZE - 1))
#define PGDIR_SIZE	(_AC(1, UL) << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE - 1))

//
// arch/x86/include/asm/page_types.h
//

#define PAGE_SHIFT 12
#define PAGE_SIZE (_AC(1,UL) << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE-1))

#define PMD_PAGE_SIZE (_AC(1, UL) << PMD_SHIFT)
#define PMD_PAGE_MASK (~(PMD_PAGE_SIZE-1))

#define PUD_PAGE_SIZE (_AC(1, UL) << PUD_SHIFT)
#define PUD_PAGE_MASK (~(PUD_PAGE_SIZE-1))

#define HPAGE_SHIFT		PMD_SHIFT
#define HPAGE_SIZE		(_AC(1,UL) << HPAGE_SHIFT)
#define HPAGE_MASK		(~(HPAGE_SIZE - 1))
#define HUGETLB_PAGE_ORDER	(HPAGE_SHIFT - PAGE_SHIFT)

//
// uapi/asm-generic/ioctl.h
//

#define _IOC_NRBITS	8
#define _IOC_TYPEBITS	8

#ifndef _IOC_SIZEBITS
# define _IOC_SIZEBITS	14
#endif

#ifndef _IOC_DIRBITS
# define _IOC_DIRBITS	2
#endif

#define _IOC_NRMASK	((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK	((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK	((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK	((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT	0
#define _IOC_TYPESHIFT	(_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT	(_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT	(_IOC_SIZESHIFT+_IOC_SIZEBITS)

#ifndef _IOC_NONE
# define _IOC_NONE	0U
#endif

#ifndef _IOC_WRITE
# define _IOC_WRITE	1U
#endif

#ifndef _IOC_READ
# define _IOC_READ	2U
#endif

#define _IOC(dir,type,nr,size) \
	(((dir)  << _IOC_DIRSHIFT) | \
	 ((type) << _IOC_TYPESHIFT) | \
	 ((nr)   << _IOC_NRSHIFT) | \
	 ((size) << _IOC_SIZESHIFT))

#ifndef __KERNEL__
#define _IOC_TYPECHECK(t) (sizeof(t))
#endif

#define _IO(type,nr)		_IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,size)	_IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOW(type,nr,size)	_IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOWR(type,nr,size)	_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOR_BAD(type,nr,size)	_IOC(_IOC_READ,(type),(nr),sizeof(size))
#define _IOW_BAD(type,nr,size)	_IOC(_IOC_WRITE,(type),(nr),sizeof(size))
#define _IOWR_BAD(type,nr,size)	_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))

#define _IOC_DIR(nr)		(((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)		(((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)		(((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)		(((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)

#define IOC_IN		(_IOC_WRITE << _IOC_DIRSHIFT)
#define IOC_OUT		(_IOC_READ << _IOC_DIRSHIFT)
#define IOC_INOUT	((_IOC_WRITE|_IOC_READ) << _IOC_DIRSHIFT)
#define IOCSIZE_MASK	(_IOC_SIZEMASK << _IOC_SIZESHIFT)
#define IOCSIZE_SHIFT	(_IOC_SIZESHIFT)

//
// include/vdso/time64.h
//

#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define PSEC_PER_SEC	1000000000000LL
#define FSEC_PER_SEC	1000000000000000LL
