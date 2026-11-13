/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DMA_FENCE_CHAIN_H_
#define _LINUX_DMA_FENCE_CHAIN_H_

#include <linux/dma-fence.h>
struct dma_fence_chain { struct dma_fence base; };
static inline struct dma_fence_chain *to_dma_fence_chain(struct dma_fence *fence) { return NULL; }
static inline struct dma_fence *dma_fence_chain_contained(struct dma_fence *fence) { return fence; }
static inline struct dma_fence_chain *dma_fence_chain_alloc(void) { return NULL; }
static inline void dma_fence_chain_free(struct dma_fence_chain *chain) { }
#define dma_fence_chain_for_each(iter, head) for (iter = (head); 0; )

#endif /* _LINUX_DMA_FENCE_CHAIN_H_ */
