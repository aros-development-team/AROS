/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _XF86ATOMIC_H_
#define _XF86ATOMIC_H_

/* the same atomic_t as the kernel side, so both can share headers */
#include <linux/types.h>
#ifndef ffs
#define ffs(x) __builtin_ffs(x)
#endif

#define HAS_ATOMIC_OPS 1
#define atomic_read(x)          __atomic_load_n(&(x)->counter, __ATOMIC_SEQ_CST)
#define atomic_set(x, val)      __atomic_store_n(&(x)->counter, (val), __ATOMIC_SEQ_CST)
#define atomic_inc(x)           ((void)__atomic_add_fetch(&(x)->counter, 1, __ATOMIC_SEQ_CST))
#define atomic_inc_return(x)    (__atomic_add_fetch(&(x)->counter, 1, __ATOMIC_SEQ_CST))
#define atomic_dec_and_test(x)  (__atomic_sub_fetch(&(x)->counter, 1, __ATOMIC_SEQ_CST) == 0)
#define atomic_add(x, v)        ((void)__atomic_add_fetch(&(x)->counter, (v), __ATOMIC_SEQ_CST))
#define atomic_dec(x, v)        ((void)__atomic_sub_fetch(&(x)->counter, (v), __ATOMIC_SEQ_CST))
#define atomic_cmpxchg(x, oldv, newv) __sync_val_compare_and_swap(&(x)->counter, oldv, newv)

#endif
