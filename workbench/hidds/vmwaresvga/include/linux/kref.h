/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_KREF_H_
#define _LINUX_KREF_H_

#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/refcount.h>

struct kref {
    refcount_t refcount;
};
#define KREF_INIT(n)            { .refcount = REFCOUNT_INIT(n), }

static inline void kref_init(struct kref *kref)              { refcount_set(&kref->refcount, 1); }
static inline unsigned int kref_read(const struct kref *kref) { return refcount_read(&kref->refcount); }
static inline void kref_get(struct kref *kref)               { refcount_inc(&kref->refcount); }
static inline int kref_put(struct kref *kref, void (*release)(struct kref *kref))
{
    if (refcount_dec_and_test(&kref->refcount)) {
        release(kref);
        return 1;
    }
    return 0;
}
static inline int kref_get_unless_zero(struct kref *kref)     { return refcount_inc_not_zero(&kref->refcount); }

static inline int kref_put_mutex(struct kref *kref, void (*release)(struct kref *kref), struct mutex *lock)
{
    mutex_lock(lock);
    if (refcount_dec_and_test(&kref->refcount)) {
        release(kref);
        return 1;
    }
    mutex_unlock(lock);
    return 0;
}
static inline int kref_put_lock(struct kref *kref, void (*release)(struct kref *kref), spinlock_t *lock)
{
    spin_lock(lock);
    if (refcount_dec_and_test(&kref->refcount)) {
        release(kref);
        return 1;
    }
    spin_unlock(lock);
    return 0;
}

#endif /* _LINUX_KREF_H_ */
