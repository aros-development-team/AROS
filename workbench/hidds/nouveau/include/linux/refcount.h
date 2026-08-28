/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_REFCOUNT_H_
#define _LINUX_REFCOUNT_H_

#include <linux/atomic.h>
#include <linux/bug.h>
#include <linux/compiler.h>
#include <linux/limits.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>

typedef struct refcount_struct {
    atomic_t refs;
} refcount_t;

#define REFCOUNT_INIT(n)        { .refs = ATOMIC_INIT(n), }
#define REFCOUNT_MAX            INT_MAX
#define REFCOUNT_SATURATED      (INT_MIN / 2)

static inline void refcount_set(refcount_t *r, int n)              { atomic_set(&r->refs, n); }
static inline unsigned int refcount_read(const refcount_t *r)     { return (unsigned int)atomic_read(&r->refs); }
static inline bool refcount_add_not_zero(int i, refcount_t *r)     { return atomic_add_unless(&r->refs, i, 0); }
static inline void refcount_add(int i, refcount_t *r)              { atomic_add(i, &r->refs); }
static inline void refcount_inc(refcount_t *r)                     { atomic_inc(&r->refs); }
static inline bool refcount_inc_not_zero(refcount_t *r)            { return refcount_add_not_zero(1, r); }
static inline bool refcount_sub_and_test(int i, refcount_t *r)     { return atomic_sub_and_test(i, &r->refs); }
static inline bool refcount_dec_and_test(refcount_t *r)            { return atomic_dec_and_test(&r->refs); }
static inline void refcount_dec(refcount_t *r)                     { atomic_dec(&r->refs); }
static inline bool refcount_dec_not_one(refcount_t *r)
{
    int c = atomic_read(&r->refs);
    do {
        if (c == 1)
            return false;
    } while (!atomic_try_cmpxchg(&r->refs, &c, c - 1));
    return true;
}
static inline bool refcount_dec_if_one(refcount_t *r)
{
    int c = 1;
    return atomic_try_cmpxchg(&r->refs, &c, 0);
}

static inline bool refcount_dec_and_lock(refcount_t *r, spinlock_t *lock)
{
    spin_lock(lock);
    if (refcount_dec_and_test(r))
        return true;
    spin_unlock(lock);
    return false;
}
static inline bool refcount_dec_and_lock_irqsave(refcount_t *r, spinlock_t *lock, unsigned long *flags)
{
    spin_lock_irqsave(lock, *flags);
    if (refcount_dec_and_test(r))
        return true;
    spin_unlock_irqrestore(lock, *flags);
    return false;
}
static inline bool refcount_dec_and_mutex_lock(refcount_t *r, struct mutex *lock)
{
    mutex_lock(lock);
    if (refcount_dec_and_test(r))
        return true;
    mutex_unlock(lock);
    return false;
}

#endif /* _LINUX_REFCOUNT_H_ */
