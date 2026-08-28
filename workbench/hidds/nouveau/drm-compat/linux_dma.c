/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>

#include <drm-aros/drm_aros_pci.h>

/*
 * RAM is identity mapped, so a mapping is the bus address of the block -
 * from the PCI driver when it translates, the CPU address otherwise - plus
 * cache maintenance on platforms whose bus does not snoop.
 */

static inline bool dma_translates(void)
{
    static int checked = 0, direct = 1;

    if (!checked && pciDriver) {
        IPTR val = TRUE;
        OOP_GetAttr(pciDriver, aHidd_PCIDriver_DirectBus, &val);
        direct = val ? 1 : 0;
        checked = 1;
    }
    return !direct;
}

dma_addr_t compat_dma_map(struct device *dev, void *cpu, size_t size, enum dma_data_direction dir)
{
    dma_addr_t bus;

    compat_dma_sync(dev, cpu, size, dir, true);

    if (dma_translates())
        bus = (dma_addr_t)(IPTR)HIDD_PCIDriver_CPUtoPCI(pciDriver, cpu);
    else
        bus = (dma_addr_t)(IPTR)cpu;
    return bus;
}

void *compat_dma_bus_to_cpu(struct device *dev, dma_addr_t addr)
{
    if (dma_translates())
        return HIDD_PCIDriver_PCItoCPU(pciDriver, (APTR)(IPTR)addr);
    return (void *)(IPTR)addr;
}

void compat_dma_sync(struct device *dev, void *cpu, size_t size, enum dma_data_direction dir, bool for_device)
{
#if !defined(__i386__) && !defined(__x86_64__)
    ULONG len = size;
    /* only a pure device read gets away with a clean; both ways = flush */
    ULONG flags = (dir == DMA_TO_DEVICE) ? DMA_ReadFromRAM : 0;

    if (!cpu || !size)
        return;
    if (for_device)
        CachePreDMA(cpu, &len, flags);
    else
        CachePostDMA(cpu, &len, flags);
#endif
}

/* --- scatterlists ----------------------------------------------------- */

int sg_alloc_table(struct sg_table *table, unsigned int nents, gfp_t gfp_mask)
{
    memset(table, 0, sizeof(*table));
    if (!nents)
        return -EINVAL;
    table->sgl = kcalloc(nents, sizeof(struct scatterlist), gfp_mask);
    if (!table->sgl)
        return -ENOMEM;
    sg_mark_end(&table->sgl[nents - 1]);
    table->nents = table->orig_nents = nents;
    return 0;
}

void sg_free_table(struct sg_table *table)
{
    kfree(table->sgl);
    table->sgl = NULL;
    table->nents = table->orig_nents = 0;
}

int sg_alloc_table_from_pages(struct sg_table *sgt, struct page **pages, unsigned int n_pages,
    unsigned int offset, unsigned long size, gfp_t gfp_mask)
{
    unsigned int chunks, i, cur;
    int ret;

    /* merge runs of contiguous pages */
    chunks = 1;
    for (i = 1; i < n_pages; i++)
        if ((IPTR)pages[i] != (IPTR)pages[i - 1] + PAGE_SIZE)
            chunks++;

    ret = sg_alloc_table(sgt, chunks, gfp_mask);
    if (ret)
        return ret;

    cur = 0;
    for (i = 0; i < chunks; i++) {
        unsigned int j, chunk_size;
        for (j = cur + 1; j < n_pages; j++)
            if ((IPTR)pages[j] != (IPTR)pages[j - 1] + PAGE_SIZE)
                break;
        chunk_size = ((j - cur) << PAGE_SHIFT) - offset;
        if (chunk_size > size)
            chunk_size = size;
        sg_set_page(&sgt->sgl[i], pages[cur], chunk_size, offset);
        size -= chunk_size;
        offset = 0;
        cur = j;
    }
    return 0;
}

size_t sg_copy_from_buffer(struct scatterlist *sgl, unsigned int nents, const void *buf, size_t buflen)
{
    size_t done = 0;
    unsigned int i;
    struct scatterlist *sg;

    for_each_sg(sgl, sg, nents, i) {
        size_t n = min_t(size_t, sg->length, buflen - done);
        memcpy(sg_virt(sg), (const char *)buf + done, n);
        done += n;
        if (done >= buflen)
            break;
    }
    return done;
}

size_t sg_copy_to_buffer(struct scatterlist *sgl, unsigned int nents, void *buf, size_t buflen)
{
    size_t done = 0;
    unsigned int i;
    struct scatterlist *sg;

    for_each_sg(sgl, sg, nents, i) {
        size_t n = min_t(size_t, sg->length, buflen - done);
        memcpy((char *)buf + done, sg_virt(sg), n);
        done += n;
        if (done >= buflen)
            break;
    }
    return done;
}

void __sg_page_iter_start(struct sg_page_iter *piter, struct scatterlist *sglist, unsigned int nents, unsigned long pgoffset)
{
    piter->__pg_advance = 0;
    piter->__nents = nents;
    piter->sg = sglist;
    piter->sg_pgoffset = pgoffset;
}

static unsigned int sg_page_count(struct scatterlist *sg)
{
    return PAGE_ALIGN(sg->offset + sg->length) >> PAGE_SHIFT;
}

static unsigned int sg_dma_page_count(struct scatterlist *sg)
{
    return PAGE_ALIGN(sg->offset + sg_dma_len(sg)) >> PAGE_SHIFT;
}

bool __sg_page_iter_next(struct sg_page_iter *piter)
{
    if (!piter->__nents || !piter->sg)
        return false;

    piter->sg_pgoffset += piter->__pg_advance;
    piter->__pg_advance = 1;

    while (piter->sg_pgoffset >= sg_page_count(piter->sg)) {
        piter->sg_pgoffset -= sg_page_count(piter->sg);
        piter->sg = sg_next(piter->sg);
        if (!--piter->__nents || !piter->sg)
            return false;
    }
    return true;
}

bool __sg_page_iter_dma_next(struct sg_dma_page_iter *dma_iter)
{
    struct sg_page_iter *piter = &dma_iter->base;

    if (!piter->__nents || !piter->sg)
        return false;

    piter->sg_pgoffset += piter->__pg_advance;
    piter->__pg_advance = 1;

    while (piter->sg_pgoffset >= sg_dma_page_count(piter->sg)) {
        piter->sg_pgoffset -= sg_dma_page_count(piter->sg);
        piter->sg = sg_next(piter->sg);
        if (!--piter->__nents || !piter->sg)
            return false;
    }
    return true;
}

int dma_map_sg_attrs(struct device *dev, struct scatterlist *sgl, int nents, enum dma_data_direction dir, unsigned long attrs)
{
    struct scatterlist *sg;
    int i;

    for_each_sg(sgl, sg, nents, i) {
        sg->dma_address = compat_dma_map(dev, sg_virt(sg), sg->length, dir);
        sg->dma_length = sg->length;
    }
    return nents;
}

void dma_unmap_sg_attrs(struct device *dev, struct scatterlist *sgl, int nents, enum dma_data_direction dir, unsigned long attrs)
{
    struct scatterlist *sg;
    int i;

    for_each_sg(sgl, sg, nents, i)
        compat_dma_sync(dev, sg_virt(sg), sg->length, dir, false);
}

int dma_map_sgtable(struct device *dev, struct sg_table *sgt, enum dma_data_direction dir, unsigned long attrs)
{
    int nents = dma_map_sg_attrs(dev, sgt->sgl, sgt->orig_nents, dir, attrs);
    if (nents <= 0)
        return -EINVAL;
    sgt->nents = nents;
    return 0;
}

void dma_unmap_sgtable(struct device *dev, struct sg_table *sgt, enum dma_data_direction dir, unsigned long attrs)
{
    dma_unmap_sg_attrs(dev, sgt->sgl, sgt->orig_nents, dir, attrs);
}

void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sgl, int nelems, enum dma_data_direction dir)
{
    struct scatterlist *sg;
    int i;

    for_each_sg(sgl, sg, nelems, i)
        compat_dma_sync(dev, sg_virt(sg), sg->length, dir, false);
}

void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sgl, int nelems, enum dma_data_direction dir)
{
    struct scatterlist *sg;
    int i;

    for_each_sg(sgl, sg, nelems, i)
        compat_dma_sync(dev, sg_virt(sg), sg->length, dir, true);
}
