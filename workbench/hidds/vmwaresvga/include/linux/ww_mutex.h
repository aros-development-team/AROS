/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_WW_MUTEX_H_
#define _LINUX_WW_MUTEX_H_

#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/rtmutex.h>
#include <linux/lockdep.h>

/*
 * Wound/wait mutexes without the wound/wait part: an acquire context is
 * only bookkeeping, contention simply blocks. The buffer objects this
 * protects are all handled under the driver's global engine lock, so the
 * cross-task ordering the ticket scheme guards against does not occur.
 */
struct ww_class {
    const char *name;
};
struct ww_acquire_ctx {
    struct task_struct *task;
    unsigned int acquired;
    unsigned int contending;
};
struct ww_mutex {
    struct mutex base;
    struct ww_acquire_ctx *ctx;
};

#define DEFINE_WW_CLASS(classname)      struct ww_class classname = { .name = #classname }
#define DEFINE_WD_CLASS(classname)      DEFINE_WW_CLASS(classname)
#define __WW_CLASS_INITIALIZER(n)       { .name = #n }
#define __WW_MUTEX_INITIALIZER(n, c)    { .base = __MUTEX_INITIALIZER(n.base), .ctx = NULL }
#define DEFINE_WW_MUTEX(mutexname, ww_class) struct ww_mutex mutexname = __WW_MUTEX_INITIALIZER(mutexname, ww_class)

static inline void ww_mutex_init(struct ww_mutex *lock, struct ww_class *ww_class)
{
    mutex_init(&lock->base);
    lock->ctx = NULL;
}
static inline void ww_mutex_destroy(struct ww_mutex *lock) { mutex_destroy(&lock->base); }
static inline void ww_acquire_init(struct ww_acquire_ctx *ctx, struct ww_class *ww_class)
{
    ctx->task = current;
    ctx->acquired = 0;
    ctx->contending = 0;
}
static inline void ww_acquire_done(struct ww_acquire_ctx *ctx) { }
static inline void ww_acquire_fini(struct ww_acquire_ctx *ctx) { }
static inline int ww_mutex_lock(struct ww_mutex *lock, struct ww_acquire_ctx *ctx)
{
    if (ctx && lock->ctx == ctx)
        return -EALREADY;
    mutex_lock(&lock->base);
    lock->ctx = ctx;
    if (ctx)
        ctx->acquired++;
    return 0;
}
static inline int ww_mutex_lock_interruptible(struct ww_mutex *lock, struct ww_acquire_ctx *ctx)
{
    return ww_mutex_lock(lock, ctx);
}
static inline void ww_mutex_lock_slow(struct ww_mutex *lock, struct ww_acquire_ctx *ctx)
{
    ww_mutex_lock(lock, ctx);
}
static inline int ww_mutex_lock_slow_interruptible(struct ww_mutex *lock, struct ww_acquire_ctx *ctx)
{
    return ww_mutex_lock(lock, ctx);
}
static inline int ww_mutex_trylock(struct ww_mutex *lock, struct ww_acquire_ctx *ctx)
{
    if (!mutex_trylock(&lock->base))
        return 0;
    lock->ctx = ctx;
    if (ctx)
        ctx->acquired++;
    return 1;
}
static inline void ww_mutex_unlock(struct ww_mutex *lock)
{
    if (lock->ctx)
        lock->ctx->acquired--;
    lock->ctx = NULL;
    mutex_unlock(&lock->base);
}
static inline bool ww_mutex_is_locked(struct ww_mutex *lock) { return mutex_is_locked(&lock->base); }

#endif /* _LINUX_WW_MUTEX_H_ */
