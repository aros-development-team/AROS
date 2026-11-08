/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DMA_FENCE_CHAIN_H_
#define _LINUX_DMA_FENCE_CHAIN_H_

#include <linux/dma-fence.h>
#include <linux/irq_work.h>
#include <linux/slab.h>

/*
 * A node of a timeline: the fence it wraps plus the node before it. The
 * node is signalled once its own fence and every earlier node are, and a
 * timeline point is found by walking back from the newest node.
 */
struct dma_fence_chain {
    struct dma_fence base;
    struct dma_fence __rcu *prev;
    u64 prev_seqno;
    struct dma_fence *fence;
    union {
        struct dma_fence_cb cb;
        struct irq_work work;
    };
    spinlock_t lock;
};

extern const struct dma_fence_ops dma_fence_chain_ops;

static inline struct dma_fence_chain *to_dma_fence_chain(struct dma_fence *fence)
{
    if (!fence || fence->ops != &dma_fence_chain_ops)
        return NULL;
    return container_of(fence, struct dma_fence_chain, base);
}

/* the fence a chain node wraps, or the fence itself when it is not a node */
static inline struct dma_fence *dma_fence_chain_contained(struct dma_fence *fence)
{
    struct dma_fence_chain *chain = to_dma_fence_chain(fence);

    return chain ? chain->fence : fence;
}

static inline struct dma_fence_chain *dma_fence_chain_alloc(void)
{
    return kmalloc(sizeof(struct dma_fence_chain), GFP_KERNEL);
}

static inline void dma_fence_chain_free(struct dma_fence_chain *chain)
{
    kfree(chain);
}

/* walks the chain from head, dropping nodes whose fences have signalled */
#define dma_fence_chain_for_each(iter, head)   \
    for (iter = dma_fence_get(head); iter; iter = dma_fence_chain_walk(iter))

struct dma_fence *dma_fence_chain_walk(struct dma_fence *fence);
int dma_fence_chain_find_seqno(struct dma_fence **pfence, uint64_t seqno);
void dma_fence_chain_init(struct dma_fence_chain *chain, struct dma_fence *prev,
    struct dma_fence *fence, uint64_t seqno);

#endif /* _LINUX_DMA_FENCE_CHAIN_H_ */
