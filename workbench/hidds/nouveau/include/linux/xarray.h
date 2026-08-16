/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_XARRAY_H_
#define _LINUX_XARRAY_H_

#include <linux/idr.h>
#include <linux/spinlock.h>
#include <linux/bug.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/kconfig.h>
#include <linux/limits.h>
#include <linux/rcupdate.h>
#include <linux/types.h>
#include <linux/kernel.h>

/*
 * A very small xarray on top of the idr: enough for handle tables that
 * allocate ids and look them up.
 */
struct xarray {
    struct idr idr;
    spinlock_t xa_lock;
    gfp_t flags;
};
#define XA_FLAGS_ALLOC          (1U << 0)
#define XA_FLAGS_ALLOC1         (1U << 1)
#define XA_FLAGS_LOCK_IRQ       (1U << 2)
#define XA_FLAGS_LOCK_BH        (1U << 3)
#define XA_LIMIT(min, max)      (struct xa_limit) { .min = (min), .max = (max) }
#define xa_limit_32b            XA_LIMIT(0, UINT_MAX)
#define xa_limit_31b            XA_LIMIT(0, INT_MAX)
struct xa_limit { u32 max; u32 min; };
#define DEFINE_XARRAY(name)     struct xarray name
#define DEFINE_XARRAY_ALLOC(name) struct xarray name
#define DEFINE_XARRAY_FLAGS(name, f) struct xarray name
static inline void xa_init_flags(struct xarray *xa, gfp_t flags) { idr_init(&xa->idr); spin_lock_init(&xa->xa_lock); xa->flags = flags; }
static inline void xa_init(struct xarray *xa) { xa_init_flags(xa, 0); }
static inline void xa_destroy(struct xarray *xa) { idr_destroy(&xa->idr); }
static inline void *xa_load(struct xarray *xa, unsigned long index) { return idr_find(&xa->idr, index); }
static inline void *xa_store(struct xarray *xa, unsigned long index, void *entry, gfp_t gfp) { return idr_replace(&xa->idr, entry, index); }
static inline void *xa_erase(struct xarray *xa, unsigned long index) { return idr_remove(&xa->idr, index); }
static inline bool xa_empty(const struct xarray *xa) { return idr_is_empty((struct idr *)&xa->idr); }
static inline int xa_alloc(struct xarray *xa, u32 *id, void *entry, struct xa_limit limit, gfp_t gfp)
{
    int ret = idr_alloc(&xa->idr, entry, limit.min, limit.max == UINT_MAX ? 0 : limit.max + 1, gfp);
    if (ret < 0)
        return ret;
    *id = ret;
    return 0;
}
static inline int xa_alloc_cyclic(struct xarray *xa, u32 *id, void *entry, struct xa_limit limit, u32 *next, gfp_t gfp)
{
    return xa_alloc(xa, id, entry, limit, gfp);
}
static inline int xa_insert(struct xarray *xa, unsigned long index, void *entry, gfp_t gfp)
{
    if (idr_find(&xa->idr, index))
        return -EBUSY;
    return idr_alloc(&xa->idr, entry, index, index + 1, gfp) < 0 ? -ENOMEM : 0;
}
static inline void *xa_find(struct xarray *xa, unsigned long *indexp, unsigned long max, unsigned int filter)
{
    int id = (int)*indexp;
    void *e = idr_get_next(&xa->idr, &id);
    if (e) *indexp = id;
    return e;
}
static inline void *xa_find_after(struct xarray *xa, unsigned long *indexp, unsigned long max, unsigned int filter)
{
    int id = (int)*indexp + 1;
    void *e = idr_get_next(&xa->idr, &id);
    if (e) *indexp = id;
    return e;
}
#define XA_PRESENT              1
#define xa_for_each(xa, index, entry) for (index = 0, entry = xa_find(xa, &index, ULONG_MAX, XA_PRESENT); entry; entry = xa_find_after(xa, &index, ULONG_MAX, XA_PRESENT))
#define xa_for_each_start(xa, index, entry, start) for (index = start, entry = xa_find(xa, &index, ULONG_MAX, XA_PRESENT); entry; entry = xa_find_after(xa, &index, ULONG_MAX, XA_PRESENT))
#define xa_lock(xa)             spin_lock(&(xa)->xa_lock)
#define xa_unlock(xa)           spin_unlock(&(xa)->xa_lock)
#define xa_lock_irq(xa)         spin_lock_irq(&(xa)->xa_lock)
#define xa_unlock_irq(xa)       spin_unlock_irq(&(xa)->xa_lock)
#define xa_lock_irqsave(xa, f)  spin_lock_irqsave(&(xa)->xa_lock, f)
#define xa_unlock_irqrestore(xa, f) spin_unlock_irqrestore(&(xa)->xa_lock, f)
#define xa_is_err(e)            IS_ERR(e)
#define xa_err(e)               (IS_ERR(e) ? (int)PTR_ERR(e) : 0)
#define xa_mk_value(v)          ((void *)(((IPTR)(v) << 1) | 1))
#define xa_to_value(e)          ((unsigned long)(e) >> 1)
#define xa_is_value(e)          ((IPTR)(e) & 1)
#define __xa_erase(xa, i)       xa_erase(xa, i)
#define __xa_store(xa, i, e, g) xa_store(xa, i, e, g)
#define __xa_alloc(xa, id, e, l, g) xa_alloc(xa, id, e, l, g)
#define __xa_insert(xa, i, e, g) xa_insert(xa, i, e, g)
#define xa_release(xa, i)       do { } while (0)
#define xa_reserve(xa, i, g)    (0)

#endif /* _LINUX_XARRAY_H_ */
