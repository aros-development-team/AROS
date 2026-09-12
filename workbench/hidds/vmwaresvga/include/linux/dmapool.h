/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Fixed size DMA-able blocks; RAM is identity mapped so the pool is a
    plain aligned allocator whose bus addresses come from compat_dma_map.
*/
#ifndef _LINUX_DMAPOOL_H_
#define _LINUX_DMAPOOL_H_
#include <linux/dma-mapping.h>

#undef dma_pool_create
#undef dma_pool_destroy

struct dma_pool;

struct dma_pool *dma_pool_create(const char *name, struct device *dev, size_t size, size_t align, size_t boundary);
void dma_pool_destroy(struct dma_pool *pool);
void *dma_pool_alloc(struct dma_pool *pool, gfp_t mem_flags, dma_addr_t *handle);
void *dma_pool_zalloc(struct dma_pool *pool, gfp_t mem_flags, dma_addr_t *handle);
void dma_pool_free(struct dma_pool *pool, void *vaddr, dma_addr_t dma);
#define dmam_pool_create(n, d, s, a, b) dma_pool_create(n, d, s, a, b)
#endif
