/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_ATOMIC_H_
#define _LINUX_ATOMIC_H_

#include <linux/types.h>
#include <linux/compiler.h>

#define ATOMIC_INIT(i)          { (i) }
#define ATOMIC64_INIT(i)        { (i) }
#define ATOMIC_LONG_INIT(i)     { (i) }

#define __ATOMIC_OPS(pfx, type, ctype)                                                          \
static inline ctype pfx##_read(const type *v)               { return __atomic_load_n(&v->counter, __ATOMIC_RELAXED); } \
static inline ctype pfx##_read_acquire(const type *v)       { return __atomic_load_n(&v->counter, __ATOMIC_ACQUIRE); } \
static inline void  pfx##_set(type *v, ctype i)             { __atomic_store_n(&v->counter, i, __ATOMIC_RELAXED); } \
static inline void  pfx##_set_release(type *v, ctype i)     { __atomic_store_n(&v->counter, i, __ATOMIC_RELEASE); } \
static inline void  pfx##_add(ctype i, type *v)             { __atomic_add_fetch(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline void  pfx##_sub(ctype i, type *v)             { __atomic_sub_fetch(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline void  pfx##_inc(type *v)                      { __atomic_add_fetch(&v->counter, 1, __ATOMIC_SEQ_CST); } \
static inline void  pfx##_dec(type *v)                      { __atomic_sub_fetch(&v->counter, 1, __ATOMIC_SEQ_CST); } \
static inline void  pfx##_and(ctype i, type *v)             { __atomic_and_fetch(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline void  pfx##_or(ctype i, type *v)              { __atomic_or_fetch(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline void  pfx##_xor(ctype i, type *v)             { __atomic_xor_fetch(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline void  pfx##_andnot(ctype i, type *v)          { __atomic_and_fetch(&v->counter, ~i, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_add_return(ctype i, type *v)      { return __atomic_add_fetch(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_sub_return(ctype i, type *v)      { return __atomic_sub_fetch(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_inc_return(type *v)               { return __atomic_add_fetch(&v->counter, 1, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_dec_return(type *v)               { return __atomic_sub_fetch(&v->counter, 1, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_add(ctype i, type *v)       { return __atomic_fetch_add(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_sub(ctype i, type *v)       { return __atomic_fetch_sub(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_inc(type *v)                { return __atomic_fetch_add(&v->counter, 1, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_dec(type *v)                { return __atomic_fetch_sub(&v->counter, 1, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_and(ctype i, type *v)       { return __atomic_fetch_and(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_or(ctype i, type *v)        { return __atomic_fetch_or(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_xor(ctype i, type *v)       { return __atomic_fetch_xor(&v->counter, i, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_andnot(ctype i, type *v)    { return __atomic_fetch_and(&v->counter, ~i, __ATOMIC_SEQ_CST); } \
static inline bool  pfx##_dec_and_test(type *v)             { return __atomic_sub_fetch(&v->counter, 1, __ATOMIC_SEQ_CST) == 0; } \
static inline bool  pfx##_inc_and_test(type *v)             { return __atomic_add_fetch(&v->counter, 1, __ATOMIC_SEQ_CST) == 0; } \
static inline bool  pfx##_sub_and_test(ctype i, type *v)    { return __atomic_sub_fetch(&v->counter, i, __ATOMIC_SEQ_CST) == 0; } \
static inline bool  pfx##_add_negative(ctype i, type *v)    { return __atomic_add_fetch(&v->counter, i, __ATOMIC_SEQ_CST) < 0; } \
static inline ctype pfx##_xchg(type *v, ctype n)            { return __atomic_exchange_n(&v->counter, n, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_cmpxchg(type *v, ctype o, ctype n) { __atomic_compare_exchange_n(&v->counter, &o, n, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return o; } \
static inline bool  pfx##_try_cmpxchg(type *v, ctype *o, ctype n) { return __atomic_compare_exchange_n(&v->counter, o, n, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); } \
static inline ctype pfx##_fetch_add_unless(type *v, ctype a, ctype u) \
{                                                                       \
    ctype c = pfx##_read(v);                                            \
    do { if (unlikely(c == u)) break; } while (!pfx##_try_cmpxchg(v, &c, c + a)); \
    return c;                                                           \
}                                                                       \
static inline bool  pfx##_add_unless(type *v, ctype a, ctype u)     { return pfx##_fetch_add_unless(v, a, u) != u; } \
static inline bool  pfx##_inc_not_zero(type *v)             { return pfx##_add_unless(v, 1, 0); } \
static inline bool  pfx##_inc_unless_negative(type *v)      { ctype c = pfx##_read(v); do { if (c < 0) return false; } while (!pfx##_try_cmpxchg(v, &c, c + 1)); return true; } \
static inline bool  pfx##_dec_unless_positive(type *v)      { ctype c = pfx##_read(v); do { if (c > 0) return false; } while (!pfx##_try_cmpxchg(v, &c, c - 1)); return true; } \
static inline ctype pfx##_dec_if_positive(type *v)          { ctype c = pfx##_read(v); do { if (c <= 0) break; } while (!pfx##_try_cmpxchg(v, &c, c - 1)); return c - 1; }

__ATOMIC_OPS(atomic, atomic_t, int)
__ATOMIC_OPS(atomic64, atomic64_t, s64)
__ATOMIC_OPS(atomic_long, atomic_long_t, long)
#undef __ATOMIC_OPS

#define atomic_add_return_relaxed       atomic_add_return
#define atomic_sub_return_relaxed       atomic_sub_return
#define atomic_inc_return_relaxed       atomic_inc_return
#define atomic_dec_return_relaxed       atomic_dec_return
#define atomic_fetch_add_relaxed        atomic_fetch_add
#define atomic_cmpxchg_relaxed          atomic_cmpxchg
#define atomic_cmpxchg_release          atomic_cmpxchg
#define atomic_cmpxchg_acquire          atomic_cmpxchg
#define atomic_try_cmpxchg_relaxed      atomic_try_cmpxchg
#define atomic_try_cmpxchg_acquire      atomic_try_cmpxchg
#define atomic_try_cmpxchg_release      atomic_try_cmpxchg
#define atomic_xchg_relaxed             atomic_xchg
#define atomic_dec_return_release       atomic_dec_return
#define atomic_inc_return_acquire       atomic_inc_return
#define atomic64_add_return_relaxed     atomic64_add_return
#define atomic64_fetch_add_relaxed      atomic64_fetch_add
#define atomic64_fetch_or_relaxed       atomic64_fetch_or
#define atomic64_cmpxchg_relaxed        atomic64_cmpxchg
#define atomic64_try_cmpxchg_relaxed    atomic64_try_cmpxchg
#define atomic64_xchg_relaxed           atomic64_xchg
#define atomic_long_add_return_relaxed  atomic_long_add_return
#define atomic_long_try_cmpxchg_relaxed atomic_long_try_cmpxchg
#define atomic_long_fetch_add_relaxed   atomic_long_fetch_add
#define atomic_long_xchg_relaxed        atomic_long_xchg
#define atomic_long_read_acquire        atomic_long_read
#define atomic_long_set_release         atomic_long_set

#define xchg(ptr, v)            __atomic_exchange_n((ptr), (v), __ATOMIC_SEQ_CST)
#define xchg_relaxed(ptr, v)    __atomic_exchange_n((ptr), (v), __ATOMIC_RELAXED)
#define cmpxchg(ptr, o, n) ({                                           \
    typeof(*(ptr)) __o = (o);                                           \
    __atomic_compare_exchange_n((ptr), &__o, (n), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
    __o; })
#define cmpxchg64(ptr, o, n)    cmpxchg(ptr, o, n)
#define cmpxchg_relaxed(ptr, o, n) cmpxchg(ptr, o, n)
#define cmpxchg_acquire(ptr, o, n) cmpxchg(ptr, o, n)
#define cmpxchg_release(ptr, o, n) cmpxchg(ptr, o, n)
#define try_cmpxchg(ptr, po, n) __atomic_compare_exchange_n((ptr), (po), (n), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define try_cmpxchg64(ptr, po, n) try_cmpxchg(ptr, po, n)
#define try_cmpxchg_relaxed(ptr, po, n) try_cmpxchg(ptr, po, n)
#define try_cmpxchg_acquire(ptr, po, n) try_cmpxchg(ptr, po, n)
#define try_cmpxchg_release(ptr, po, n) try_cmpxchg(ptr, po, n)

#define smp_mb()                __sync_synchronize()
#define smp_rmb()               __sync_synchronize()
#define smp_wmb()               __sync_synchronize()
#define smp_mb__before_atomic() __sync_synchronize()
#define smp_mb__after_atomic()  __sync_synchronize()
#define smp_mb__after_spinlock() __sync_synchronize()
#define smp_read_barrier_depends() do { } while (0)
#define smp_acquire__after_ctrl_dep() __sync_synchronize()
#define dma_rmb()               __sync_synchronize()
#define dma_wmb()               __sync_synchronize()
#if defined(__i386__) || defined(__x86_64__)
#define mb()                    __asm__ __volatile__("mfence" : : : "memory")
#define rmb()                   __asm__ __volatile__("lfence" : : : "memory")
#define wmb()                   __asm__ __volatile__("sfence" : : : "memory")
#else
#define mb()                    __sync_synchronize()
#define rmb()                   __sync_synchronize()
#define wmb()                   __sync_synchronize()
#endif
#define virt_rmb()              rmb()
#define virt_wmb()              wmb()

#define atomic_dec_and_lock(a, l) ({ bool __r; spin_lock(l); __r = atomic_dec_and_test(a); if (!__r) spin_unlock(l); __r; })

#endif /* _LINUX_ATOMIC_H_ */
