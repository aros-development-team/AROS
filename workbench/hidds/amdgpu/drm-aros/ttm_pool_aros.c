/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>

#include <drm/ttm/ttm_pool.h>
#include <drm/ttm/ttm_tt.h>
#include <drm/ttm/ttm_bo.h>

/*
 * The page pool of this port hands every ttm_tt one contiguous block of
 * pages. That is what makes ttm_bo_kmap() of a system-memory object a
 * plain address here: there is no MMU to stitch scattered pages together
 * with, so they are never scattered in the first place. The DMA address of
 * each page is filled in at the same time.
 */

void ttm_pool_init(struct ttm_pool *pool, struct device *dev, int nid, bool use_dma_alloc, bool use_dma32)
{
    memset(pool, 0, sizeof(*pool));
    pool->dev = dev;
    pool->nid = nid;
    pool->use_dma_alloc = use_dma_alloc;
    pool->use_dma32 = use_dma32;
}

void ttm_pool_fini(struct ttm_pool *pool)
{
}

int ttm_pool_alloc(struct ttm_pool *pool, struct ttm_tt *tt, struct ttm_operation_ctx *ctx)
{
    struct page *base;
    unsigned int i;

    if (WARN_ON(!tt->num_pages))
        return -EINVAL;
    if (WARN_ON(tt->pages[0]))
        return -EINVAL;

    /* one block for the whole object; page pointers index into it */
    base = (struct page *)vzalloc((unsigned long)tt->num_pages << PAGE_SHIFT);
    if (!base)
        return -ENOMEM;

    for (i = 0; i < tt->num_pages; i++) {
        tt->pages[i] = nth_page(base, i);
        if (tt->dma_address)
            tt->dma_address[i] = compat_dma_map(pool->dev, page_address(tt->pages[i]), PAGE_SIZE, DMA_BIDIRECTIONAL);
    }
    return 0;
}

void ttm_pool_free(struct ttm_pool *pool, struct ttm_tt *tt)
{
    unsigned int i;

    if (!tt->num_pages || !tt->pages[0])
        return;

    if (tt->dma_address)
        for (i = 0; i < tt->num_pages; i++)
            compat_dma_sync(pool->dev, page_address(tt->pages[i]), PAGE_SIZE, DMA_BIDIRECTIONAL, false);

    vfree(page_address(tt->pages[0]));
    for (i = 0; i < tt->num_pages; i++)
        tt->pages[i] = NULL;
}

int ttm_pool_debugfs(struct ttm_pool *pool, struct seq_file *m)
{
    return 0;
}

void ttm_pool_drop_backed_up(struct ttm_tt *tt)
{
}

long ttm_pool_backup(struct ttm_pool *pool, struct ttm_tt *ttm, const struct ttm_backup_flags *flags)
{
    return -EINVAL;
}

int ttm_pool_restore_and_alloc(struct ttm_pool *pool, struct ttm_tt *tt, const struct ttm_operation_ctx *ctx)
{
    return ttm_pool_alloc(pool, tt, (struct ttm_operation_ctx *)ctx);
}

int ttm_pool_mgr_init(unsigned long num_pages)
{
    return 0;
}

void ttm_pool_mgr_fini(void)
{
}
