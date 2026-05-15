/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_DMA_
#define _DRM_COMPAT_DMA_

#include "drm_compat_types.h"

/* dma handling */
#define DMA_BIDIRECTIONAL   0
#define DMA_TO_DEVICE       1
#define DMA_FROM_DEVICE     2
#define DMA_ATTR_NON_CONSISTENT (1UL << 3)
#define DMA_ATTR_WEAK_ORDERING (1UL << 1)
#define DMA_ATTR_WRITE_COMBINE (1UL << 2)
#define dma_map_page(a, b, c, d, e)     (dma_addr_t)(b->address + c)
static inline int dma_mapping_error(struct device *dev, dma_addr_t dma_addr) { return 0; }
void dma_unmap_page(struct device *dev, dma_addr_t dma_handle, size_t size, ULONG dir);
void dma_free_attrs(struct device *dev, size_t size, void *cpuaddr, dma_addr_t dma_handle, unsigned long attrs);
void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle, ULONG flags, unsigned long attrs);
void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, ULONG flags);
void dma_free_coherent(struct device *dev, size_t size, void *cpuaddr, dma_addr_t dma_handle);

/* dma fence handling */
struct dma_resv
{
    ULONG dummy;
};
struct dma_fence;
struct dma_fence_ops
{
    const char * (*get_driver_name)(struct dma_fence *fence);
    const char * (*get_timeline_name)(struct dma_fence *fence);
    bool (*enable_signaling)(struct dma_fence *fence);
    bool (*signaled)(struct dma_fence *fence);
    void (*release)(struct dma_fence *fence);
    signed long (*wait)(struct dma_fence *fence, bool intr, signed long timeout);
};
struct dma_fence
{
    struct kref refcount;
    ULONG flags;
    ULONG seqno;
    struct dma_fence_ops *ops;
    IPTR context;
    spinlock_t *lock;
};

#define DMA_FENCE_FLAG_SIGNALED_BIT 0
#define DMA_FENCE_FLAG_USER_BITS 3 /* last */

u64 dma_fence_context_alloc(unsigned int num);
struct dma_fence *dma_fence_get(struct dma_fence *fence);
struct dma_fence *dma_resv_get_excl(struct dma_resv *resv);
void dma_resv_add_excl_fence(struct dma_resv *resv, struct dma_fence *fence);
void dma_resv_add_shared_fence(struct dma_resv *resv, struct dma_fence *fence);
void dma_fence_free(struct dma_fence *fence);
signed long dma_fence_wait(struct dma_fence *fence, bool intr);
bool dma_fence_is_signaled(struct dma_fence *fence);
void dma_fence_init(struct dma_fence *fence, const struct dma_fence_ops *ops, spinlock_t *lock, u64 context, u64 seqno);
void dma_fence_put(struct dma_fence *fence);
int dma_fence_signal_locked(struct dma_fence *fence);
void dma_fence_enable_sw_signaling(struct dma_fence *fence);
void dma_resv_assert_held(struct dma_resv *resv);
int dma_resv_trylock(struct dma_resv *resv);
struct ww_acquire_ctx;
int dma_resv_lock(struct dma_resv *resv, struct ww_acquire_ctx *ctx);
int dma_resv_lock_interruptible(struct dma_resv *resv, struct ww_acquire_ctx *ctx);
void dma_resv_unlock(struct dma_resv *resv);
struct ww_acquire_ctx *dma_resv_locking_ctx(struct dma_resv *resv);
void dma_resv_init(struct dma_resv *resv);
void dma_resv_fini(struct dma_resv *resv);
int dma_resv_reserve_shared(struct dma_resv *resv, unsigned int num);
struct dma_resv_list;
struct dma_resv_list *dma_resv_get_list(struct dma_resv *resv);
bool dma_resv_held(struct dma_resv *resv);
int dma_resv_copy_fences(struct dma_resv *dst, struct dma_resv *src);
long dma_fence_wait_timeout(struct dma_fence *fence, bool intr, unsigned long timeout);
void dma_resv_lock_slow(struct dma_resv *resv, struct ww_acquire_ctx *ctx);
int dma_resv_lock_slow_interruptible(struct dma_resv *resv, struct ww_acquire_ctx *ctx);

#endif /* _DRM_COMPAT_DMA_ */
