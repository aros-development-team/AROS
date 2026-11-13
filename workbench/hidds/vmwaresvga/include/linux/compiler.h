/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_COMPILER_H_
#define _LINUX_COMPILER_H_

#include <aros/system.h>
#include <linux/compiler_types.h>
#include <linux/compiler_attributes.h>

#ifndef likely
#define likely(x)               __builtin_expect(!!(x), 1)
#define unlikely(x)             __builtin_expect(!!(x), 0)
#endif

#ifndef barrier
#define barrier()               __asm__ __volatile__("" : : : "memory")
#endif
#define barrier_data(ptr)       __asm__ __volatile__("" : : "r"(ptr) : "memory")

#ifndef __always_inline
#define __always_inline         inline __attribute__((__always_inline__))
#endif
#define noinline                __attribute__((__noinline__))
#define __maybe_unused          __attribute__((__unused__))
#define __always_unused         __attribute__((__unused__))
#ifndef __used
#define __used                  __attribute__((__used__))
#endif
#ifndef __unused
#define __unused                __attribute__((__unused__))
#endif
#ifndef __cold
#define __cold                  __attribute__((__cold__))
#endif
#define __hot
#ifndef __nonstring
#if __has_attribute(__nonstring__)
#define __nonstring             __attribute__((__nonstring__))
#else
#define __nonstring
#endif
#endif
#ifndef __pure
#define __pure                  __attribute__((__pure__))
#endif
#ifndef __weak
#define __weak                  __attribute__((__weak__))
#endif
#ifndef __noreturn
#define __noreturn              __attribute__((__noreturn__))
#endif
#ifndef __printf
#define __printf(a, b)          __attribute__((__format__(__printf__, a, b)))
#endif
#ifndef __scanf
#define __scanf(a, b)           __attribute__((__format__(__scanf__, a, b)))
#endif
#ifndef __packed
#define __packed                __attribute__((__packed__))
#endif
#ifndef __aligned
#define __aligned(x)            __attribute__((__aligned__(x)))
#endif
#ifndef __aligned_largest
#define __aligned_largest       __attribute__((__aligned__))
#endif
#ifndef __section
#define __section(s)            __attribute__((__section__(s)))
#endif
#ifndef __malloc
#define __malloc                __attribute__((__malloc__))
#endif
#define __alloc_size(x, ...)
#define __designated_init
#define __visible
#define __init
#define __exit
#define __initdata
#define __initconst
#define __exitdata
#define __ro_after_init
#define __read_mostly
#define __refdata
#define __randomize_layout
#define __no_randomize_layout
#define __nocast
#define __counted_by(m)
#define __counted_by_le(m)
#define __counted_by_be(m)
#define __diag_push()
#define __diag_pop()
#define __diag_ignore_all(a, b)
#define __assume_aligned(a, ...)
#define __flatten
#define __fix_address
#define __no_kasan_or_inline    inline
#define __no_sanitize_or_inline inline
#ifndef __attribute_const__
#define __attribute_const__     __attribute__((__const__))
#endif
#define __must_hold(x)
#define __acquires(x)
#define __releases(x)
#define __acquire(x)            (void)0
#define __release(x)            (void)0
#define __cond_lock(x, c)       (c)
#define __chk_user_ptr(x)       (void)0
#define __chk_io_ptr(x)         (void)0
#define __same_type(a, b)       __builtin_types_compatible_p(typeof(a), typeof(b))
#define __must_be_array(a)      __same_type(a, &(a)[0])
#define __is_constexpr(x)       __builtin_constant_p(x)
#define __compiletime_error(m)
#define __compiletime_warning(m)
#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)
#define notrace
#define __nostackprotector
#define asmlinkage
#ifndef fallthrough
#if defined(__GNUC__) && __GNUC__ >= 7
#define fallthrough             __attribute__((__fallthrough__))
#else
#define fallthrough             do { } while (0)
#endif
#endif
#define ___PASTE(a, b)          a##b
#define __PASTE(a, b)           ___PASTE(a, b)
#define __UNIQUE_ID(prefix)     __PASTE(__PASTE(__UNIQUE_ID_, prefix), __COUNTER__)
#define __auto_type             __auto_type
#define __no_const
#define OPTIMIZER_HIDE_VAR(var) __asm__ ("" : "=r" (var) : "0" (var))
#define __force_order

#define READ_ONCE(x)            (*(const volatile typeof(x) *)&(x))
#define WRITE_ONCE(x, val)      do { *(volatile typeof(x) *)&(x) = (val); } while (0)
#define ACCESS_ONCE(x)          (*(volatile typeof(x) *)&(x))
#define smp_load_acquire(p)     ({ typeof(*p) ___p1 = READ_ONCE(*p); __sync_synchronize(); ___p1; })
#define smp_store_release(p, v) do { __sync_synchronize(); WRITE_ONCE(*p, v); } while (0)
#define smp_store_mb(var, value) do { WRITE_ONCE(var, value); __sync_synchronize(); } while (0)
#define data_race(expr)         (expr)

#define unreachable()           __builtin_unreachable()
#define __builtin_expect_probability(e, v, p) __builtin_expect(e, v)

#endif /* _LINUX_COMPILER_H_ */
