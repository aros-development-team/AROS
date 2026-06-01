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
    return test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags);
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

u64 dma_fence_context_alloc(unsigned int num)
{
    static u64 context_counter = 0;
    u64 ret;

    ret = context_counter;
    context_counter += num;
    return ret;
}
