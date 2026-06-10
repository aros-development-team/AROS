/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <drm-compat/drm_compat_funcs.h>
#include <drm-compat/drm_compat_dma.h>
#include <drm-compat/drm_compat_mem.h>
#include <linux/bitops.h>

static void dma_fence_free_kref(struct kref *kref)
{
    struct dma_fence *fence = container_of(kref, struct dma_fence, refcount);
    if (fence->ops && fence->ops->release)
        fence->ops->release(fence);
}

struct dma_fence *dma_fence_get(struct dma_fence *fence)
{
    if (fence)
        kref_get(&fence->refcount);
    return fence;
}

void dma_fence_init(struct dma_fence *fence, const struct dma_fence_ops *ops,
                    spinlock_t *lock, u64 context, u64 seqno)
{
    kref_init(&fence->refcount);
    fence->ops = ops;
    fence->lock = lock;
    fence->context = context;
    fence->seqno = seqno;
    fence->flags = 0;
}

void dma_fence_put(struct dma_fence *fence)
{
    if (fence)
        kref_put(&fence->refcount, dma_fence_free_kref);
}

int dma_fence_signal_locked(struct dma_fence *fence)
{
    set_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags);
    return 0;
}

bool dma_fence_is_signaled(struct dma_fence *fence)
{
    if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
        return TRUE;

    if (fence->ops && fence->ops->signaled && fence->ops->signaled(fence))
    {
        set_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags);
        return TRUE;
    }

    return FALSE;
}

void dma_fence_free(struct dma_fence *fence)
{
    kfree(fence);
}

void dma_fence_enable_sw_signaling(struct dma_fence *fence)
{
    if (fence->ops && fence->ops->enable_signaling &&
        !dma_fence_is_signaled(fence))
        fence->ops->enable_signaling(fence);
}

signed long dma_fence_wait(struct dma_fence *fence, bool intr)
{
    signed long ret;

    ret = dma_fence_wait_timeout(fence, intr, LONG_MAX);
    if (ret > 0)
        return 0;
    return ret;
}

long dma_fence_wait_timeout(struct dma_fence *fence, bool intr, unsigned long timeout)
{
    if (fence->ops && fence->ops->wait)
        return fence->ops->wait(fence, intr, timeout);

    dma_fence_enable_sw_signaling(fence);

    if (dma_fence_is_signaled(fence))
        return timeout ? timeout : 1;

    unsigned int usecs = jiffies_to_usecs(timeout);

    while (usecs > 0)
    {
        unsigned long step = usecs > 2000 ? 2000 : usecs;
        udelay(step);
        usecs -= step;

        if (dma_fence_is_signaled(fence))
            return timeout;

#if 0
        if (intr && (SetSignal(0, 0) & SIGBREAKF_CTRL_C))
            return -ERESTARTSYS;
#endif
    }

    return 0;
}

u64 dma_fence_context_alloc(unsigned int num)
{
    static u64 context_counter = 0;
    u64 ret;

    ret = context_counter;
    context_counter += num;
    return ret;
}

/* DMA RESV */

struct dma_fence *dma_resv_get_excl(struct dma_resv *resv)
{
    return resv ? resv->fence_excl : NULL;
}

struct dma_fence *dma_resv_get_excl_rcu(struct dma_resv *resv)
{
    return dma_fence_get(dma_resv_get_excl(resv));
}

void dma_resv_add_excl_fence(struct dma_resv *resv, struct dma_fence *fence)
{
    dma_fence_get(fence);
    dma_fence_put(resv->fence_excl);
    resv->fence_excl = fence;
}

bool dma_resv_test_signaled_rcu(struct dma_resv *resv, bool test_all)
{
    if (!resv->fence_excl)
        return 1;
    return dma_fence_is_signaled(resv->fence_excl);
}

static bool dma_resv_all_fences_signaled(struct dma_resv *resv, bool wait_all)
{
    if (resv->fence_excl && !dma_fence_is_signaled(resv->fence_excl))
        return 0;
    if (wait_all && resv->fences_shared)
    {
        int i;
        for (i = 0; i < resv->fences_shared->shared_count; i++)
        {
            struct dma_fence *fence = resv->fences_shared->shared[i];
            if (fence && !dma_fence_is_signaled(fence))
                return 0;
        }
    }
    return 1;
}

long dma_resv_wait_timeout_rcu(struct dma_resv *resv, bool wait_all,
                               bool intr, unsigned long timeout)
{
    int i;

    if (resv->fence_excl)
        dma_fence_enable_sw_signaling(resv->fence_excl);
    if (wait_all && resv->fences_shared)
    {
        for (i = 0; i < resv->fences_shared->shared_count; i++)
        {
            struct dma_fence *fence = resv->fences_shared->shared[i];
            if (fence)
                dma_fence_enable_sw_signaling(fence);
        }
    }

    if (dma_resv_all_fences_signaled(resv, wait_all))
        return timeout ? timeout : 1;

    unsigned int usecs = jiffies_to_usecs(timeout);

    while (usecs > 0)
    {
        unsigned long step = usecs > 2000 ? 2000 : usecs;
        udelay(step);
        usecs -= step;
        if (dma_resv_all_fences_signaled(resv, wait_all))
            return timeout;
    }

    return 0;
}

struct dma_resv_list *dma_resv_get_list(struct dma_resv *resv)
{
    return resv ? resv->fences_shared : NULL;
}

void dma_resv_init(struct dma_resv *resv)
{
    resv->fence_excl = NULL;
    resv->fences_shared = NULL;
    resv->locked = 0;
    resv->locking_ctx = NULL;
}

void dma_resv_fini(struct dma_resv *resv)
{
    dma_fence_put(resv->fence_excl);
    resv->fence_excl = NULL;
    resv->fences_shared = NULL;
    resv->locked = 0;
    resv->locking_ctx = NULL;
}

int dma_resv_trylock(struct dma_resv *resv)
{
    if (__sync_val_compare_and_swap(&resv->locked, 0, 1) == 0)
    {
        resv->locking_ctx = NULL;
        return 1;
    }
    return 0;
}

int dma_resv_lock(struct dma_resv *resv, struct ww_acquire_ctx *ctx)
{
    while (__sync_val_compare_and_swap(&resv->locked, 0, 1) != 0)
    {
        udelay(1);
    }
    resv->locking_ctx = ctx;
    return 0;
}

int dma_resv_lock_interruptible(struct dma_resv *resv, struct ww_acquire_ctx *ctx)
{
    while (__sync_val_compare_and_swap(&resv->locked, 0, 1) != 0)
    {
#if 0
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
            return -EINTR;
#endif
        udelay(1);
    }
    resv->locking_ctx = ctx;
    return 0;
}

void dma_resv_unlock(struct dma_resv *resv)
{
    resv->locking_ctx = NULL;
    resv->locked = 0;
}

struct ww_acquire_ctx *dma_resv_locking_ctx(struct dma_resv *resv)
{
    return resv->locking_ctx;
}

bool dma_resv_held(struct dma_resv *resv)
{
    return resv->locked != 0;
}

void dma_resv_lock_slow(struct dma_resv *resv, struct ww_acquire_ctx *ctx)
{
    dma_resv_lock(resv, ctx);
}

int dma_resv_lock_slow_interruptible(struct dma_resv *resv, struct ww_acquire_ctx *ctx)
{
    return dma_resv_lock_interruptible(resv, ctx);
}
